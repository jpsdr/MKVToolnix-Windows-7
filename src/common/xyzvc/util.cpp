/** AVC & HEVC video helper functions

   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   \author Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/bit_reader.h"
#include "common/endian.h"
#include "common/xyzvc/types.h"
#include "common/xyzvc/util.h"

namespace mtx::xyzvc {

bool
might_be_xyzvc(memory_c const &buffer) {
  if (buffer.get_size() < 5)
    return false;

  auto marker      = get_uint32_be(buffer.get_buffer());
  auto marker_size = marker        == mtx::xyzvc::NALU_START_CODE ? 4
                   : (marker >> 8) == mtx::xyzvc::NALU_START_CODE ? 3
                   :                                                0;

  if (!marker_size)
    return false;

  auto next_byte = buffer[marker_size];

  // forbidden_zero_bit — MPEG ES & PS have start codes here in the
  // range ≥ 0xb0, which have that bit set.
  if ((next_byte & 0x80) != 0)
    return false;

  return true;
}

// Indexed by frame_packing_arrangement_type, then by whether frame 0 is the
// right view. See RFC 9559, section 5.1.4.1.28.3, table 5. Arrangement type 5
// (temporal interleaving) and up have no StereoMode equivalent and are not
// listed.
static std::array<std::array<stereo_mode_c::mode, 2>, 5> const s_stereo_modes = {{
  { stereo_mode_c::checkerboard_left_first,       stereo_mode_c::checkerboard_right_first       }, // 0
  { stereo_mode_c::column_interleaved_left_first, stereo_mode_c::column_interleaved_right_first }, // 1
  { stereo_mode_c::row_interleaved_left_first,    stereo_mode_c::row_interleaved_right_first    }, // 2
  { stereo_mode_c::side_by_side_left_first,       stereo_mode_c::side_by_side_right_first       }, // 3
  { stereo_mode_c::top_bottom_left_first,         stereo_mode_c::top_bottom_right_first         }, // 4
}};

// Parses the leading fields of a frame packing arrangement SEI message
// (payloadType 45) and maps them to the Matroska StereoMode values.
//
// The syntax is the same in Rec. ITU-T H.264 and Rec. ITU-T H.265, Annex D,
// up to and including content_interpretation_type, which is as far as this
// reads; the two codecs only differ further into the payload. The caller is
// responsible for positioning the reader at the start of the payload and for
// skipping to its end afterwards.
//
// content_interpretation_type 2 means frame 0 is the right view; 0
// (unspecified) and 1 are treated as frame 0 being the left view, which is
// also how FFmpeg interprets them.
//
// Returns nothing for messages that cancel a previous arrangement and for
// arrangement types without a StereoMode equivalent.
std::optional<stereo_mode_c::mode>
parse_frame_packing_arrangement(mtx::bits::reader_c &r) {
  r.get_unsigned_golomb();                                   // frame_packing_arrangement_id

  if (r.get_bit())                                           // frame_packing_arrangement_cancel_flag
    return {};

  auto arrangement_type            = r.get_bits(7);          // frame_packing_arrangement_type
  r.skip_bit();                                              // quincunx_sampling_flag
  auto content_interpretation_type = r.get_bits(6);

  if (arrangement_type >= s_stereo_modes.size())
    return {};

  return s_stereo_modes[arrangement_type][content_interpretation_type == 2 ? 1 : 0];
}

} // namespace mtx::xyzvc
