/** VVC types

   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

*/

#pragma once

#include "common/common_pch.h"

namespace mtx::vvc {

constexpr auto NALU_TYPE_TRAIL       =  0u;
constexpr auto NALU_TYPE_STSA        =  1u;
constexpr auto NALU_TYPE_RADL        =  2u;
constexpr auto NALU_TYPE_RASL        =  3u;
constexpr auto NALU_TYPE_RSV_VCL_4   =  4u;
constexpr auto NALU_TYPE_RSV_VCL_5   =  5u;
constexpr auto NALU_TYPE_RSV_VCL_6   =  6u;
constexpr auto NALU_TYPE_IDR_W_RADL  =  7u;
constexpr auto NALU_TYPE_IDR_N_LP    =  8u;
constexpr auto NALU_TYPE_CRA         =  9u;
constexpr auto NALU_TYPE_GDR         = 10u;
constexpr auto NALU_TYPE_RSV_IRAP_11 = 11u;
constexpr auto NALU_TYPE_OPI         = 12u;
constexpr auto NALU_TYPE_DCI         = 13u;
constexpr auto NALU_TYPE_VPS         = 14u;
constexpr auto NALU_TYPE_SPS         = 15u;
constexpr auto NALU_TYPE_PPS         = 16u;
constexpr auto NALU_TYPE_PREFIX_APS  = 17u;
constexpr auto NALU_TYPE_SUFFIX_APS  = 18u;
constexpr auto NALU_TYPE_PH          = 19u;
constexpr auto NALU_TYPE_AUD         = 20u;
constexpr auto NALU_TYPE_EOS         = 21u;
constexpr auto NALU_TYPE_EOB         = 22u;
constexpr auto NALU_TYPE_PREFIX_SEI  = 23u;
constexpr auto NALU_TYPE_SUFFIX_SEI  = 24u;
constexpr auto NALU_TYPE_FD          = 25u;
constexpr auto NALU_TYPE_RSV_NVCL_26 = 26u;
constexpr auto NALU_TYPE_RSV_NVCL_27 = 27u;
constexpr auto NALU_TYPE_UNSPEC_28   = 28u;
constexpr auto NALU_TYPE_UNSPEC_29   = 29u;
constexpr auto NALU_TYPE_UNSPEC_30   = 30u;
constexpr auto NALU_TYPE_UNSPEC_31   = 31u;

constexpr auto SLICE_TYPE_B          = 0u;
constexpr auto SLICE_TYPE_P          = 1u;
constexpr auto SLICE_TYPE_I          = 2u;

}
