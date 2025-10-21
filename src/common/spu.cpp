/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   helper functions for SPU data (SubPicture Units — subtitles on DVDs)

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/endian.h"
#include "common/spu.h"
#include "common/strings/formatting.h"

namespace mtx::spu {

static std::optional<std::size_t>
find_stop_display_position(uint8_t const *data,
                           std::size_t const buf_size) {
  static debugging_option_c debug{"spu|spu_find_stop_display_position"};

  if (buf_size < 4)
    return {};

  uint32_t control_start = get_uint16_be(&data[2]);
  uint32_t start_off     = 0;
  auto next_off          = control_start;

  while ((start_off != next_off) && (next_off < buf_size)) {
    start_off = next_off;
    next_off  = get_uint16_be(&data[start_off + 2]);

    if (next_off < start_off) {
      mxdebug_if(debug, "spu::extraction_duration: Encountered broken SPU packet (next_off < start_off)\n");
      return {};
    }

    auto off = start_off + 4;
    for (auto type = data[off++]; type != 0xff; type = data[off++]) {
      auto info    = fmt::format("spu_extraction_duration: cmd = {0:02x} ", static_cast<unsigned int>(type));
      auto unknown = false;
      switch(type) {
        case 0x00:
          // Menu ID, 1 byte
          info += "menu ID";
          break;
        case 0x01:
          // Start display
          info += "start display";
          break;
        case 0x02: {
          // Stop display
          auto date = timestamp_c::mpeg(static_cast<int64_t>(get_uint16_be(&data[start_off])) * 1024);
          info     += fmt::format("stop display: {0}\n", mtx::string::format_timestamp(date));
          mxdebug_if(debug, info);
          return start_off;
        }
        case 0x03:
          // Palette
          info += "palette";
          off += 2;
          break;
        case 0x04:
          // Alpha
          info += "alpha";
          off += 2;
          break;
        case 0x05:
          info += "coords";
          off += 6;
          break;
        case 0x06:
          info += "graphic lines";
          off += 4;
          break;
        case 0xff:
          // All done, bye-bye
          info += "done";
          return {};
        default:
          info += fmt::format("unknown (0x{0:02x}), skipping {1} bytes.", type, next_off - off);
          unknown = true;
      }
      mxdebug_if(debug, fmt::format("{0}\n", info));
      if (unknown)
        break;
    }
  }

  return {};
}

timestamp_c
get_duration(uint8_t const *data,
             std::size_t const buf_size) {
  auto position = find_stop_display_position(data, buf_size);
  if (!position || ((*position + 2) > buf_size))
    return {};

  return timestamp_c::mpeg(static_cast<int64_t>(get_uint16_be(&data[*position])) * 1024);
}

void
set_duration(uint8_t *data,
             std::size_t const buf_size,
             timestamp_c const &duration) {
  auto position = find_stop_display_position(data, buf_size);

  if (position && ((*position + 2) <= buf_size))
    put_uint16_be(&data[*position], duration.to_mpeg() / 1024);
}

}
