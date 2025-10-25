/*
  mkvmerge -- utility for splicing together matroska files
  from component media subtypes

  Distributed under the GPL v2
  see the file COPYING for details
  or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

  image handling

  Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/image/jpeg.h"
#include "common/image/png.h"
#include "common/mm_file_io.h"
#include "common/mm_io_x.h"

namespace mtx::image {

std::optional<std::pair<unsigned int, unsigned int>>
get_size(boost::filesystem::path const &file_name) {
  try {
    mm_file_io_c file{file_name.string(), libebml::MODE_READ};

    auto marker = file.read_uint64_be();

    if ((marker & 0xffff'0000'0000'0000ull) == 0xffd8'0000'0000'0000ull)
      return mtx::image::jpeg::get_size(file);

    if (marker == 0x8950'4e47'0d0a'1a0aull)
      return mtx::image::png::get_size(file);

  } catch (mtx::mm_io::exception &) {
  }

  return {};
}

} // namespace mtx::image
