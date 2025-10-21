/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   OS dependant file system & system helper functions

   Written by Moritz Bunkus <mo@bunkus.online>
*/

#include "common/common_pch.h"

#include <mach-o/dyld.h>
#include <CoreFoundation/CoreFoundation.h>

#include "common/at_scope_exit.h"
#include "common/fs_sys_helpers.h"
#include "common/path.h"

namespace mtx::sys {

std::string
normalize_unicode_string(std::string const &src,
                         unicode_normalization_form_e form) {
  if (src.empty())
    return src;

  auto cf_src_ref     = CFStringCreateWithCString(kCFAllocatorDefault, src.c_str(), kCFStringEncodingUTF8);
  auto cf_src_mutable = CFStringCreateMutableCopy(NULL, 0, cf_src_ref);

  at_scope_exit_c cleanup([&]() {
    CFRelease(cf_src_ref);
    CFRelease(cf_src_mutable);
  });

  CFStringNormalize(cf_src_mutable, form == unicode_normalization_form_e::c ? kCFStringNormalizationFormC : kCFStringNormalizationFormD);

  CFIndex byte_length;

  auto whole_string = CFRangeMake(0, CFStringGetLength(cf_src_mutable));
  auto converted    = CFStringGetBytes(cf_src_mutable, whole_string, kCFStringEncodingUTF8, 0, false, NULL, 0, &byte_length);

  if (!converted || (byte_length == 0))
    return {};

  std::string buffer;
  buffer.reserve(byte_length + 1);

  converted = CFStringGetBytes(cf_src_mutable, whole_string, kCFStringEncodingUTF8, 0, false, reinterpret_cast<uint8_t *>(&buffer[0]), byte_length + 1, NULL);
  if (!converted)
    return {};

  return { &buffer[0], static_cast<std::string::size_type>(byte_length) };
}

boost::filesystem::path
get_current_exe_path([[maybe_unused]] std::string const &argv0) {
  std::string file_name;
  file_name.resize(4000);

  while (true) {
    memset(&file_name[0], 0, file_name.size());
    uint32_t size = file_name.size();
    auto result   = _NSGetExecutablePath(&file_name[0], &size);
    file_name.resize(size);

    if (0 == result)
      break;
  }

  return boost::filesystem::absolute(mtx::fs::to_path(file_name)).parent_path();
}

boost::filesystem::path
get_package_data_folder() {
  return get_current_exe_path({}) / "data";
}

}
