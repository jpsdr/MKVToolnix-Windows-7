/*
   mkvtoolnix - programs for manipulating Matroska files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   base64 encoding and decoding functions

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

namespace mtx::base64 {
class exception: public mtx::exception {
public:
  virtual const char *what() const throw() {
    return "unspecified Base64 encoder/decoder error";
  }
};

class invalid_data_x: public exception {
public:
  virtual const char *what() const throw() {
    return Y("Invalid Base64 character encountered");
  }
};

std::string encode(const uint8_t *src, int src_len, bool line_breaks = false, int max_line_len = 72);
memory_cptr decode(std::string const &src);

}
