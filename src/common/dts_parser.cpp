/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   DTS decoder & parser

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/bswap.h"
#include "common/private/dts_parser.h"

namespace mtx::dts {

parser_c::impl_t::~impl_t() {   // NOLINT(modernize-use-equals-default) Need to tell compiler where to put code for this function.
}

void
parser_c::impl_t::reset() {
  swap_bytes   = false;
  pack_14_16   = false;
  first_header = header_t{};

  decode_buffer.clear();
  swap_remainder.clear();
  pack_remainder.clear();
}

// ----------------------------------------------------------------------

parser_c::parser_c()
  : m{new parser_c::impl_t{}}
{
  m->reset();
}

parser_c::~parser_c() {         // NOLINT(modernize-use-equals-default) Need to tell compiler where to put code for this function.
}

void
parser_c::reset() {
  m->reset();
}

bool
parser_c::detect(uint8_t const *buffer,
                 std::size_t size,
                 std::size_t num_required_headers) {
  for (auto swap_bytes = 0; swap_bytes < 2; ++swap_bytes) {
    for (auto pack_14_16 = 0; pack_14_16 < 2; ++pack_14_16) {
      reset();

      m->decode_buffer.add(buffer, size);

      m->swap_bytes = !!swap_bytes;
      m->pack_14_16 = !!pack_14_16;

      decode_buffer();

      auto offset = mtx::dts::find_consecutive_headers(m->decode_buffer.get_buffer(), m->decode_buffer.get_size(), num_required_headers);

      mxdebug_if(m->debug, fmt::format("dts::parser::detect: in: size {0} #req headers {1}; result: swap {2} 14/16 {3} offset {4}\n", size, num_required_headers, m->swap_bytes, m->pack_14_16, offset));

      if (offset < 0)
        continue;

      if (0 > find_header(m->decode_buffer.get_buffer() + offset, m->decode_buffer.get_size() - offset, m->first_header))
        continue;

      m->decode_buffer.clear();
      m->swap_remainder.clear();
      m->pack_remainder.clear();

      return true;
    }
  }

  reset();

  return false;
}

bool
parser_c::detect(memory_c &buffer,
                 std::size_t num_required_headers) {
  return detect(buffer.get_buffer(), buffer.get_size(), num_required_headers);
}

header_t
parser_c::get_first_header()
  const {
  return m->first_header;
}

void
parser_c::decode_step(mtx::bytes::buffer_c &remainder_buffer,
                      std::size_t multiples_of,
                      std::function<void()> const &worker) {
  if (remainder_buffer.get_size()) {
    m->decode_buffer.prepend(remainder_buffer.get_buffer(), remainder_buffer.get_size());
    remainder_buffer.clear();
  }

  auto remaining_bytes = m->decode_buffer.get_size() % multiples_of;
  if (remaining_bytes) {
    remainder_buffer.add(m->decode_buffer.get_buffer() + m->decode_buffer.get_size() - remaining_bytes, remaining_bytes);
    m->decode_buffer.remove(remaining_bytes, mtx::bytes::buffer_c::at_back);
  }

  worker();
}

void
parser_c::decode_buffer() {
  if (m->swap_bytes)
    decode_step(m->swap_remainder, 2, [this]() {
      mtx::bytes::swap_buffer(m->decode_buffer.get_buffer(), m->decode_buffer.get_buffer(), m->decode_buffer.get_size(), 2);
    });

  if (m->pack_14_16)
    decode_step(m->pack_remainder, 16, [this]() {
      auto len_16 = m->decode_buffer.get_size();
      auto len_14 = len_16 * 14 / 16;

      convert_14_to_16_bits(reinterpret_cast<unsigned short *>(m->decode_buffer.get_buffer()), len_16 / 2, reinterpret_cast<unsigned short *>(m->decode_buffer.get_buffer()));
      m->decode_buffer.remove(len_16 - len_14, mtx::bytes::buffer_c::at_back);
    });
}

memory_cptr
parser_c::decode(uint8_t const *buffer,
                 std::size_t size) {
  m->decode_buffer.add(buffer, size);
  decode_buffer();

  auto decoded = memory_c::clone(m->decode_buffer.get_buffer(), m->decode_buffer.get_size());
  m->decode_buffer.clear();

  return decoded;
}

memory_cptr
parser_c::decode(memory_c &buffer) {
  return decode(buffer.get_buffer(), buffer.get_size());
}

}
