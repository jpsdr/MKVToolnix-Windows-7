/** AVC & HEVC video helper functions

   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   \author Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/mpeg.h"
#include "common/xyzvc/types.h"

namespace mtx::xyzvc {

void
slice_info_t::dump()
  const {
  mxinfo(fmt::format("slice_info dump:\n"
                     "  common:\n"
                     "    nalu_type:                       {0}\n"
                     "    slice_type:                      {1}\n"
                     "    pps_id:                          {2}\n"
                     "    sps:                             {3}\n"
                     "    pps:                             {4}\n"
                     "    pic_order_cnt_lsb:               {5}\n"
                     "  AVC-specific:\n"
                     "    nal_ref_idc:                     {6}\n"
                     "    frame_num:                       {7}\n"
                     "    field_pic_flag:                  {8}\n"
                     "    bottom_field_flag:               {9}\n"
                     "    idr_pic_id:                      {10}\n"
                     "    delta_pic_order_cnt_bottom:      {11}\n"
                     "    delta_pic_order_cnt:             {12}\n"
                     "    first_mb_in_slice:               {13}\n"
                     "  HEVC-specific:\n"
                     "    first_slice_segment_in_pic_flag: {14}\n"
                     "    temporal_id:                     {15}\n",
                     static_cast<unsigned int>(nalu_type),
                     static_cast<unsigned int>(slice_type),
                     static_cast<unsigned int>(pps_id),
                     sps,
                     pps,
                     pic_order_cnt_lsb,
                     static_cast<unsigned int>(nal_ref_idc),
                     frame_num,
                     field_pic_flag,
                     bottom_field_flag,
                     idr_pic_id,
                     delta_pic_order_cnt_bottom,
                     delta_pic_order_cnt[0] << 8 | delta_pic_order_cnt[1],
                     first_mb_in_slice,
                     first_slice_segment_in_pic_flag,
                     temporal_id));
}

void
slice_info_t::clear() {
  *this = slice_info_t{};
}

// ------------------------------------------------------------

void
frame_t::clear() {
  *this = frame_t{};
}

bool
frame_t::is_i_frame()
  const {
  return 'I' == m_type;
}

bool
frame_t::is_p_frame()
  const {
  return 'P' == m_type;
}

bool
frame_t::is_b_frame()
  const {
  return 'B' == m_type;
}

bool
frame_t::is_key_frame()
  const {
  return m_keyframe;
}

std::optional<bool>
frame_t::is_discardable()
  const {
  return m_discardable;
}

void
frame_t::combine_nalus_to_data(std::size_t nalu_size_length) {
  if (m_data)
    return;

  auto final_size = 0;

  for (auto const &nalu : m_data_parts)
    final_size += nalu_size_length + nalu->get_size();

  m_data    = memory_c::alloc(final_size);
  auto dest = m_data->get_buffer();

  for (auto const &nalu : m_data_parts) {
    mtx::mpeg::write_nalu_size(dest, nalu->get_size(), nalu_size_length);
    std::memcpy(dest + nalu_size_length, nalu->get_buffer(), nalu->get_size());

    dest += nalu_size_length + nalu->get_size();
  }

  m_data_parts.clear();
}

std::size_t
frame_t::get_data_size()
  const {
  return m_data ? m_data->get_size() : std::accumulate(m_data_parts.begin(), m_data_parts.end(), 0u, [](unsigned int size, auto const &mem) { return size + 4 + mem->get_size(); });
}

}
