/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Ogg Theora helper functions

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/bit_reader.h"
#include "common/math_fwd.h"
#include "common/theora.h"

namespace mtx::theora {

void
parse_identification_header(uint8_t *buffer,
                            int size,
                            identification_header_t &header) {
  mtx::bits::reader_c bc(buffer, size);
  int i;

  header.headertype = bc.get_bits(8);
  if (HEADERTYPE_IDENTIFICATION != header.headertype)
    throw mtx::theora::header_parsing_x(fmt::format(FY("Wrong header type: 0x{0:02x} != 0x{1:02x}"), header.headertype, HEADERTYPE_IDENTIFICATION));

  for (i = 0; 6 > i; ++i)
    header.theora_string[i] = bc.get_bits(8);
  if (strncmp(header.theora_string, "theora", 6))
    throw mtx::theora::header_parsing_x(fmt::format(FY("Wrong identification string: '{0:6s}' != 'theora'"), header.theora_string));

  header.vmaj = bc.get_bits(8);
  header.vmin = bc.get_bits(8);
  header.vrev = bc.get_bits(8);

  if ((3 != header.vmaj) || (2 != header.vmin))
    throw mtx::theora::header_parsing_x(fmt::format(FY("Wrong Theora version: {0}.{1}.{2} != 3.2.x"), header.vmaj, header.vmin, header.vrev));

  header.fmbw = bc.get_bits(16) * 16;
  header.fmbh = bc.get_bits(16) * 16;
  header.picw = bc.get_bits(24);
  header.pich = bc.get_bits(24);
  header.picx = bc.get_bits(8);
  header.picy = bc.get_bits(8);

  header.frn = bc.get_bits(32);
  header.frd = bc.get_bits(32);

  header.parn = bc.get_bits(24);
  header.pard = bc.get_bits(24);

  header.cs = bc.get_bits(8);
  header.nombr = bc.get_bits(24);
  header.qual = bc.get_bits(6);

  header.kfgshift = bc.get_bits(5);

  header.pf = bc.get_bits(2);

  if ((0 != header.parn) && (0 != header.pard)) {
    if (mtx::rational(header.fmbw, header.fmbh) < mtx::rational(header.parn, header.pard)) {
      header.display_width  = mtx::to_int_rounded(mtx::rational(header.fmbw * header.parn, header.pard));
      header.display_height = header.fmbh;
    } else {
      header.display_width  = header.fmbw;
      header.display_height = mtx::to_int_rounded(mtx::rational(header.fmbh * header.pard, header.parn));
    }
  }
}

} // namespace mtx:theora
