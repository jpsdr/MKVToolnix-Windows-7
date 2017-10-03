/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   The "mtx::bits::value_c" class definition

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/memory.h"

#include <ebml/EbmlBinary.h>

namespace mtx { namespace bits {

class value_parser_x: public exception {
protected:
  std::string m_message;
public:
  value_parser_x(const std::string &message)  : m_message(message)       { }
  value_parser_x(const boost::format &message): m_message(message.str()) { }
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
  value_c(const EbmlBinary &elt);
  virtual ~value_c();

  value_c &operator =(const value_c &src);
  bool operator ==(const value_c &cmp) const;
  unsigned char operator [](size_t index) const;

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
  unsigned char *data() const {
    return m_value->get_buffer();
  }
  void zero_content();
};
using value_cptr = std::shared_ptr<value_c>;

}}
