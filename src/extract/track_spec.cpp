/*
   mkvextract -- extract tracks from Matroska files into other files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   extracts tracks and other items from Matroska files into other files

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include "extract/track_spec.h"

track_spec_t::track_spec_t()
  : tid(0)
  , tuid(0)
  , extract_cuesheet(false)
  , target_mode(track_spec_t::tm_normal)
  , extract_blockadd_level(-1)
  , done(false)
{
}

void
track_spec_t::dump(std::string const &prefix)
  const {
  mxinfo(boost::format("%1%tid, tuid:        %2% / %3%\n"
                       "%1%out name:         %4%\n"
                       "%1%sub charset:      %5%\n"
                       "%1%extract cuesheet: %6%\n"
                       "%1%blackadd level:   %7%\n"
                       "%1%target mdoe:      %8%\n")
         % prefix % tid % tuid % out_name % sub_charset % extract_cuesheet % extract_blockadd_level
         % (  target_mode == tm_normal   ? "normal"
            : target_mode == tm_raw      ? "raw"
            : target_mode == tm_full_raw ? "full raw"
            :                              "timestamps"));
}
