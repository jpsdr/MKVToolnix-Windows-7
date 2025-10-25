/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Ogg Theora helper functions

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/error.h"

namespace mtx::theora {

constexpr auto HEADERTYPE_IDENTIFICATION = 0x80;
constexpr auto HEADERTYPE_COMMENT        = 0x81;
constexpr auto HEADERTYPE_SETUP          = 0x82;

class header_parsing_x: public mtx::exception {
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
  char theora_string[6]{};

  uint8_t vmaj{};
  uint8_t vmin{};
  uint8_t vrev{};

  uint16_t fmbw{};
  uint16_t fmbh{};

  uint32_t picw{};
  uint32_t pich{};
  uint8_t picx{};
  uint8_t picy{};

  uint32_t frn{};
  uint32_t frd{};

  uint32_t parn{};
  uint32_t pard{};

  uint8_t cs{};
  uint8_t pf{};

  uint32_t nombr{};
  uint8_t qual{};

  uint8_t kfgshift{};

  int display_width{}, display_height{};
};

void parse_identification_header(uint8_t *buffer, int size, identification_header_t &header);

} // namespace mtx::theora
