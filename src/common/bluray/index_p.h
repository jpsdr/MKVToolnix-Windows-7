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

#include "common/bit_reader.h"
#include "common/debugging.h"

namespace mtx::bluray::index {

class parser_private_c {
public:
  std::string m_file_name;
  bool m_ok{};
  uint32_t m_index_start{};
  debugging_option_c m_debug{"index|index_parser"};

  index_t m_index;

  std::shared_ptr<mtx::bits::reader_c> m_bc;

  parser_private_c(std::string file_name)
    : m_file_name{std::move(file_name)}
  {
  }
};

}
