/*
  mkvmerge -- utility for splicing together matroska files
  from component media subtypes

  Distributed under the GPL v2
  see the file COPYING for details
  or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

  DV demultiplexer module

  Written by Moritz Bunkus <mo@bunkus.online>.
  Probe code from ffmpeg's libavformat.
*/

#include "common/common_pch.h"

#include <algorithm>

#include "common/endian.h"
#include "common/file_types.h"
#include "common/mm_mem_io.h"
#include "input/r_dv.h"
#include "merge/id_result.h"

void
dv_reader_c::probe_file(mm_io_c &in) {
  try {
    uint64_t probe_size = std::min<uint64_t>(in.get_size(), 20 * 1024 * 1024);
    memory_cptr mem     = memory_c::alloc(probe_size);

    if (in.read(mem, probe_size) != probe_size)
      return;

    mm_mem_io_c mem_io(mem->get_buffer(), probe_size);
    uint32_t state             = mem_io.read_uint32_be();
    unsigned matches           = 0;
    unsigned secondary_matches = 0;
    uint64_t marker_pos        = 0;
    uint64_t i;

    for (i = 4; i < probe_size; ++i) {
      if ((state & 0xffffff7f) == 0x1f07003f)
        ++matches;

      // any section header, also with seq/chan num != 0,
      // should appear around every 12000 bytes, at least 10 per frame
      if ((state & 0xff07ff7f) == 0x1f07003f)
        ++secondary_matches;

      if ((0x003f0700 == state) || (0xff3f0700 == state))
        marker_pos = i;

      if ((0xff3f0701 == state) && (80 == (i - marker_pos)))
        ++matches;

      state = (state << 8) | mem_io.read_uint8();
    }

    if (   matches
        && ((probe_size / matches) < (1024 * 1024))
        && (   (matches > 4)
            || (   (secondary_matches                >= 10)
                && ((probe_size / secondary_matches) <  24000)))) {
      id_result_container_unsupported(in.get_file_name(), mtx::file_type_t::get_name(mtx::file_type_e::dv));
    }

  } catch (...) {
  }
}
