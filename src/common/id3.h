/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   helper functions for ID3 tags

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

namespace mtx::id3 {

int skip_v2_tag(mm_io_c &io);
int v1_tag_present_at_end(mm_io_c &io);
int v2_tag_present_at_end(mm_io_c &io);
int tag_present_at_end(mm_io_c &io);

}
