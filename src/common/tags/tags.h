/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   definition of functions helping dealing with tags

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include <optional>

#include "common/bcp47.h"
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

namespace mtx::tags {

void remove_track_uid_targets(libebml::EbmlMaster *tag);
void remove_elements_unsupported_by_webm(libebml::EbmlMaster &master);
bool remove_track_statistics(libmatroska::KaxTags *tags, std::optional<uint64_t> track_uid);
bool remove_track_tags(libmatroska::KaxTags *tags, std::optional<uint64_t> track_uid_to_remove = std::nullopt);

libmatroska::KaxTags *select_for_chapters(libmatroska::KaxTags &tags, libmatroska::KaxChapters &chapters);

libmatroska::KaxTagSimple &find_simple(const std::string &name, libebml::EbmlMaster &m);
libmatroska::KaxTagSimple &find_simple(const libebml::UTFstring &name, libebml::EbmlMaster &m);
std::string get_simple_value(const std::string &name, libebml::EbmlMaster &m);
int64_t get_tuid(const libmatroska::KaxTag &tag);
int64_t get_cuid(const libmatroska::KaxTag &tag);

std::string get_simple_name(const libmatroska::KaxTagSimple &tag);
std::string get_simple_value(const libmatroska::KaxTagSimple &tag);

void set_simple_name(libmatroska::KaxTagSimple &tag, const std::string &name);
void set_simple_value(libmatroska::KaxTagSimple &tag, const std::string &value);
void set_simple(libmatroska::KaxTagSimple &tag, const std::string &name, const std::string &value);
void set_simple(libmatroska::KaxTag &tag, std::string const &name, std::string const &value, mtx::bcp47::language_c const &language = {});
void set_target_type(libmatroska::KaxTag &tag, target_type_e target_type_value, std::string const &target_type);

int count_simple(libebml::EbmlMaster &master);

void convert_old(libmatroska::KaxTags &tags);

template<typename T>
bool
remove_simple_tags_for(libmatroska::KaxTags &tags,
                       std::optional<uint64_t> id,
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
      auto targets   = find_child<libmatroska::KaxTagTargets>(*tag);
      auto actual_id = targets ? std::optional<uint64_t>{ find_child_value<T>(*targets, 0llu) } : std::optional<uint64_t>{ std::nullopt };

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

std::shared_ptr<libmatroska::KaxTags> merge(std::shared_ptr<libmatroska::KaxTags> const &t1, std::shared_ptr<libmatroska::KaxTags> const &t2);

}
