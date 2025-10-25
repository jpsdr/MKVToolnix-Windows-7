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

#include "common/math_fwd.h"

namespace mtx::mpeg1_2 {

// MPEG-1/-2 video start codes
constexpr auto PICTURE_START_CODE            = 0x00000100u;
constexpr auto SLICE_START_CODE_LOWER        = 0x00000101u;
constexpr auto SLICE_START_CODE_UPPER        = 0x000001afu;
constexpr auto USER_DATA_START_CODE          = 0x000001b2u;
constexpr auto SEQUENCE_HEADER_START_CODE    = 0x000001b3u;
constexpr auto SEQUENCE_ERROR_START_CODE     = 0x000001b4u;
constexpr auto EXT_START_CODE                = 0x000001b5u;
constexpr auto SEQUENCE_END_CODE             = 0x000001b7u;
constexpr auto GROUP_OF_PICTURES_START_CODE  = 0x000001b8u;
constexpr auto MPEG_PROGRAM_END_CODE         = 0x000001b9u;
constexpr auto PACKET_START_CODE             = 0x000001bau;
constexpr auto SYSTEM_HEADER_START_CODE      = 0x000001bbu;
constexpr auto PROGRAM_STREAM_MAP_START_CODE = 0x000001bcu;

// MPEG transport stream stram IDs
constexpr auto PROGRAM_STREAM_MAP            = 0xbcu;
constexpr auto PRIVATE_STREAM_1              = 0xbdu;
constexpr auto PADDING_STREAM                = 0xbeu;
constexpr auto PRIVATE_STREAM_2              = 0xbfu;
constexpr auto ECM_STREAM                    = 0xf0u;
constexpr auto EMM_STREAM                    = 0xf1u;
constexpr auto PROGRAM_STREAM_DIRECTORY      = 0xffu;
constexpr auto DSMCC_STREAM                  = 0xf2u;
constexpr auto ITUTRECH222TYPEE_STREAM       = 0xf8u;

// MPEG-1/-2 video frame rate indices
constexpr auto FPS_23_976                    = 0x01u; // 24000/1001
constexpr auto FPS_24                        = 0x02u; //    24
constexpr auto FPS_25                        = 0x03u; //    25
constexpr auto FPS_29_97                     = 0x04u; // 30000/1001
constexpr auto FPS_30                        = 0x05u; //    30
constexpr auto FPS_50                        = 0x06u; //    50
constexpr auto FPS_59_94                     = 0x07u; // 60000/1001
constexpr auto FPS_60                        = 0x08u; //    60

//MPEG-1/-2 video aspect ratio indices
constexpr auto AR_1_1                        = 0x10u; //  1:1
constexpr auto AR_4_3                        = 0x20u; //  4:3
constexpr auto AR_16_9                       = 0x30u; // 16:9
constexpr auto AR_2_21                       = 0x40u; //  2.21

constexpr auto FOURCC_MPEG1                  = 0x10000001u;
constexpr auto FOURCC_MPEG2                  = 0x10000002u;

int extract_fps_idx(const uint8_t *buffer, int buffer_size);
double get_fps(int idx);
std::optional<mtx_mp_rational_t> extract_aspect_ratio(const uint8_t *buffer, int buffer_size);
bool is_fourcc(uint32_t fourcc);

}
