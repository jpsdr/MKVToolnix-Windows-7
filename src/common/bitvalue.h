/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   The "mtx::bits::value_c" class definition

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/memory.h"

#include <ebml/EbmlBinary.h>

namespace mtx::bits {

class value_parser_x: public exception {
protected:
  std::string m_message;
public:
  value_parser_x(const std::string &message) : m_message{message} { }
  virtual ~value_parser_x() throw() { }

  virtual const char *what() const throw() {
    return m_message.c_str();
  }
};

class value_c {
private:
  memory_cptr m_value;
public:
  value_c(int size);
  value_c(const value_c &src);
  value_c(std::string s, unsigned int allowed_bitlength = 0);
  value_c(const libebml::EbmlBinary &elt);
  virtual ~value_c() = default;

  value_c &operator =(const value_c &src);
  bool operator ==(const value_c &cmp) const;
  uint8_t operator [](size_t index) const;

  inline bool empty() const {
    return 0 == m_value->get_size();
  }
  inline size_t bit_size() const {
    return m_value->get_size() * 8;
  }
  inline size_t byte_size() const {
    return m_value->get_size();
  }
  void generate_random();
  uint8_t *data() const {
    return m_value->get_buffer();
  }
  void zero_content();
};
using value_cptr = std::shared_ptr<value_c>;

}
