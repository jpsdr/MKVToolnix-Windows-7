/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   tag helper functions

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include <set>

#include "common/chapters/chapters.h"
#include "common/ebml.h"
#include "common/strings/editing.h"
#include "common/strings/formatting.h"
#include "common/strings/utf8.h"
#include "common/tags/tags.h"
#include "common/tags/target_type.h"

namespace mtx::tags {

libmatroska::KaxTags *
select_for_chapters(libmatroska::KaxTags &tags,
                    libmatroska::KaxChapters &chapters) {
  libmatroska::KaxTags *new_tags = nullptr;

  for (auto tag_child : tags) {
    auto tag = dynamic_cast<libmatroska::KaxTag *>(tag_child);
    if (!tag)
      continue;

    bool copy              = true;
    libmatroska::KaxTagTargets *targets = find_child<libmatroska::KaxTagTargets>(tag);

    if (targets) {
      for (auto child : *targets) {
        auto t_euid = dynamic_cast<libmatroska::KaxTagEditionUID *>(child);
        if (t_euid && !mtx::chapters::find_edition_with_uid(chapters, t_euid->GetValue())) {
          copy = false;
          break;
        }

        auto t_cuid = dynamic_cast<libmatroska::KaxTagChapterUID *>(child);
        if (t_cuid && !mtx::chapters::find_chapter_with_uid(chapters, t_cuid->GetValue())) {
          copy = false;
          break;
        }
      }
    }

    if (!copy)
      continue;

    if (!new_tags)
      new_tags = new libmatroska::KaxTags;
    new_tags->PushElement(*(tag->Clone()));
  }

  return new_tags;
}

libmatroska::KaxTagSimple &
find_simple(const std::string &name,
            libebml::EbmlMaster &m) {
  return find_simple(to_utfstring(name), m);
}

libmatroska::KaxTagSimple &
find_simple(const libebml::UTFstring &name,
            libebml::EbmlMaster &m) {
  if (is_type<libmatroska::KaxTagSimple>(&m)) {
    libmatroska::KaxTagName *tname = find_child<libmatroska::KaxTagName>(&m);
    if (tname && (name == libebml::UTFstring(*tname)))
      return *static_cast<libmatroska::KaxTagSimple *>(&m);
  }

  size_t i;
  for (i = 0; i < m.ListSize(); i++)
    if (is_type<libmatroska::KaxTag, libmatroska::KaxTagSimple>(m[i]))
      try {
        return find_simple(name, *static_cast<libebml::EbmlMaster *>(m[i]));
      } catch (...) {
      }

  throw false;
}

std::string
get_simple_value(const std::string &name,
                 libebml::EbmlMaster &m) {
  try {
    auto tvalue = find_child<libmatroska::KaxTagString>(&find_simple(name, m));
    if (tvalue)
      return tvalue->GetValueUTF8();
  } catch (...) {
  }

  return "";
}

std::string
get_simple_name(const libmatroska::KaxTagSimple &tag) {
  auto tname = find_child<libmatroska::KaxTagName>(&tag);
  return tname ? tname->GetValueUTF8() : ""s;
}

std::string
get_simple_value(const libmatroska::KaxTagSimple &tag) {
  auto tstring = find_child<libmatroska::KaxTagString>(&tag);
  return tstring ? tstring->GetValueUTF8() : ""s;
}

void
set_simple_name(libmatroska::KaxTagSimple &tag,
                const std::string &name) {
  get_child<libmatroska::KaxTagName>(tag).SetValueUTF8(name);
}

void
set_simple_value(libmatroska::KaxTagSimple &tag,
                 const std::string &value) {
  get_child<libmatroska::KaxTagString>(tag).SetValueUTF8(value);
}

void
set_simple(libmatroska::KaxTagSimple &tag,
           const std::string &name,
           const std::string &value) {
  set_simple_name(tag, name);
  set_simple_value(tag, value);
}

int64_t
get_tuid(const libmatroska::KaxTag &tag) {
  auto targets = find_child<libmatroska::KaxTagTargets>(&tag);
  if (!targets)
    return -1;

  auto tuid = find_child<libmatroska::KaxTagTrackUID>(targets);
  if (!tuid)
    return -1;

  return tuid->GetValue();
}

int64_t
get_cuid(const libmatroska::KaxTag &tag) {
  auto targets = find_child<libmatroska::KaxTagTargets>(&tag);
  if (!targets)
    return -1;

  auto cuid = find_child<libmatroska::KaxTagChapterUID>(targets);
  if (!cuid)
    return -1;

  return cuid->GetValue();
}

/** \brief Convert older tags to current specs
*/
void
convert_old(libmatroska::KaxTags &tags) {
  bool has_level_type = false;
  try {
    find_simple("LEVEL_TYPE"s, tags);
    has_level_type = true;
  } catch (...) {
  }

  size_t tags_idx;
  for (tags_idx = 0; tags_idx < tags.ListSize(); tags_idx++) {
    if (!is_type<libmatroska::KaxTag>(tags[tags_idx]))
      continue;

    libmatroska::KaxTag &tag = *static_cast<libmatroska::KaxTag *>(tags[tags_idx]);

    auto target_type_value = Track;
    size_t tag_idx         = 0;
    while (tag_idx < tag.ListSize()) {
      tag_idx++;
      if (!is_type<libmatroska::KaxTagSimple>(tag[tag_idx - 1]))
        continue;

      libmatroska::KaxTagSimple &simple = *static_cast<libmatroska::KaxTagSimple *>(tag[tag_idx - 1]);

      std::string name  = get_simple_name(simple);
      std::string value = get_simple_value(simple);

      if (name == "CATALOG")
        set_simple_name(simple, "CATALOG_NUMBER");

      else if (name == "DATE")
        set_simple_name(simple, "DATE_RELEASED");

      else if (name == "LEVEL_TYPE") {
        if (value == "MEDIA")
          target_type_value = Album;
        tag_idx--;
        delete tag[tag_idx];
        tag.Remove(tag_idx);
      }
    }

    if (!has_level_type)
      continue;

    auto targets = find_child<libmatroska::KaxTagTargets>(&tag);
    if (targets)
      get_child<libmatroska::KaxTagTargetTypeValue>(*targets).SetValue(target_type_value);
  }
}

int
count_simple(libebml::EbmlMaster &master) {
  int count = 0;

  for (auto child : master)
    if (is_type<libmatroska::KaxTagSimple>(child))
      ++count;

    else if (dynamic_cast<libebml::EbmlMaster *>(child))
      count += count_simple(*static_cast<libebml::EbmlMaster *>(child));

  return count;
}

void
remove_track_uid_targets(libebml::EbmlMaster *tag) {
  for (auto el : *tag) {
    if (!is_type<libmatroska::KaxTagTargets>(el))
      continue;

    libmatroska::KaxTagTargets *targets = static_cast<libmatroska::KaxTagTargets *>(el);
    size_t idx_target      = 0;

    while (targets->ListSize() > idx_target) {
      libebml::EbmlElement *uid_el = (*targets)[idx_target];
      if (is_type<libmatroska::KaxTagTrackUID>(uid_el)) {
        targets->Remove(idx_target);
        delete uid_el;

      } else
        ++idx_target;
    }
  }
}

void
set_simple(libmatroska::KaxTag &tag,
           std::string const &name,
           std::string const &value,
           mtx::bcp47::language_c const &language) {
  libmatroska::KaxTagSimple *k_simple_tag = nullptr;

  for (auto const &element : tag) {
    auto s_tag = dynamic_cast<libmatroska::KaxTagSimple *>(element);
    if (!s_tag || (to_utf8(find_child_value<libmatroska::KaxTagName>(s_tag)) != name))
      continue;

    k_simple_tag = s_tag;
    break;
  }

  if (!k_simple_tag) {
    k_simple_tag = static_cast<libmatroska::KaxTagSimple *>(empty_ebml_master(new libmatroska::KaxTagSimple));
    tag.PushElement(*k_simple_tag);
  }

  get_child<libmatroska::KaxTagName>(k_simple_tag).SetValueUTF8(name);
  get_child<libmatroska::KaxTagString>(k_simple_tag).SetValueUTF8(value);

  if (!language.is_valid())
    return;

  get_child<libmatroska::KaxTagLangue>(k_simple_tag).SetValue(language.get_closest_iso639_2_alpha_3_code());

  if (!mtx::bcp47::language_c::is_disabled())
    get_child<libmatroska::KaxTagLanguageIETF>(k_simple_tag).SetValue(language.format());
}

void
set_target_type(libmatroska::KaxTag &tag,
                target_type_e target_type_value,
                std::string const &target_type) {
  auto &targets = get_child<libmatroska::KaxTagTargets>(tag);

  get_child<libmatroska::KaxTagTargetTypeValue>(targets).SetValue(target_type_value);
  get_child<libmatroska::KaxTagTargetType>(targets).SetValue(target_type);
}

void
remove_elements_unsupported_by_webm(libebml::EbmlMaster &master) {
  static auto s_supported_elements = std::map<uint32_t, bool>{};

  if (s_supported_elements.empty()) {
    s_supported_elements[ EBML_ID(libmatroska::KaxTags).GetValue()               ] = true;
    s_supported_elements[ EBML_ID(libmatroska::KaxTag).GetValue()                ] = true;
    s_supported_elements[ EBML_ID(libmatroska::KaxTagTargets).GetValue()         ] = true;
    s_supported_elements[ EBML_ID(libmatroska::KaxTagTargetTypeValue).GetValue() ] = true;
    s_supported_elements[ EBML_ID(libmatroska::KaxTagTargetType).GetValue()      ] = true;
    s_supported_elements[ EBML_ID(libmatroska::KaxTagTrackUID).GetValue()        ] = true;
    s_supported_elements[ EBML_ID(libmatroska::KaxTagSimple).GetValue()          ] = true;
    s_supported_elements[ EBML_ID(libmatroska::KaxTagName).GetValue()            ] = true;
    s_supported_elements[ EBML_ID(libmatroska::KaxTagLangue).GetValue()          ] = true;
    s_supported_elements[ EBML_ID(libmatroska::KaxTagLanguageIETF).GetValue()    ] = true;
    s_supported_elements[ EBML_ID(libmatroska::KaxTagDefault).GetValue()         ] = true;
    s_supported_elements[ EBML_ID(libmatroska::KaxTagString).GetValue()          ] = true;
    s_supported_elements[ EBML_ID(libmatroska::KaxTagBinary).GetValue()          ] = true;
  }

  auto is_simple = is_type<libmatroska::KaxTagSimple>(master);
  auto idx       = 0u;

  while (idx < master.ListSize()) {
    auto e = master[idx];

    if (e && s_supported_elements[ get_ebml_id(*e).GetValue() ] && !(is_simple && is_type<libmatroska::KaxTagSimple>(e))) {
      ++idx;

      auto sub_master = dynamic_cast<libebml::EbmlMaster *>(e);
      if (sub_master)
        remove_elements_unsupported_by_webm(*sub_master);

    } else {
      delete master[idx];
      master.Remove(idx);
    }
  }
}

bool
remove_track_statistics(libmatroska::KaxTags *tags,
                        std::optional<uint64_t> track_uid) {
  if (!tags)
    return false;

  auto tags_to_discard = std::set<std::string>{
    "_STATISTICS_TAGS",
    "_STATISTICS_WRITING_APP",
    "_STATISTICS_WRITING_DATE_UTC",
  };

  auto const wanted_target_type = static_cast<unsigned int>(mtx::tags::Movie);

  for (auto const &tag_elt : *tags) {
    auto tag = dynamic_cast<libmatroska::KaxTag *>(tag_elt);
    if (!tag)
      continue;

    auto targets = find_child<libmatroska::KaxTagTargets>(tag);
    if (!targets || (find_child_value<libmatroska::KaxTagTargetTypeValue>(targets, wanted_target_type) != wanted_target_type))
      continue;

    for (auto const &simple_tag_elt : *tag) {
      auto simple_tag = dynamic_cast<libmatroska::KaxTagSimple *>(simple_tag_elt);
      if (!simple_tag)
        continue;

      auto simple_tag_name = mtx::tags::get_simple_name(*simple_tag);
      if (simple_tag_name != "_STATISTICS_TAGS")
        continue;

      auto all_to_discard = mtx::string::split(mtx::tags::get_simple_value(*simple_tag), QRegularExpression{"\\s+"});
      for (auto const &to_discard : all_to_discard)
        tags_to_discard.insert(to_discard);
    }
  }

  auto removed_something = false;

  for (auto const &tag_name : tags_to_discard) {
    auto removed_something_here = mtx::tags::remove_simple_tags_for<libmatroska::KaxTagTrackUID>(*tags, track_uid, tag_name);
    removed_something           = removed_something || removed_something_here;
  }

  return removed_something;
}

bool
remove_track_tags(libmatroska::KaxTags *tags,
                  std::optional<uint64_t> track_uid_to_remove) {
  if (!tags)
    return false;

  auto idx               = 0u;
  auto removed_something = false;

  while (idx < tags->ListSize()) {
    auto tag_elt = (*tags)[idx];
    auto tag     = dynamic_cast<libmatroska::KaxTag *>(tag_elt);
    if (!tag) {
      ++idx;
      continue;
    }

    auto targets = find_child<libmatroska::KaxTagTargets>(tag);
    if (!targets) {
      ++idx;
      continue;
    }

    auto track_uid_elt = find_child<libmatroska::KaxTagTrackUID>(targets);
    if (!track_uid_elt || (track_uid_to_remove && (*track_uid_to_remove != track_uid_elt->GetValue()))) {
      ++idx;
      continue;
    }

    tags->Remove(idx);
    delete tag_elt;

    removed_something = true;
  }

  return removed_something;
}

namespace {
std::string
convert_tagets_to_index(libmatroska::KaxTagTargets const &targets) {
  std::vector<std::string> properties;

  properties.emplace_back(fmt::format("TargetTypeValue:{}", find_child_value<libmatroska::KaxTagTargetTypeValue>(targets, 50)));
  properties.emplace_back(fmt::format("TargetType:{}",      find_child_value<libmatroska::KaxTagTargetType>(     targets, ""s)));

  for (auto const &child : targets)
    if (dynamic_cast<libebml::EbmlUInteger *>(child) && !dynamic_cast<libmatroska::KaxTagTargetTypeValue *>(child))
      properties.emplace_back(fmt::format("{}:{}", EBML_NAME(child), static_cast<libebml::EbmlUInteger *>(child)->GetValue()));

  std::sort(properties.begin(), properties.end());

  return mtx::string::join(properties, " ");
}

}

std::shared_ptr<libmatroska::KaxTags>
merge(std::shared_ptr<libmatroska::KaxTags> const &t1,
      std::shared_ptr<libmatroska::KaxTags> const &t2) {
  if (!t1 && !t2)
    return {};

  std::map<std::string, libmatroska::KaxTag *> tag_by_target_type;
  auto dest = std::make_shared<libmatroska::KaxTags>();

  for (auto const &src : std::vector<std::shared_ptr<libmatroska::KaxTags>>{t1, t2}) {
    if (!src)
      continue;

    for (auto src_child : *src) {
      if (!dynamic_cast<libmatroska::KaxTag *>(src_child)) {
        delete src_child;
        continue;
      }

      auto tag              = static_cast<libmatroska::KaxTag *>(src_child);
      auto &targets         = get_child<libmatroska::KaxTagTargets>(*tag);
      auto idx              = convert_tagets_to_index(targets);
      auto existing_tag_itr = tag_by_target_type.find(idx);

      if (existing_tag_itr == tag_by_target_type.end()) {
        dest->PushElement(*tag);
        tag_by_target_type[idx] = tag;
        continue;
      }

      for (auto const &tag_child : *tag) {
        if (dynamic_cast<libmatroska::KaxTagTargets *>(tag_child))
          delete tag_child;
        else
          existing_tag_itr->second->PushElement(*tag_child);
      }

      tag->RemoveAll();
    }

    src->RemoveAll();
  }

  return dest;
}

}
