/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   helper functions for APE tags

   Written by James Almer <jamrial@gmail.com>
   Adapted from ID3 parsing code by Moritz Bunkus <mo@bunkus.online>
*/

#pragma once

#include "common/common_pch.h"

namespace mtx::ape {

int tag_present_at_end(mm_io_c &io);

}
