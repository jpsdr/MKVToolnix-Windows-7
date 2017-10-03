/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   helper functions for IVF data

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include "common/bit_reader.h"
#include "common/fourcc.h"
#include "common/ivf.h"

namespace ivf {

file_header_t::file_header_t()
{
  memset(this, 0, sizeof(*this));
}

codec_c
file_header_t::get_codec()
  const {
  auto f = fourcc_c{fourcc};
  return codec_c::look_up(  f.equiv("VP80") ? codec_c::type_e::V_VP8
                          : f.equiv("VP90") ? codec_c::type_e::V_VP9
                          :                   codec_c::type_e::UNKNOWN);
}

frame_header_t::frame_header_t()
{
  memset(this, 0, sizeof(*this));
}

static bool
is_keyframe_vp9(memory_c const &buffer) {
  mtx::bits::reader_c bc{buffer.get_buffer(), buffer.get_size()};

  // frame marker
  if (bc.get_bits(2) != 0x02)
    return false;

  auto profile = bc.get_bit() + (bc.get_bit() * 2);
  if (3 == profile)
    profile += bc.get_bit();

  if (bc.get_bit())             // show_existing_frame
    return false;

  return !bc.get_bit();
}

bool
is_keyframe(memory_cptr const &buffer,
            codec_c::type_e codec) {
  // Remember: bit numbers start with the least significant bit. Bit 0
  // == 1, bit 1 == 2 etc.

  if (!buffer || !buffer->get_size())
    return false;

  auto data = buffer->get_buffer();

  if (codec == codec_c::type_e::V_VP8)
    return (data[0] & 0x01) == 0x00;

  try {
    return is_keyframe_vp9(*buffer);
  } catch (...) {
    return false;
  }
}

};
