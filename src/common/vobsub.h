/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#ifndef MTX_COMMON_VOBSUB_H
#define MTX_COMMON_VOBSUB_H

#include "common/common_pch.h"

#include "common/math.h"

namespace mtx { namespace vobsub {

std::string create_default_index(unsigned int width, unsigned int height, std::string const &palette);

}}

#endif  // MTX_COMMON_VOBSUB_H
