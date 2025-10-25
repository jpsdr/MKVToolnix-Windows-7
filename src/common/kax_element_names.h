/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Written by Moritz Bunkus <mo@bunkus.online>.
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
  static std::string get(libebml::EbmlId const &id);
  static std::string get(libebml::EbmlElement const &elt);

  static void init();
  static void reset();
};

}
