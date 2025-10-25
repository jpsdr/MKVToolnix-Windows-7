/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Simple signature-based prober for unsupported file types.

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/file_types.h"
#include "common/translation.h"
#include "merge/id_result.h"
#include "input/unsupported_types_signature_prober.h"

namespace {

struct sub_signature_t {
  unsigned int offset{};
  std::vector<uint8_t> bytes;

  sub_signature_t(unsigned int _offset,
                  std::vector<uint8_t> const &_bytes)
    : offset{_offset}
    , bytes{_bytes}
  {
  }
};

struct signature_t {
  std::vector<sub_signature_t> sub_signatures;
  translatable_string_c name;

  void match(mm_io_c &in) const;
};

void
signature_t::match(mm_io_c &in)
  const {
  try {
    for (auto const &sub_signature : sub_signatures) {
      in.setFilePointer(sub_signature.offset);

      auto actual_bytes = in.read(sub_signature.bytes.size());

      if (0 != std::memcmp(&sub_signature.bytes[0], actual_bytes->get_buffer(), sub_signature.bytes.size()))
        return;
    }

    id_result_container_unsupported(in.get_file_name(), name);

  } catch (...) {
  }
}

}

void
unsupported_types_signature_prober_c::probe_file(mm_io_c &in) {
  std::vector<signature_t> signatures{
    { { { 0u, { 0x41, 0x44, 0x49, 0x46                          } } }, YT("AAC with ADIF headers") },
    { { { 0u, { 0x30, 0x26, 0xb2, 0x75                          } } }, mtx::file_type_t::get_name(mtx::file_type_e::asf) },
    { { { 0u, { 0x52, 0x49, 0x46, 0x46                          } },
        { 8u, { 0x43, 0x44, 0x58, 0x41                          } } }, mtx::file_type_t::get_name(mtx::file_type_e::cdxa) },
    { { { 0u, { 0x53, 0x50                                      } } }, mtx::file_type_t::get_name(mtx::file_type_e::hdsub) },
    { { { 0u, { 0x2e, 0x52, 0x31, 0x4d                          } } }, YT("Internet Video Recording (IVR)") },
    { { { 0u, { 0xb7, 0xd8, 0x00, 0x20, 0x37, 0x49, 0xda, 0x11,
                0xa6, 0x4e, 0x00, 0x07, 0xe9, 0x5e, 0xad, 0x8d  } } }, YT("Windows Television DVR") },
  };

  for (auto const &signature : signatures)
    signature.match(in);
}
