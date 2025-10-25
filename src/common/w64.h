/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   definitions for the W64 file format

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

namespace mtx::w64 {

struct chunk_t {
  uint8_t guid[16];
  uint64_t size;
};

struct header_t {
  chunk_t riff;
  uint8_t wave_guid[16];
};

extern uint8_t const g_guid_riff[16], g_guid_wave[16], g_guid_fmt[16], g_guid_data[16];

}
