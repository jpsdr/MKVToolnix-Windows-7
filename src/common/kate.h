/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Kate helper functions

   Written by ogg.k.ogg.k <ogg.k.ogg.k@googlemail.com>.
   Adapted from code by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/error.h"

namespace mtx::kate {

constexpr auto HEADERTYPE_IDENTIFICATION = 0x80;

class header_parsing_x: public exception {
protected:
  std::string m_message;
public:
  header_parsing_x(const std::string &message) : m_message{message} { }
  virtual ~header_parsing_x() throw() { }

  virtual const char *what() const throw() {
    return m_message.c_str();
  }
};

struct identification_header_t {
  uint8_t headertype{};
  char kate_string[7]{};

  uint8_t vmaj{};
  uint8_t vmin{};

  uint8_t nheaders{};
  uint8_t tenc{};
  uint8_t tdir{};

  uint8_t kfgshift{};

  int32_t gnum{};
  int32_t gden{};

  std::string language, category;
};

void parse_identification_header(uint8_t const *buffer, int size, identification_header_t &header);

} // namespace mtx::kate
