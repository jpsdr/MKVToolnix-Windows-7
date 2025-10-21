 /*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   IO callback class implementation

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/mm_io_x.h"
#include "common/mm_file_io.h"
#include "common/mm_file_io_p.h"
#include "common/path.h"

bool mm_file_io_private_c::ms_flush_on_close = false;

mm_file_io_c::mm_file_io_c(std::string const &path,
                           libebml::open_mode const mode)
  : mm_io_c{*new mm_file_io_private_c{path, mode}}
{
}

mm_file_io_c::mm_file_io_c(mm_file_io_private_c &p)
  : mm_io_c{p}
{
}

void
mm_file_io_c::prepare_path(const std::string &path) {
  auto directory = mtx::fs::to_path(path).parent_path();
  if (directory.empty() || boost::filesystem::is_directory(directory))
    return;

  boost::system::error_code error_code;
  boost::filesystem::create_directories(directory, error_code);
  if (error_code)
    throw mtx::mm_io::create_directory_x(path, mtx::mm_io::make_error_code());
}

memory_cptr
mm_file_io_c::slurp(std::string const &file_name) {
  mm_file_io_c in(file_name, libebml::MODE_READ);

  // Don't try to retrieve the file size in order to enable reading
  // from pseudo file systems such as /proc on Linux.
  auto const chunk_size = 10 * 1024;
  auto content          = memory_c::alloc(chunk_size);
  auto total_size_read  = 0;

  while (true) {
    auto num_read    = in.read(content->get_buffer() + total_size_read, chunk_size);
    total_size_read += num_read;

    if (num_read != chunk_size) {
      content->resize(total_size_read);
      break;
    }

    content->resize(content->get_size() + chunk_size);
  }

  return content;
}

uint64_t
mm_file_io_c::getFilePointer() {
  return p_func()->current_position;
}

mm_file_io_c::~mm_file_io_c() {
  close();
  p_func()->file_name.clear();
}

std::string
mm_file_io_c::get_file_name()
  const {
  return p_func()->file_name;
}

mm_io_cptr
mm_file_io_c::open(const std::string &path,
                   const libebml::open_mode mode) {
  return mm_io_cptr(new mm_file_io_c(path, mode));
}

void
mm_file_io_c::enable_flushing_on_close(bool enable) {
  mm_file_io_private_c::ms_flush_on_close = enable;
}
