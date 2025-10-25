/*
  mkvmerge -- utility for splicing together matroska files
  from component media subtypes

  Distributed under the GPL v2
  see the file COPYING for details
  or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

  JPEG image handling

  Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/debugging.h"
#include "common/image/jpeg.h"
#include "common/mm_file_io.h"
#include "common/mm_io_x.h"

namespace mtx::image::jpeg {

namespace {
constexpr uint16_t JM_SOI  = 0xffd8;
constexpr uint16_t JM_SOF0 = 0xffc0;
constexpr uint16_t JM_SOF2 = 0xffc2;
constexpr uint16_t JM_RST0 = 0xffd0;
constexpr uint16_t JM_RST7 = 0xffd7;
constexpr uint16_t JM_EOI  = 0xffd9;

debugging_option_c s_debug{"jpeg"};
}

std::optional<std::pair<unsigned int, unsigned int>>
get_size(boost::filesystem::path const &file_name) {
  try {
    mm_file_io_c file{file_name.string(), libebml::MODE_READ};
    return get_size(file);
  } catch (mtx::mm_io::exception &) {
  }

  return {};
}

std::optional<std::pair<unsigned int, unsigned int>>
get_size(mm_io_c &file) {
  try {
    file.setFilePointer(0);

    while (true) {
      auto marker = file.read_uint16_be();

      mxdebug_if(s_debug, fmt::format("jpeg::get_size: marker at {0}: {1:04x}\n", file.getFilePointer() - 2, marker));

      if ((marker & 0xff00) != 0xff00)
        return {};

      if (marker == JM_EOI)
        return {};

      if (marker == 0xffff) {
        file.setFilePointer(-1, libebml::seek_current);
        continue;
      }

      if (   (marker == JM_SOI)
          || (   (marker >= JM_RST0)
              && (marker <= JM_RST7)))
        continue;

      auto data_size = file.read_uint16_be();

      if (data_size < 2)
        return {};

      if ((marker != JM_SOF0) && (marker != JM_SOF2)) {
        file.setFilePointer(data_size - 2, libebml::seek_current);
        continue;
      }

      file.read_uint8();       // data density
      unsigned int height = file.read_uint16_be();
      unsigned int width  = file.read_uint16_be();

      mxdebug_if(s_debug, fmt::format("jpeg::get_size: size found: {0}x{1}\n", width, height));

      return std::make_pair(width, height);
    }

  } catch (mtx::mm_io::exception &) {
  }

  return {};
}

} // namespace mtx::image::jpeg
