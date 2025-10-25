/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   DTS decoder & parser

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

namespace mtx {

namespace bytes {
class buffer_c;
}

namespace dts {

struct header_t;

class parser_c {
protected:
  struct impl_t;
  std::unique_ptr<impl_t> m;

public:
  parser_c();
  virtual ~parser_c();

  bool detect(uint8_t const *buffer, std::size_t size, std::size_t num_required_headers);
  bool detect(memory_c &buffer, std::size_t num_required_headers);
  header_t get_first_header() const;

  memory_cptr decode(uint8_t const *buffer, std::size_t size);
  memory_cptr decode(memory_c &buffer);

  void reset();

protected:
  void decode_buffer();
  void decode_step(mtx::bytes::buffer_c &remainder_buffer, std::size_t multiples_of, std::function<void()> const &worker);
};

}}
