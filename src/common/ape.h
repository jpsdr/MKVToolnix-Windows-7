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

#pragma once

#include "common/common_pch.h"

namespace mtx { namespace ape {

int tag_present_at_end(mm_io_c &io);

}}
