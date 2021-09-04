/** AVC & HEVC video helper functions

   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   \author Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include "common/avc_hevc/types.h"
#include "common/avc_hevc/util.h"
#include "common/endian.h"

namespace mtx::avc_hevc {

bool
might_be_avc_or_hevc(memory_c const &buffer) {
  if (buffer.get_size() < 5)
    return false;

  auto marker      = get_uint32_be(buffer.get_buffer());
  auto marker_size = marker        == mtx::avc_hevc::NALU_START_CODE ? 4
                   : (marker >> 8) == mtx::avc_hevc::NALU_START_CODE ? 3
                   :                                                   0;

  if (!marker_size)
    return false;

  auto next_byte = buffer[marker_size];

  // forbidden_zero_bit — MPEG ES & PS have start codes here in the
  // range ≥ 0xb0, which have that bit set.
  if ((next_byte & 0x80) != 0)
    return false;

  return true;
}

} // namespace mtx::avc_hevc
