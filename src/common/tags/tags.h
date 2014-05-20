/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   definition of functions helping dealing with tags

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#ifndef MTX_COMMON_TAG_COMMON_H
#define MTX_COMMON_TAG_COMMON_H

#include "common/common_pch.h"

#include "common/ebml.h"
#include "common/tags/target_type.h"

namespace libmatroska {
  class KaxTags;
  class KaxTag;
  class KaxTagSimple;
  class KaxChapters;
};

namespace libebml {
  class EbmlElement;
  class EbmlMaster;
};

using namespace libebml;
using namespace libmatroska;

namespace mtx { namespace tags {

void fix_mandatory_elements(EbmlElement *e);
void remove_track_uid_targets(EbmlMaster *tag);

KaxTags *select_for_chapters(KaxTags &tags, KaxChapters &chapters);

KaxTagSimple &find_simple(const std::string &name, EbmlMaster &m);
KaxTagSimple &find_simple(const UTFstring &name, EbmlMaster &m);
std::string get_simple_value(const std::string &name, EbmlMaster &m);
int64_t get_tuid(const KaxTag &tag);
int64_t get_cuid(const KaxTag &tag);

std::string get_simple_name(const KaxTagSimple &tag);
std::string get_simple_value(const KaxTagSimple &tag);

void set_simple_name(KaxTagSimple &tag, const std::string &name);
void set_simple_value(KaxTagSimple &tag, const std::string &value);
void set_simple(KaxTagSimple &tag, const std::string &name, const std::string &value);
void set_simple(KaxTag &tag, std::string const &name, std::string const &value, std::string const &language = "eng");
void set_target_type(KaxTag &tag, target_type_e target_type_value, std::string const &target_type);

int count_simple(EbmlMaster &master);

void convert_old(KaxTags &tags);

template<typename T>
KaxTag *
find_tag_for(KaxTags &tags,
             uint64_t id,
             bool create_if_not_found) {
  for (auto &element : tags) {
    auto tag = dynamic_cast<KaxTag *>(element);
    if (!tag)
      continue;

    auto targets = FindChild<KaxTagTargets>(*tag);
    if (!targets)
      continue;

    auto actual_id = FindChildValue<T>(*targets, 0);
    if (actual_id == id)
      return tag;
  }

  if (!create_if_not_found)
    return nullptr;

  auto tag = new KaxTag;
  tags.PushElement(*tag);

  auto &targets = GetChild<KaxTagTargets>(tag);
  GetChild<T>(targets).SetValue(id);

  return tag;
}

}}

#endif // MTX_COMMON_TAG_COMMON_H
