/** MPEG video helper functions (MPEG 1, 2 and 4)

   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   \file

   \author Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/debugging.h"
#include "common/endian.h"
#include "common/mpeg1_2.h"

namespace mtx::mpeg1_2 {

namespace {
debugging_option_c s_debug{"mpeg1_2"};
}

/** \brief Extract the FPS from a MPEG video sequence header

   This function looks for a MPEG sequence header in a buffer containing
   a MPEG1 or MPEG2 video frame. If such a header is found its
   FPS index is extracted and returned. This index can be mapped to the
   actual number of frames per second with the function
   ::mpeg_video_get_fps

   \param buffer The buffer to search for the header.
   \param buffer_size The buffer size.

   \return The index or \c -1 if no MPEG sequence header was found or
     if the buffer was too small.
*/
int
extract_fps_idx(uint8_t const *buffer,
                int buffer_size) {
  mxdebug_if(s_debug, fmt::format("mpeg_video_fps: start search in {0} bytes\n", buffer_size));
  if (buffer_size < 8) {
    mxdebug_if(s_debug, "mpeg_video_fps: sequence header too small\n");
    return -1;
  }
  auto marker = get_uint32_be(buffer);
  int idx     = 4;
  while ((idx < buffer_size) && (marker != SEQUENCE_HEADER_START_CODE)) {
    marker <<= 8;
    marker |= buffer[idx];
    idx++;
  }

  if ((idx + 3) >= buffer_size) {
    mxdebug_if(s_debug, "mpeg_video_fps: no full sequence header start code found\n");
    return -1;
  }

  mxdebug_if(s_debug, fmt::format("mpeg_video_fps: found sequence header start code at {0}\n", idx - 4));

  return buffer[idx + 3] & 0x0f;
}

/** \brief Extract the aspect ratio from a MPEG video sequence header

   This function looks for a MPEG sequence header in a buffer containing
   a MPEG1 or MPEG2 video frame. If such a header is found its
   aspect ratio is extracted and returned.

   \param buffer The buffer to search for the header.
   \param buffer_size The buffer size.

   \return \c true if a MPEG sequence header was found and \c false otherwise.
*/
std::optional<mtx_mp_rational_t>
extract_aspect_ratio(uint8_t const *buffer,
                     int buffer_size) {
  uint32_t marker;
  int idx;

  mxdebug_if(s_debug, fmt::format("mpeg_video_ar: start search in {0} bytes\n", buffer_size));
  if (buffer_size < 8) {
    mxdebug_if(s_debug, "mpeg_video_ar: sequence header too small\n");
    return {};
  }
  marker = get_uint32_be(buffer);
  idx = 4;
  while ((idx < buffer_size) && (marker != SEQUENCE_HEADER_START_CODE)) {
    marker <<= 8;
    marker |= buffer[idx];
    idx++;
  }
  if (idx >= buffer_size) {
    mxdebug_if(s_debug, "mpeg_video_ar: no sequence header start code found\n");
    return {};
  }

  mxdebug_if(s_debug, fmt::format("mpeg_video_ar: found sequence header start code at {0}\n", idx - 4));
  idx += 3;                     // width and height
  if (idx >= buffer_size) {
    mxdebug_if(s_debug, "mpeg_video_ar: sequence header too small\n");
    return {};
  }

  switch (buffer[idx] & 0xf0) {
    case AR_1_1:  return mtx::rational(1, 1);
    case AR_4_3:  return mtx::rational(4, 3);
    case AR_16_9: return mtx::rational(16, 9);
    case AR_2_21: return mtx::rational(221, 100);
  }

  return {};
}

/** \brief Get the number of frames per second

   Converts the index returned by ::mpeg_video_extract_fps_idx to a number.

   \param idx The index as to convert.

   \return The number of frames per second or \c -1.0 if the index was
     invalid.
*/
double
get_fps(int idx) {
  static const int s_fps[8] = {0, 24, 25, 0, 30, 50, 0, 60};

  return ((idx < 1) || (idx > 8)) ?             -1.0
      : FPS_23_976 == idx         ? 24000.0 / 1001.0
      : FPS_29_97  == idx         ? 30000.0 / 1001.0
      : FPS_59_94  == idx         ? 60000.0 / 1001.0
      :                             s_fps[idx - 1];
}

bool
is_fourcc(uint32_t fourcc) {
  return (FOURCC_MPEG1 == fourcc) || (FOURCC_MPEG2 == fourcc);
}

} // namespace mtx::mpeg1_2
