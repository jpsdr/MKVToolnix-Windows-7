/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   MicroDVD demultiplexer module

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include <QRegularExpression>

#include "common/mm_io_x.h"
#include "common/mm_proxy_io.h"
#include "common/mm_text_io.h"
#include "common/qt.h"
#include "common/strings/formatting.h"
#include "input/r_microdvd.h"
#include "merge/id_result.h"

void
microdvd_reader_c::probe_file(mm_io_c &in) {
  QRegularExpression re("^\\{\\d+?\\}\\{\\d+?\\}.+$");

  std::string line;
  auto line_num = 0u;

  while (line_num < 20) {
    if (!in.getline2(line, 50))
      return;

    mtx::string::strip(line);

    if (!line.empty())
      break;

    ++line_num;
  }

  if (Q(line).contains(re))
    id_result_container_unsupported(in.get_file_name(), mtx::file_type_t::get_name(mtx::file_type_e::microdvd));
}
