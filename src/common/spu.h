/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   helper functions for SPU data (SubPicture Unist — subtitles on DVDs)

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/timestamp.h"

namespace mtx::spu {

timestamp_c get_duration(uint8_t const *data, std::size_t const buf_size);
void set_duration(uint8_t *data, std::size_t const buf_size, timestamp_c const &duration);

}
