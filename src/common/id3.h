/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   helper functions for ID3 tags

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

int skip_id3v2_tag(mm_io_c &io);
int id3v1_tag_present_at_end(mm_io_c &io);
int id3v2_tag_present_at_end(mm_io_c &io);
int id3_tag_present_at_end(mm_io_c &io);
