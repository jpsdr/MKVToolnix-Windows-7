/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   MicroDVD demultiplexer module

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include "common/mm_io_x.h"
#include "common/strings/formatting.h"
#include "input/r_microdvd.h"
#include "merge/id_result.h"

int
microdvd_reader_c::probe_file(mm_text_io_c *in,
                              uint64_t) {
  try {
    boost::regex re("^\\{\\d+?\\}\\{\\d+?\\}.+$", boost::regex::perl);

    in->setFilePointer(0, seek_beginning);

    std::string line;
    auto line_num = 0u;

    while (line_num < 20) {
      line = in->getline(50);
      strip(line);

      if (!line.empty())
        break;

      ++line_num;
    }

    if (boost::regex_match(line, re))
      id_result_container_unsupported(in->get_file_name(), mtx::file_type_t::get_name(mtx::file_type_e::microdvd));

  } catch (mtx::mm_io::end_of_file_x &) {
  }

  return 0;
}
