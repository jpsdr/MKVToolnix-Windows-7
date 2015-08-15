/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include "common/translation.h"
#include "merge/id_result.h"
#include "merge/output_control.h"

void
id_result_container_unsupported(std::string const &filename,
                                translatable_string_c const &info) {
  if (g_identifying) {
    if (g_identify_for_gui)
      mxinfo(boost::format("File '%1%': unsupported container: %2%\n") % filename % info);
    else
      mxinfo(boost::format(Y("File '%1%': unsupported container: %2%\n")) % filename % info);
    mxexit(3);

  } else
    mxerror(boost::format(Y("The file '%1%' is a non-supported file type (%2%).\n")) % filename % info);
}
