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

#include "common/debugging.h"

namespace mtx {

class doc_type_version_handler_c;

class doc_type_version_handler_private_c {
  friend class doc_type_version_handler_c;

  debugging_option_c debug{"doc_type_version|doc_type_version_handler"};

  unsigned int version{1}, read_version{1};

  static std::unordered_map<unsigned int, unsigned int> s_version_by_element, s_read_version_by_element;
  static void init_tables();
};

} // namespace mtx
