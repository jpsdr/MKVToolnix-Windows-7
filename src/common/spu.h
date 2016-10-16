/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   helper functions for SPU data (SubPicture Unist — subtitles on DVDs)

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#ifndef MTX_COMMON_SPU_H
#define MTX_COMMON_SPU_H

#include "common/common_pch.h"

#include "common/timestamp.h"

namespace mtx { namespace spu {

timestamp_c get_duration(unsigned char const *data, std::size_t const buf_size);

}}

#endif // MTX_COMMON_SPU_H
