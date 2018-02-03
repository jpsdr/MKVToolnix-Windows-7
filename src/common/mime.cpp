/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#if HAVE_MAGIC_H
extern "C" {
#include <magic.h>
};
#endif

#include "common/fs_sys_helpers.h"
#include "common/mime.h"
#include "common/mm_file_io.h"

namespace {

std::string
guess_mime_type_by_ext(std::string ext) {
  /* chop off basename */
  auto i = ext.rfind('.');
  if (std::string::npos == i)
    return "";
  ext = balg::to_lower_copy(ext.substr(i + 1));

  for (auto &mime_type : g_mime_types)
    if (brng::find(mime_type.extensions, ext) != mime_type.extensions.end())
      return mime_type.name;

  return "";
}

#if HAVE_MAGIC_H
std::string
guess_mime_type_by_content(magic_t &m,
                           std::string const &file_name) {
  try {
    mm_file_io_c file(file_name);
    uint64_t file_size = file.get_size();
    size_t buffer_size = 0;
    memory_cptr buf    = memory_c::alloc(1024 * 1024);
    size_t i;

    for (i = 1; 3 >= i; ++i) {
      uint64_t bytes_to_read = std::min(file_size - buffer_size, static_cast<uint64_t>(1024 * 1024));

      if (0 == bytes_to_read)
        break;

      if (buf->get_size() < (buffer_size + bytes_to_read))
        buf->resize(buffer_size + bytes_to_read);

      int64_t bytes_read     = file.read(buf->get_buffer() + buffer_size, bytes_to_read);
      buffer_size           += bytes_read;

      std::string mime_type  = magic_buffer(m, buf->get_buffer(), buffer_size);

      if (!mime_type.empty())
        return mime_type;
    }
  } catch (...) {
  }

  return "";
}

std::string
guess_mime_type_internal(std::string ext,
                         bool is_file) {
  std::string ret;
  magic_t m;

  if (!is_file)
    return guess_mime_type_by_ext(ext);

  // In newer versions of libmagic MAGIC_MIME is declared as MAGIC_MIME_TYPE | MAGIC_MIME_ENCODING.
  // Older versions don't know MAGIC_MIME_TYPE, though -- the old MAGIC_MIME is the new MAGIC_MIME_TYPE,
  // and the new MAGIC_MIME has been redefined.
# ifdef MAGIC_MIME_TYPE
  m = magic_open(MAGIC_MIME_TYPE | MAGIC_SYMLINK);
# else  // MAGIC_MIME_TYPE
  m = magic_open(MAGIC_MIME      | MAGIC_SYMLINK);
# endif  // MAGIC_MIME_TYPE

# ifdef SYS_WINDOWS
  auto magic_filename = (mtx::sys::get_installation_path() / "data" / "magic").string();
  if (!m || (-1 == magic_load(m, magic_filename.c_str())))
    return guess_mime_type_by_ext(ext);
# else  // defined(SYS_WINDOWS)
#  ifdef MTX_APPIMAGE
  auto magic_filename = (mtx::sys::get_installation_path() / ".." / "share" / "file" / "magic.mgc").string();
  if (!m || (-1 == magic_load(m, magic_filename.c_str())) || (-1 == magic_load(m, nullptr)))
    return guess_mime_type_by_ext(ext);
#  else
  if (!m || (-1 == magic_load(m, nullptr)))
    return guess_mime_type_by_ext(ext);
#  endif  // defined(MTX_APPIMAGE)
# endif  // defined(SYS_WINDOWS)

  ret = guess_mime_type_by_content(m, ext);
  magic_close(m);

  if (ret == "")
    return guess_mime_type_by_ext(ext);
  else {
    int idx = ret.find(';');
    if (-1 != idx)
      ret.erase(idx);

    if (ret == "application/octet-stream")
      ret = guess_mime_type_by_ext(ext);

    return ret;
  }
}

#else  // HAVE_MAGIC_H

std::string
guess_mime_type_internal(std::string ext,
                         bool) {
  return guess_mime_type_by_ext(ext);
}
#endif  // HAVE_MAGIC_H

} // anonymous namespace

std::string
primary_file_extension_for_mime_type(std::string const &mime_type) {
  auto itr = brng::find_if(g_mime_types, [&mime_type](auto const &m) { return m.name == mime_type; });

  return (itr != g_mime_types.end()) && !itr->extensions.empty() ? itr->extensions[0] : std::string{};
}

std::string
guess_mime_type(std::string ext,
                bool is_file) {
  std::string mime_type = guess_mime_type_internal(ext, is_file);

  if (mime_type.empty())
    return "application/octet-stream";

  else if (mime_type == "application/x-font-ttf")
    return "application/x-truetype-font";

  return mime_type;
}
