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

int count_simple(EbmlMaster &master);

void convert_old(KaxTags &tags);

}}

#endif // MTX_COMMON_TAG_COMMON_H
