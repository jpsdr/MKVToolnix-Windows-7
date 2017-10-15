/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   RIFF CDXA demultiplexer module

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include "common/endian.h"
#include "common/file_types.h"
#include "merge/id_result.h"
#include "input/r_cdxa.h"

bool
cdxa_reader_c::probe_file(mm_io_c *in,
                          uint64_t size) {
  try {
    if (12 > size)
      return false;

    unsigned char buffer[12];

    in->setFilePointer(0, seek_beginning);
    if (in->read(buffer, 12) != 12)
      return false;

    if ((FOURCC('R', 'I', 'F', 'F') == get_uint32_be(&buffer[0])) && (FOURCC('C', 'D', 'X', 'A') == get_uint32_be(&buffer[8]))) {
      id_result_container_unsupported(in->get_file_name(), mtx::file_type_t::get_name(mtx::file_type_e::cdxa));
      // Never reached:
      return true;
    }

  } catch (...) {
  }

  return false;
}
