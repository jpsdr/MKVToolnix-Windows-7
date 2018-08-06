/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   Simple signature-based prober for unsupported file types.

   Written by Moritz Bunkus <moritz@bunkus.org>.
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

int
unsupported_types_signature_prober_c::probe_file(mm_io_c &in) {
  std::vector<signature_t> signatures{
    { { { 0u, { 0x41, 0x44, 0x49, 0x46 } } }, YT("AAC with ADIF headers") },
    { { { 0u, { 0x30, 0x26, 0xb2, 0x75 } } }, mtx::file_type_t::get_name(mtx::file_type_e::asf) },
    { { { 0u, { 0x52, 0x49, 0x46, 0x46 } },
        { 8u, { 0x43, 0x44, 0x58, 0x41 } } }, mtx::file_type_t::get_name(mtx::file_type_e::cdxa) },
    { { { 0u, { 0x53, 0x50 },            } }, mtx::file_type_t::get_name(mtx::file_type_e::hdsub) },
  };

  for (auto const &signature : signatures)
    signature.match(in);

  return 0;
}
