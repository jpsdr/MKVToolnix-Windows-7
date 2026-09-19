/** AVC & HEVC video helper functions

   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   \author Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/stereo_mode.h"

namespace mtx::bits {
class reader_c;
}

namespace mtx::xyzvc {

bool might_be_xyzvc(memory_c const &buffer);
std::optional<stereo_mode_c::mode> parse_frame_packing_arrangement(mtx::bits::reader_c &r);

}
