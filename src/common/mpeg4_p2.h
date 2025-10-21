/** MPEG video helper functions (MPEG 1, 2 and 4)

   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   \file

   \author Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

namespace mtx::mpeg4_p2 {

// Start code for a MPEG-4 part 2 (?) video object plain
constexpr auto VOP_START_CODE                = 0x000001b6u;
constexpr auto VOL_START_CODE                = 0x00000120u;
// Start code for a MPEG-4 part 2 group of pictures
constexpr auto GOP_START_CODE                = 0x000001b3u;
constexpr auto SLICE_START_CODE_LOWER        = 0x00000101u;
constexpr auto SLICE_START_CODE_UPPER        = 0x000001afu;

// Start code for a MPEG-4 part 2 visual object sequence start marker
constexpr auto VOS_START_CODE                = 0x000001b0u;
// Start code for a MPEG-4 part 2 visual object sequence end marker
constexpr auto VOS_END_CODE                  = 0x000001b1u;
// Start code for a MPEG-4 part 2 visual object marker
constexpr auto VISUAL_OBJECT_START_CODE      = 0x000001b5u;

enum mpeg_video_type_e {
  MPEG_VIDEO_NONE = 0,
  MPEG_VIDEO_V1,
  MPEG_VIDEO_V2,
  MPEG_VIDEO_V4_LAYER_2,
  MPEG_VIDEO_V4_LAYER_10
};

enum frame_type_e {
  FRAME_TYPE_I,
  FRAME_TYPE_P,
  FRAME_TYPE_B
};

/** Pointers to MPEG4 video frames and their data

   MPEG4 video can be stored in a "packed" format, e.g. in AVI. This means
   that one AVI chunk may contain more than one video frame. This is
   usually the case with B frames due to limitations in how AVI and
   Windows' media frameworks work. With ::mpeg4_find_frame_types
   such packed frames can be analyzed. The results are stored in these
   structures: one structure for one frame in the analyzed chunk.
*/
struct video_frame_t {
  /** The beginning of the frame data. This is a pointer into an existing
      buffer handed over to ::mpeg4_find_frame_types. */
  uint8_t *data;
  /** The size of the frame in bytes. */
  int size;
  /** The position of the frame in the original buffer. */
  int pos;
  /** The frame type: \c 'I', \c 'P' or \c 'B'. */
  frame_type_e type;
  /** Private data. */
  uint8_t *priv;
  /** The timestamp of the frame in \c ns. */
  int64_t timestamp;
  /** The duration of the frame in \c ns. */
  int64_t duration;
  /** The frame's backward reference in \c ns relative to its
      \link video_frame_t::timestamp timestamp \endlink.
      This value is only set for P and B frames. */
  int64_t bref;
  /** The frame's forward reference in \c ns relative to its
      \link video_frame_t::timestamp timestamp \endlink.
      This value is only set for B frames. */
  int64_t fref;

  /** Whether or not this frame is actually coded. */
  bool is_coded;

  video_frame_t()
    : data(nullptr)
    , size(0)
    , pos(0)
    , type(FRAME_TYPE_I)
    , priv(nullptr)
    , timestamp(0)
    , duration(0)
    , bref(0)
    , fref(0)
    , is_coded(false)
  {
  }
};

struct config_data_t {
  int m_time_increment_bits;
  int m_width, m_height;
  bool m_width_height_found;

  config_data_t();
};

bool is_fourcc(const void *fourcc);
bool is_v3_fourcc(const void *fourcc);

bool extract_par(const uint8_t *buffer, int buffer_size, uint32_t &par_num, uint32_t &par_den);
bool extract_size(const uint8_t *buffer, int buffer_size, uint32_t &width, uint32_t &height);
void find_frame_types(const uint8_t *buffer, int buffer_size, std::vector<video_frame_t> &frames, const config_data_t &config_data);
memory_cptr parse_config_data(const uint8_t *buffer, int buffer_size, config_data_t &config_data);

}
