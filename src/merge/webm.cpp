/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   helper functions for WebM in mkvmerge

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "merge/webm.h"

static output_compatibility_e s_output_compatibility = OC_MATROSKA;

bool
outputting_webm() {
  return OC_WEBM == s_output_compatibility;
}

void
set_output_compatibility(output_compatibility_e compatibility) {
  s_output_compatibility = compatibility;
}

output_compatibility_e
get_output_compatbility() {
  return s_output_compatibility;
}
