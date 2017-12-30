/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

namespace mtx {

class kax_element_names_c {
protected:
  static std::unordered_map<uint32_t, std::string> ms_names;

public:
  kax_element_names_c() = delete;

  static std::string get(uint32_t id);
  static std::string get(EbmlId const &id) {
    return get(id.GetValue());
  }
  static std::string get(EbmlElement const &elt) {
    return get(EbmlId(elt).GetValue());
  }

  static void init();
  static void reset();
};

}
