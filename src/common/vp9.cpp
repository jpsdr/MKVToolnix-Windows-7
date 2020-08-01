/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html
*/

#include "common/common_pch.h"

#include "common/bit_reader.h"
#include "common/mm_io_x.h"
#include "common/vp9.h"

namespace mtx::vp9 {

namespace {
bool
parse_color_config(mtx::bits::reader_c &r,
                   header_data_t &h) {
  if (h.profile >= 2)
    h.bit_depth = r.get_bit() ? 12 : 10;
  else
    h.bit_depth = 8;

  auto color_space = r.get_bits(3);
  h.subsampling_x  = false;
  h.subsampling_y  = false;

  if (color_space == CS_RGB) {
    r.skip_bit();               // color_range
    if ((h.profile == 1) || (h.profile == 3)) {
      h.subsampling_x = r.get_bit();
      h.subsampling_y = r.get_bit();
    } else {
      h.subsampling_x  = true;
      h.subsampling_y  = true;
    }
  }

  return true;
}

bool
parse_uncompressed_header(mtx::bits::reader_c &r,
                          header_data_t &h) {
  if (r.get_bits(2) != 2)       // frame_marker
    return false;

  h.profile = r.get_bit() + (r.get_bit() << 1);

  if (h.profile == 3)
    r.skip_bit();               // reserved_zero

  if (r.get_bit())              // show_existing_frame
    return false;

  if (r.get_bit())              // frame_type, 0 == KEY_FRAME, 1 == NON_KEY_FRAME
    return false;

  r.skip_bits(2);               // show_frame, error_resilient_mode

  auto frame_sync_marker = r.get_bits(24);
  if (frame_sync_marker != 0x498342)
    return false;

  if (!parse_color_config(r, h))
    return false;

  return true;
}
}

std::optional<header_data_t>
parse_header_data(memory_c const &mem) {
  header_data_t h;

  try {
    mtx::bits::reader_c r{mem.get_buffer(), mem.get_size()};

    if (!parse_uncompressed_header(r, h))
      return std::nullopt;

  } catch (mtx::mm_io::end_of_file_x &) {
    return std::nullopt;
  }

  return h;
}

}
