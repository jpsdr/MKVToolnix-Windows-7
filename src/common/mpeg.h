/** MPEG helper functions

   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   \author Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

namespace mtx { namespace mpeg {

class nalu_size_length_x: public mtx::exception {
protected:
  std::size_t m_required_length;

public:
  nalu_size_length_x(std::size_t required_length)
    : m_required_length(required_length)
  {
  }
  virtual ~nalu_size_length_x() throw() { }

  virtual const char *what() const throw() {
    return "'NALU size' length too small";
  }

  virtual std::string error() const throw() {
    return (boost::format("length of the 'NALU size' field too small: need at least %1% bytes") % m_required_length).str();
  }

  virtual std::size_t get_required_length() const {
    return m_required_length;
  }
};

memory_cptr nalu_to_rbsp(memory_cptr const &buffer);
memory_cptr rbsp_to_nalu(memory_cptr const &buffer);

void write_nalu_size(unsigned char *buffer, std::size_t size, std::size_t nalu_size_length, bool ignore_nalu_size_length_errors = false);
memory_cptr create_nalu_with_size(memory_cptr const &src, std::size_t nalu_size_length, std::vector<memory_cptr> extra_data);

void remove_trailing_zero_bytes(memory_c &buffer);

}}
