/*
  mkvmerge -- utility for splicing together matroska files
  from component media subtypes

  Distributed under the GPL v2
  see the file COPYING for details
  or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

  PNG image handling

  Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/debugging.h"
#include "common/image/png.h"
#include "common/mm_file_io.h"
#include "common/mm_io_x.h"

namespace mtx::image::png {

namespace {

constexpr uint32_t PCT_IHDR = 0x4948'4452;

debugging_option_c s_debug{"png"};
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
    file.setFilePointer(8);

    while (true) {
      auto chunk_length = file.read_uint32_be();
      auto chunk_type   = file.read_uint32_be();

      mxdebug_if(s_debug, fmt::format("png::get_size: type at {0}: {1:08x} length {2}\n", file.getFilePointer() - 8, chunk_type, chunk_length));

      if (chunk_type != PCT_IHDR) {
        file.setFilePointer(chunk_length + 4, libebml::seek_current);
        continue;
      }

      unsigned int width  = file.read_uint32_be();
      unsigned int height = file.read_uint32_be();

      mxdebug_if(s_debug, fmt::format("png::get_size: size found: {0}x{1}\n", width, height));

      return std::make_pair(width, height);
    }

  } catch (mtx::mm_io::exception &) {
  }

  return {};
}

} // namespace mtx::image::png
