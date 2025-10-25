/*
   mkvextract -- extract tracks from Matroska files into other files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   extracts tracks and other items from Matroska files into other files

   Written by Moritz Bunkus <mo@bunkus.online>.
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
  mxinfo(fmt::format("{0}tid, tuid:        {1} / {2}\n"
                     "{0}out name:         {3}\n"
                     "{0}sub charset:      {4}\n"
                     "{0}extract cuesheet: {5}\n"
                     "{0}blackadd level:   {6}\n"
                     "{0}target mdoe:      {7}\n",
                     prefix, tid, tuid, out_name, sub_charset, extract_cuesheet, extract_blockadd_level,
                       target_mode == tm_normal   ? "normal"
                     : target_mode == tm_raw      ? "raw"
                     : target_mode == tm_full_raw ? "full raw"
                     :                              "timestamps"));
}
