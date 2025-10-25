/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Kate helper functions

   Written by ogg.k.ogg.k <ogg.k.ogg.k@googlemail.com>.
   Adapted from code by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/bit_reader.h"
#include "common/kate.h"

namespace mtx::kate {

namespace {
int32_t
get_bits32_le(mtx::bits::reader_c &bc) {
  int32_t v = 0;

  for (int n = 0; n < 4; ++n) {
    v |= bc.get_bits(8) << (n * 8);
  }

  return v;
}

} // anonymous namespace

void
parse_identification_header(uint8_t const *buffer,
                            int size,
                            identification_header_t &header) {
  mtx::bits::reader_c bc(buffer, size);
  int i;

  header.headertype = bc.get_bits(8);
  if (HEADERTYPE_IDENTIFICATION != header.headertype)
    throw mtx::kate::header_parsing_x(fmt::format(FY("Wrong header type: 0x{0:02x} != 0x{1:02x}"), header.headertype, HEADERTYPE_IDENTIFICATION));

  for (i = 0; 7 > i; ++i)
    header.kate_string[i] = bc.get_bits(8);
  if (memcmp(header.kate_string, "kate\0\0\0", 7))
    throw mtx::kate::header_parsing_x(fmt::format(FY("Wrong identification string: '{0:7s}' != 'kate\\0\\0\\0'"), header.kate_string)); /* won't print NULs well, but hey */

  bc.get_bits(8); // we don't need those - they are reserved

  header.vmaj = bc.get_bits(8);
  header.vmin = bc.get_bits(8);

  // do not test vmin, as the header is stable for minor version changes
  static const int supported_version = 0;
  if (header.vmaj > supported_version)
    throw mtx::kate::header_parsing_x(fmt::format(FY("Wrong Kate version: {0}.{1} > {2}.x"), header.vmaj, header.vmin, supported_version));

  header.nheaders = bc.get_bits(8);
  header.tenc     = bc.get_bits(8);
  header.tdir     = bc.get_bits(8);
  bc.get_bits(8);           // we don't need those - they are reserved
  header.kfgshift = bc.get_bits(8);

  bc.skip_bits(32); // from bitstream 0.3, these 32 bits are allocated
  bc.skip_bits(32);         // we don't need those - they are reserved

  header.gnum     = get_bits32_le(bc);
  header.gden     = get_bits32_le(bc);

  auto read_string16 = [&bc]() -> std::string {
    std::string s;
    bool end = false;

    for (int idx = 0; idx < 16; ++idx) {
      auto uc = bc.get_bits(8);
      if (!uc)
        end = true;
      else if (!end)
        s += static_cast<char>(uc);
    }

    return s;
  };

  header.language = read_string16();
  header.category = read_string16();
}

} // namespace mtx::kate
