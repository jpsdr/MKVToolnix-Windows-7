/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   definition of functions helping dealing with tags

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/ebml.h"
#include "common/strings/utf8.h"
#include "common/tags/target_type.h"

namespace libmatroska {
  class KaxTags;
  class KaxTag;
  class KaxTagSimple;
  class KaxChapters;
}

namespace libebml {
  class EbmlElement;
  class EbmlMaster;
}

namespace mtx { namespace tags {

void remove_track_uid_targets(libebml::EbmlMaster *tag);
void remove_elements_unsupported_by_webm(libebml::EbmlMaster &master);
bool remove_track_statistics(libmatroska::KaxTags *tags, boost::optional<uint64_t> track_uid);

libmatroska::KaxTags *select_for_chapters(libmatroska::KaxTags &tags, libmatroska::KaxChapters &chapters);

libmatroska::KaxTagSimple &find_simple(const std::string &name, libebml::EbmlMaster &m);
libmatroska::KaxTagSimple &find_simple(const UTFstring &name, libebml::EbmlMaster &m);
std::string get_simple_value(const std::string &name, libebml::EbmlMaster &m);
int64_t get_tuid(const libmatroska::KaxTag &tag);
int64_t get_cuid(const libmatroska::KaxTag &tag);

std::string get_simple_name(const libmatroska::KaxTagSimple &tag);
std::string get_simple_value(const libmatroska::KaxTagSimple &tag);

void set_simple_name(libmatroska::KaxTagSimple &tag, const std::string &name);
void set_simple_value(libmatroska::KaxTagSimple &tag, const std::string &value);
void set_simple(libmatroska::KaxTagSimple &tag, const std::string &name, const std::string &value);
void set_simple(libmatroska::KaxTag &tag, std::string const &name, std::string const &value, std::string const &language = "eng");
void set_target_type(libmatroska::KaxTag &tag, target_type_e target_type_value, std::string const &target_type);

int count_simple(libebml::EbmlMaster &master);

void convert_old(libmatroska::KaxTags &tags);

template<typename T>
libmatroska::KaxTag *
find_tag_for(libmatroska::KaxTags &tags,
             uint64_t id,
             target_type_e target_type,
             bool create_if_not_found) {
  for (auto &element : tags) {
    auto tag = dynamic_cast<libmatroska::KaxTag *>(element);
    if (!tag)
      continue;

    auto targets = FindChild<libmatroska::KaxTagTargets>(*tag);
    if (!targets)
      continue;

    if (Unknown != target_type) {
      auto actual_target_type = static_cast<target_type_e>(FindChildValue<libmatroska::KaxTagTargetTypeValue>(*targets, 0ull));
      if (actual_target_type != target_type)
        continue;
    }

    auto actual_id = FindChildValue<T>(*targets, 0ull);
    if (actual_id == id)
      return tag;
  }

  if (!create_if_not_found)
    return nullptr;

  auto tag = new libmatroska::KaxTag;
  tags.PushElement(*tag);

  auto &targets = GetChild<libmatroska::KaxTagTargets>(tag);
  GetChild<libmatroska::KaxTagTargetTypeValue>(targets).SetValue(target_type);
  GetChild<T>(targets).SetValue(id);

  return tag;
}

template<typename T>
bool
remove_simple_tags_for(libmatroska::KaxTags &tags,
                       boost::optional<uint64_t> id,
                       std::string const &name_to_remove) {
  auto removed_something = false;
  auto tag_idx = 0u;
  while (tag_idx < tags.ListSize()) {
    auto tag = dynamic_cast<libmatroska::KaxTag *>(tags[tag_idx]);
    if (!tag) {
      ++tag_idx;
      continue;
    }

    if (id) {
      auto targets   = FindChild<libmatroska::KaxTagTargets>(*tag);
      auto actual_id = targets ? boost::optional<uint64_t>{ FindChildValue<T>(*targets, 0llu) } : boost::optional<uint64_t>{ boost::none };

      if (!targets || !actual_id || (*actual_id != *id)) {
        ++tag_idx;
        continue;
      }
    }

    auto simple_idx = 0u;
    while (simple_idx < tag->ListSize()) {
      auto simple = dynamic_cast<libmatroska::KaxTagSimple *>((*tag)[simple_idx]);
      if (!simple || (to_utf8(get_simple_name(*simple)) != name_to_remove))
        ++simple_idx;

      else {
        tag->Remove(simple_idx);
        delete simple;
        removed_something = true;
      }
    }

    if (0 < count_simple(*tag))
      ++tag_idx;

    else {
      tags.Remove(tag_idx);
      delete tag;
      removed_something = true;
    }
  }

  return removed_something;
}

}}
