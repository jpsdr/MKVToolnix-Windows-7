/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   helper functions for APE tags

   Written by James Almer <jamrial@gmail.com>
   Adapted from ID3 parsing code by Moritz Bunkus <moritz@bunkus.org>
*/

#ifndef MTX_COMMON_APE_COMMON_H
#define MTX_COMMON_APE_COMMON_H

#include "common/common_pch.h"

int apev2_tag_present_at_end(mm_io_c &io);
int ape_tag_present_at_end(mm_io_c &io);

#endif // MTX_COMMON_APE_COMMON_H
