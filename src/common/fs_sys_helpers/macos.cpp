/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   OS dependant file system & system helper functions

   Written by Moritz Bunkus <moritz@bunkus.org>
*/

#include "common/common_pch.h"

#include <CoreFoundation/CoreFoundation.h>

#include "common/at_scope_exit.h"
#include "common/fs_sys_helpers.h"

namespace mtx { namespace sys {

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

  converted = CFStringGetBytes(cf_src_mutable, whole_string, kCFStringEncodingUTF8, 0, false, reinterpret_cast<unsigned char *>(&buffer[0]), byte_length + 1, NULL);
  if (!converted)
    return {};

  return { &buffer[0], static_cast<std::string::size_type>(byte_length) };
}

}}
