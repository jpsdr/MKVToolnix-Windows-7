/** MPEG video helper functions (MPEG 1, 2 and 4)

   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   \file

*/

#include "common/common_pch.h"

#include <cmath>

#include "common/bit_reader.h"
#include "common/checksums/base.h"
#include "common/endian.h"
#include "common/hacks.h"
#include "common/mm_io.h"
#include "common/mpeg.h"
#include "common/hevc.h"
#include "common/hevc_es_parser.h"
#include "common/hevcc.h"
#include "common/memory_slice_cursor.h"
#include "common/strings/formatting.h"
#include "common/timestamp.h"

namespace mtx { namespace hevc {

std::unordered_map<int, std::string> es_parser_c::ms_nalu_names_by_type;

void
slice_info_t::dump()
  const {
  mxinfo(boost::format("slice_info dump:\n"
                       "  nalu_type:                  %1%\n"
                       "  type:                       %2%\n"
                       "  pps_id:                     %3%\n"
                       "  pic_order_cnt_lsb:          %4%\n"
                       "  sps:                        %5%\n"
                       "  pps:                        %6%\n")
         % static_cast<unsigned int>(nalu_type)
         % static_cast<unsigned int>(type)
         % static_cast<unsigned int>(pps_id)
         % pic_order_cnt_lsb
         % sps
         % pps);
}

es_parser_c::es_parser_c()
  : m_nalu_size_length(4)
  , m_keep_ar_info(true)
  , m_hevcc_ready(false)
  , m_hevcc_changed(false)
  , m_stream_default_duration(-1)
  , m_forced_default_duration(-1)
  , m_container_default_duration(-1)
  , m_frame_number(0)
  , m_num_skipped_frames(0)
  , m_first_keyframe_found(false)
  , m_recovery_point_valid(false)
  , m_b_frames_since_keyframe(false)
  , m_first_cleanup{true}
  , m_par_found(false)
  , m_max_timestamp(0)
  , m_stream_position(0)
  , m_parsed_position(0)
  , m_have_incomplete_frame(false)
  , m_simple_picture_order{}
  , m_ignore_nalu_size_length_errors(false)
  , m_discard_actual_frames(false)
  , m_debug_keyframe_detection(debugging_c::requested("hevc_parser|hevc_keyframe_detection"))
  , m_debug_nalu_types(        debugging_c::requested("hevc_parser|hevc_nalu_types"))
  , m_debug_timestamps(        debugging_c::requested("hevc_parser|hevc_timestamps"))
  , m_debug_sps_info(          debugging_c::requested("hevc_parser|hevc_sps|hevc_sps_info"))
{
  if (debugging_c::requested("hevc_statistics"))
    init_nalu_names();
}

es_parser_c::~es_parser_c() {
  mxdebug_if(m_debug_timestamps, boost::format("stream_position %1% parsed_position %2%\n") % m_stream_position % m_parsed_position);

  if (!debugging_c::requested("hevc_statistics"))
    return;

  mxdebug(boost::format("HEVC statistics: #frames: out %1% discarded %2% #timestamps: in %3% generated %4% discarded %5% num_fields: %6% num_frames: %7%\n")
          % m_stats.num_frames_out % m_stats.num_frames_discarded % m_stats.num_timestamps_in % m_stats.num_timestamps_generated % m_stats.num_timestamps_discarded
          % m_stats.num_field_slices % m_stats.num_frame_slices);

  static const char *s_type_names[] = {
    "B",  "P",  "I", "unknown"
  };

  mxdebug("hevc: Number of NALUs by type:\n");
  for (int i = 0, size = m_stats.num_nalus_by_type.size(); i < size; ++i)
    if (0 != m_stats.num_nalus_by_type[i])
      mxdebug(boost::format("  %1%: %2%\n") % get_nalu_type_name(i) % m_stats.num_nalus_by_type[i]);

  mxdebug("hevc: Number of slices by type:\n");
  for (int i = 0; 2 >= i; ++i)
    if (0 != m_stats.num_slices_by_type[i])
      mxdebug(boost::format("  %1%: %2%\n") % s_type_names[i] % m_stats.num_slices_by_type[i]);
}

bool
es_parser_c::headers_parsed()
  const {
  return m_hevcc_ready
      && !m_sps_info_list.empty()
      && (m_sps_info_list.front().width  > 0)
      && (m_sps_info_list.front().height > 0);
}

void
es_parser_c::discard_actual_frames(bool discard) {
  m_discard_actual_frames = discard;
}

void
es_parser_c::add_bytes(unsigned char *buffer,
                       size_t size) {
  mtx::mem::slice_cursor_c cursor;
  int marker_size              = 0;
  int previous_marker_size     = 0;
  int previous_pos             = -1;
  uint64_t previous_parsed_pos = m_parsed_position;

  if (m_unparsed_buffer && (0 != m_unparsed_buffer->get_size()))
    cursor.add_slice(m_unparsed_buffer);
  cursor.add_slice(buffer, size);

  if (3 <= cursor.get_remaining_size()) {
    uint32_t marker =                               1 << 24
                    | (unsigned int)cursor.get_char() << 16
                    | (unsigned int)cursor.get_char() <<  8
                    | (unsigned int)cursor.get_char();

    while (1) {
      if (NALU_START_CODE == marker)
        marker_size = 4;
      else if (NALU_START_CODE == (marker & 0x00ffffff))
        marker_size = 3;

      if (0 != marker_size) {
        if (-1 != previous_pos) {
          auto new_size = cursor.get_position() - marker_size - previous_pos - previous_marker_size;
          auto nalu     = memory_c::alloc(new_size);
          cursor.copy(nalu->get_buffer(), previous_pos + previous_marker_size, new_size);
          m_parsed_position = previous_parsed_pos + previous_pos;

          mtx::mpeg::remove_trailing_zero_bytes(*nalu);
          if (nalu->get_size())
            handle_nalu(nalu, m_parsed_position);
        }
        previous_pos         = cursor.get_position() - marker_size;
        previous_marker_size = marker_size;
        marker_size          = 0;
      }

      if (!cursor.char_available())
        break;

      marker <<= 8;
      marker  |= (unsigned int)cursor.get_char();
    }
  }

  if (-1 == previous_pos)
    previous_pos = 0;

  m_stream_position += size;
  m_parsed_position  = previous_parsed_pos + previous_pos;

  int new_size = cursor.get_size() - previous_pos;
  if (0 != new_size) {
    m_unparsed_buffer = memory_c::alloc(new_size);
    cursor.copy(m_unparsed_buffer->get_buffer(), previous_pos, new_size);

  } else
    m_unparsed_buffer.reset();
}

void
es_parser_c::flush() {
  if (m_unparsed_buffer && (5 <= m_unparsed_buffer->get_size())) {
    m_parsed_position += m_unparsed_buffer->get_size();
    auto marker_size   = get_uint32_be(m_unparsed_buffer->get_buffer()) == NALU_START_CODE ? 4 : 3;
    auto nalu_size     = m_unparsed_buffer->get_size() - marker_size;
    handle_nalu(memory_c::clone(m_unparsed_buffer->get_buffer() + marker_size, nalu_size), m_parsed_position - nalu_size);
  }

  m_unparsed_buffer.reset();
  if (m_have_incomplete_frame) {
    m_frames.push_back(m_incomplete_frame);
    m_have_incomplete_frame = false;
  }

  cleanup();
}

void
es_parser_c::add_timestamp(int64_t timestamp) {
  m_provided_timestamps.emplace_back(timestamp, m_stream_position);
  ++m_stats.num_timestamps_in;
}

void
es_parser_c::flush_incomplete_frame() {
  if (!m_have_incomplete_frame || !m_hevcc_ready)
    return;

  m_frames.push_back(m_incomplete_frame);
  m_incomplete_frame.clear();
  m_have_incomplete_frame = false;
}

void
es_parser_c::flush_unhandled_nalus() {
  for (auto const &nalu_with_pos : m_unhandled_nalus)
    handle_nalu(nalu_with_pos.first, nalu_with_pos.second);

  m_unhandled_nalus.clear();
}

void
es_parser_c::handle_slice_nalu(memory_cptr const &nalu,
                               uint64_t nalu_pos) {
  if (!m_hevcc_ready) {
    m_unhandled_nalus.emplace_back(nalu, nalu_pos);
    return;
  }

  slice_info_t si;
  if (!parse_slice(nalu, si))   // no conversion to RBSP; the bit reader takes care of it
    return;

  if (m_have_incomplete_frame && si.first_slice_segment_in_pic_flag)
    flush_incomplete_frame();

  if (m_have_incomplete_frame) {
    memory_c &mem = *(m_incomplete_frame.m_data.get());
    int offset    = mem.get_size();
    mem.resize(offset + m_nalu_size_length + nalu->get_size());
    mtx::mpeg::write_nalu_size(mem.get_buffer() + offset, nalu->get_size(), m_nalu_size_length, m_ignore_nalu_size_length_errors);
    memcpy(mem.get_buffer() + offset + m_nalu_size_length, nalu->get_buffer(), nalu->get_size());

    return;
  }

  bool is_i_slice =  (HEVC_SLICE_TYPE_I == si.type);
  bool is_b_slice =  (HEVC_SLICE_TYPE_B == si.type);

  m_incomplete_frame.m_si       =  si;
  m_incomplete_frame.m_keyframe =  m_recovery_point_valid
                                || (   is_i_slice
                                    && (   (m_debug_keyframe_detection && !m_b_frames_since_keyframe)
                                        || (HEVC_NALU_TYPE_IDR_W_RADL == si.nalu_type)
                                        || (HEVC_NALU_TYPE_IDR_N_LP   == si.nalu_type)
                                        || (HEVC_NALU_TYPE_CRA_NUT    == si.nalu_type)));
  m_incomplete_frame.m_position = nalu_pos;
  m_recovery_point_valid        = false;

  if (m_incomplete_frame.m_keyframe) {
    m_first_keyframe_found    = true;
    m_b_frames_since_keyframe = false;
    cleanup();

  } else
    m_b_frames_since_keyframe |= is_b_slice;

  m_incomplete_frame.m_data = create_nalu_with_size(nalu, true);
  m_have_incomplete_frame   = true;

  ++m_frame_number;
}

void
es_parser_c::handle_vps_nalu(memory_cptr const &nalu) {
  vps_info_t vps_info;

  if (!parse_vps(mpeg::nalu_to_rbsp(nalu), vps_info))
    return;

  size_t i;
  for (i = 0; m_vps_info_list.size() > i; ++i)
    if (m_vps_info_list[i].id == vps_info.id)
      break;

  if (m_vps_info_list.size() == i) {
    m_vps_list.push_back(nalu);
    m_vps_info_list.push_back(vps_info);
    m_hevcc_changed = true;

  } else if (m_vps_info_list[i].checksum != vps_info.checksum) {
    mxverb(2, boost::format("hevc: VPS ID %|1$04x| changed; checksum old %|2$04x| new %|3$04x|\n") % vps_info.id % m_vps_info_list[i].checksum % vps_info.checksum);

    m_vps_info_list[i] = vps_info;
    m_vps_list[i]      = nalu;
    m_hevcc_changed    = true;

    // Update codec private if needed
    if (m_codec_private.vps_data_id == (int) vps_info.id) {
      m_codec_private.profile_space              = vps_info.profile_space;
      m_codec_private.tier_flag                  = vps_info.tier_flag;
      m_codec_private.profile_idc                = vps_info.profile_idc;
      m_codec_private.profile_compatibility_flag = vps_info.profile_compatibility_flag;
      m_codec_private.progressive_source_flag    = vps_info.progressive_source_flag;
      m_codec_private.interlaced_source_flag     = vps_info.interlaced_source_flag;
      m_codec_private.non_packed_constraint_flag = vps_info.non_packed_constraint_flag;
      m_codec_private.frame_only_constraint_flag = vps_info.frame_only_constraint_flag;
      m_codec_private.level_idc                  = vps_info.level_idc;
      m_codec_private.vps_data_id                = vps_info.id;
    }
  }

  // Update codec private if needed
  if (-1 == m_codec_private.vps_data_id) {
    m_codec_private.profile_space              = vps_info.profile_space;
    m_codec_private.tier_flag                  = vps_info.tier_flag;
    m_codec_private.profile_idc                = vps_info.profile_idc;
    m_codec_private.profile_compatibility_flag = vps_info.profile_compatibility_flag;
    m_codec_private.progressive_source_flag    = vps_info.progressive_source_flag;
    m_codec_private.interlaced_source_flag     = vps_info.interlaced_source_flag;
    m_codec_private.non_packed_constraint_flag = vps_info.non_packed_constraint_flag;
    m_codec_private.frame_only_constraint_flag = vps_info.frame_only_constraint_flag;
    m_codec_private.level_idc                  = vps_info.level_idc;
    m_codec_private.vps_data_id                = vps_info.id;
  }

  m_extra_data.push_back(create_nalu_with_size(nalu));
}

void
es_parser_c::handle_sps_nalu(memory_cptr const &nalu) {
  sps_info_t sps_info;

  auto parsed_nalu = parse_sps(mpeg::nalu_to_rbsp(nalu), sps_info, m_vps_info_list, m_keep_ar_info);
  if (!parsed_nalu)
    return;

  parsed_nalu = mpeg::rbsp_to_nalu(parsed_nalu);

  size_t i;
  for (i = 0; m_sps_info_list.size() > i; ++i)
    if (m_sps_info_list[i].id == sps_info.id)
      break;

  bool use_sps_info = true;
  if (m_sps_info_list.size() == i) {
    m_sps_list.push_back(parsed_nalu);
    m_sps_info_list.push_back(sps_info);
    m_hevcc_changed = true;

  } else if (m_sps_info_list[i].checksum != sps_info.checksum) {
    mxverb(2, boost::format("hevc: SPS ID %|1$04x| changed; checksum old %|2$04x| new %|3$04x|\n") % sps_info.id % m_sps_info_list[i].checksum % sps_info.checksum);

    cleanup();

    m_sps_info_list[i] = sps_info;
    m_sps_list[i]      = parsed_nalu;
    m_hevcc_changed    = true;

    // Update codec private if needed
    if (m_codec_private.sps_data_id == (int) sps_info.id) {
      m_codec_private.min_spatial_segmentation_idc = sps_info.min_spatial_segmentation_idc;
      m_codec_private.chroma_format_idc = sps_info.chroma_format_idc;
      m_codec_private.bit_depth_luma_minus8 = sps_info.bit_depth_luma_minus8;
      m_codec_private.bit_depth_chroma_minus8 = sps_info.bit_depth_chroma_minus8;
      m_codec_private.max_sub_layers_minus1 = sps_info.max_sub_layers_minus1;
      m_codec_private.temporal_id_nesting_flag = sps_info.temporal_id_nesting_flag;
    }
  } else
    use_sps_info = false;

  m_extra_data.push_back(create_nalu_with_size(parsed_nalu));

  // Update codec private if needed
  if (-1 == m_codec_private.sps_data_id) {
    m_codec_private.min_spatial_segmentation_idc = sps_info.min_spatial_segmentation_idc;
    m_codec_private.chroma_format_idc = sps_info.chroma_format_idc;
    m_codec_private.bit_depth_luma_minus8 = sps_info.bit_depth_luma_minus8;
    m_codec_private.bit_depth_chroma_minus8 = sps_info.bit_depth_chroma_minus8;
    m_codec_private.max_sub_layers_minus1 = sps_info.max_sub_layers_minus1;
    m_codec_private.temporal_id_nesting_flag = sps_info.temporal_id_nesting_flag;
    m_codec_private.sps_data_id = sps_info.id;
  }

  if (use_sps_info && m_debug_sps_info)
    sps_info.dump();

  if (!use_sps_info)
    return;

  if (!has_stream_default_duration()
      && sps_info.timing_info_valid()) {
    m_stream_default_duration = sps_info.default_duration();
    mxdebug_if(m_debug_timestamps, boost::format("Stream default duration: %1%\n") % m_stream_default_duration);
  }

  if (!m_par_found
      && sps_info.ar_found
      && (0 != sps_info.par_den)) {
    m_par_found = true;
    m_par       = int64_rational_c(sps_info.par_num, sps_info.par_den);
  }
}

void
es_parser_c::handle_pps_nalu(memory_cptr const &nalu) {
  pps_info_t pps_info;

  if (!parse_pps(mpeg::nalu_to_rbsp(nalu), pps_info))
    return;

  size_t i;
  for (i = 0; m_pps_info_list.size() > i; ++i)
    if (m_pps_info_list[i].id == pps_info.id)
      break;

  if (m_pps_info_list.size() == i) {
    m_pps_list.push_back(nalu);
    m_pps_info_list.push_back(pps_info);
    m_hevcc_changed = true;

  } else if (m_pps_info_list[i].checksum != pps_info.checksum) {
    mxverb(2, boost::format("hevc: PPS ID %|1$04x| changed; checksum old %|2$04x| new %|3$04x|\n") % pps_info.id % m_pps_info_list[i].checksum % pps_info.checksum);

    if (m_pps_info_list[i].sps_id != pps_info.sps_id)
      cleanup();

    m_pps_info_list[i] = pps_info;
    m_pps_list[i]      = nalu;
    m_hevcc_changed     = true;
  }

  m_extra_data.push_back(create_nalu_with_size(nalu));
}

void
es_parser_c::handle_sei_nalu(memory_cptr const &nalu) {
  if (parse_sei(mpeg::nalu_to_rbsp(nalu), m_user_data))
    m_extra_data.push_back(create_nalu_with_size(nalu));
}

void
es_parser_c::handle_nalu(memory_cptr const &nalu,
                         uint64_t nalu_pos) {
  if (1 > nalu->get_size())
    return;

  int type = (*(nalu->get_buffer()) >> 1) & 0x3F;

  mxdebug_if(m_debug_nalu_types, boost::format("NALU type 0x%|1$02x| (%2%) size %3%\n") % type % get_nalu_type_name(type) % nalu->get_size());

  ++m_stats.num_nalus_by_type[std::min(type, 63)];

  switch (type) {
    case HEVC_NALU_TYPE_VIDEO_PARAM:
      flush_incomplete_frame();
      handle_vps_nalu(nalu);
      break;

    case HEVC_NALU_TYPE_SEQ_PARAM:
      flush_incomplete_frame();
      handle_sps_nalu(nalu);
      break;

    case HEVC_NALU_TYPE_PIC_PARAM:
      flush_incomplete_frame();
      handle_pps_nalu(nalu);
      break;

    case HEVC_NALU_TYPE_PREFIX_SEI:
      flush_incomplete_frame();
      handle_sei_nalu(nalu);
      break;

    case HEVC_NALU_TYPE_END_OF_SEQ:
    case HEVC_NALU_TYPE_END_OF_STREAM:
    case HEVC_NALU_TYPE_ACCESS_UNIT:
      flush_incomplete_frame();
      break;

    case HEVC_NALU_TYPE_FILLER_DATA:
      // Skip these.
      break;

    case HEVC_NALU_TYPE_TRAIL_N:
    case HEVC_NALU_TYPE_TRAIL_R:
    case HEVC_NALU_TYPE_TSA_N:
    case HEVC_NALU_TYPE_TSA_R:
    case HEVC_NALU_TYPE_STSA_N:
    case HEVC_NALU_TYPE_STSA_R:
    case HEVC_NALU_TYPE_RADL_N:
    case HEVC_NALU_TYPE_RADL_R:
    case HEVC_NALU_TYPE_RASL_N:
    case HEVC_NALU_TYPE_RASL_R:
    case HEVC_NALU_TYPE_BLA_W_LP:
    case HEVC_NALU_TYPE_BLA_W_RADL:
    case HEVC_NALU_TYPE_BLA_N_LP:
    case HEVC_NALU_TYPE_IDR_W_RADL:
    case HEVC_NALU_TYPE_IDR_N_LP:
    case HEVC_NALU_TYPE_CRA_NUT:
      if (!m_hevcc_ready && !m_vps_info_list.empty() && !m_sps_info_list.empty() && !m_pps_info_list.empty()) {
        m_hevcc_ready = true;
        flush_unhandled_nalus();
      }
      handle_slice_nalu(nalu, nalu_pos);
      break;

    default:
      flush_incomplete_frame();
      if (!m_hevcc_ready && !m_vps_info_list.empty() && !m_sps_info_list.empty() && !m_pps_info_list.empty()) {
        m_hevcc_ready = true;
        flush_unhandled_nalus();
      }
      m_extra_data.push_back(create_nalu_with_size(nalu));

      break;
  }
}

bool
es_parser_c::parse_slice(memory_cptr const &nalu,
                         slice_info_t &si) {
  try {
    mtx::bits::reader_c r(nalu->get_buffer(), nalu->get_size());
    r.enable_rbsp_mode();

    unsigned int i;

    memset(&si, 0, sizeof(si));

    r.get_bits(1);                // forbidden_zero_bit
    si.nalu_type = r.get_bits(6); // nal_unit_type
    r.get_bits(6);                // nuh_reserved_zero_6bits
    r.get_bits(3);                // nuh_temporal_id_plus1

    bool RapPicFlag = (si.nalu_type >= 16 && si.nalu_type <= 23); // RapPicFlag
    si.first_slice_segment_in_pic_flag = r.get_bits(1); // first_slice_segment_in_pic_flag

    if (RapPicFlag)
      r.get_bits(1);  // no_output_of_prior_pics_flag

    si.pps_id = r.get_unsigned_golomb();  // slice_pic_parameter_set_id

    size_t pps_idx;
    for (pps_idx = 0; m_pps_info_list.size() > pps_idx; ++pps_idx)
      if (m_pps_info_list[pps_idx].id == si.pps_id)
        break;
    if (m_pps_info_list.size() == pps_idx) {
      mxverb(3, boost::format("slice parser error: PPS not found: %1%\n") % si.pps_id);
      return false;
    }

    pps_info_t &pps = m_pps_info_list[pps_idx];
    size_t sps_idx;
    for (sps_idx = 0; m_sps_info_list.size() > sps_idx; ++sps_idx)
      if (m_sps_info_list[sps_idx].id == pps.sps_id)
        break;
    if (m_sps_info_list.size() == sps_idx)
      return false;

    si.sps = sps_idx;
    si.pps = pps_idx;

    sps_info_t &sps = m_sps_info_list[sps_idx];

    bool dependent_slice_segment_flag = false;
    if (!si.first_slice_segment_in_pic_flag) {
      if (pps.dependent_slice_segments_enabled_flag)
        dependent_slice_segment_flag = r.get_bits(1); // dependent_slice_segment_flag

      auto log2_min_cb_size_y   = sps.log2_min_luma_coding_block_size_minus3 + 3;
      auto log2_ctb_size_y      = log2_min_cb_size_y + sps.log2_diff_max_min_luma_coding_block_size;
      auto ctb_size_y           = 1 << log2_ctb_size_y;
      auto pic_width_in_ctbs_y  = ceil(static_cast<double>(sps.width  / ctb_size_y));
      auto pic_height_in_ctbs_y = ceil(static_cast<double>(sps.height / ctb_size_y));
      auto pic_size_in_ctbs_y   = pic_width_in_ctbs_y * pic_height_in_ctbs_y;
      auto v                    = mtx::math::int_log2(pic_size_in_ctbs_y);

      r.get_bits(v);  // slice_segment_address
    }

    if (!dependent_slice_segment_flag) {
      for (i = 0; i < pps.num_extra_slice_header_bits; i++)
        r.get_bits(1);  // slice_reserved_undetermined_flag[i]

      si.type = r.get_unsigned_golomb();  // slice_type

      if (pps.output_flag_present_flag)
        r.get_bits(1);    // pic_output_flag

      if (sps.separate_colour_plane_flag == 1)
        r.get_bits(1);    // colour_plane_id

      if ( (si.nalu_type != HEVC_NALU_TYPE_IDR_W_RADL) && (si.nalu_type != HEVC_NALU_TYPE_IDR_N_LP) ) {
        si.pic_order_cnt_lsb = r.get_bits(sps.log2_max_pic_order_cnt_lsb); // slice_pic_order_cnt_lsb
      }

      ++m_stats.num_slices_by_type[1 < si.type ? 2 : si.type];
    }

    return true;
  } catch (...) {
    return false;
  }
}

int64_t
es_parser_c::duration_for(slice_info_t const &si)
  const {
  int64_t duration = -1 != m_forced_default_duration                                                  ? m_forced_default_duration * 2
                   : (m_sps_info_list.size() > si.sps) && m_sps_info_list[si.sps].timing_info_valid() ? m_sps_info_list[si.sps].default_duration()
                   : -1 != m_stream_default_duration                                                  ? m_stream_default_duration * 2
                   : -1 != m_container_default_duration                                               ? m_container_default_duration * 2
                   :                                                                                    20000000 * 2;
  return duration;
}

int64_t
es_parser_c::get_most_often_used_duration()
  const {
  int64_t const s_common_default_durations[] = {
    1000000000ll / 50,
    1000000000ll / 25,
    1000000000ll / 60,
    1000000000ll / 30,
    1000000000ll * 1001 / 48000,
    1000000000ll * 1001 / 24000,
    1000000000ll * 1001 / 60000,
    1000000000ll * 1001 / 30000,
  };

  auto most_often = m_duration_frequency.begin();
  for (auto current = m_duration_frequency.begin(); m_duration_frequency.end() != current; ++current)
    if (current->second > most_often->second)
      most_often = current;

  // No duration at all!? No frame?
  if (m_duration_frequency.end() == most_often) {
    mxdebug_if(m_debug_timestamps, boost::format("Duration frequency: none found, using 25 FPS\n"));
    return 1000000000ll / 25;
  }

  auto best = std::make_pair(most_often->first, std::numeric_limits<uint64_t>::max());

  for (auto common_default_duration : s_common_default_durations) {
    uint64_t diff = std::abs(most_often->first - common_default_duration);
    if ((diff < 20000) && (diff < best.second))
      best = std::make_pair(common_default_duration, diff);
  }

  mxdebug_if(m_debug_timestamps, boost::format("Duration frequency. Result: %1%, diff %2%. Best before adjustment: %3%. All: %4%\n")
             % best.first % best.second % most_often->first
             % boost::accumulate(m_duration_frequency, std::string(""), [](std::string const &accu, std::pair<int64_t, int64_t> const &pair) {
                 return accu + (boost::format(" <%1% %2%>") % pair.first % pair.second).str();
               }));

  return best.first;
}

void
es_parser_c::calculate_frame_order() {
  auto frames_begin           = m_frames.begin();
  auto frames_end             = m_frames.end();
  auto frame_itr              = frames_begin;

  auto &idr                   = frame_itr->m_si;
  auto &sps                   = m_sps_info_list[idr.sps];

  auto idx                    = 0u;
  auto prev_pic_order_cnt_msb = 0u;
  auto prev_pic_order_cnt_lsb = 0u;

  m_simple_picture_order      = false;

  while (frames_end != frame_itr) {
    auto &si = frame_itr->m_si;

    if (si.sps != idr.sps) {
      m_simple_picture_order = true;
      break;
    }

    if ((HEVC_NALU_TYPE_IDR_W_RADL == idr.type) || (HEVC_NALU_TYPE_IDR_N_LP == idr.type)) {
      frame_itr->m_presentation_order = 0;
      prev_pic_order_cnt_lsb = prev_pic_order_cnt_msb = 0;
    } else {
      unsigned int poc_msb;
      auto max_poc_lsb = 1u << (sps.log2_max_pic_order_cnt_lsb);
      auto poc_lsb     = si.pic_order_cnt_lsb;

      if (poc_lsb < prev_pic_order_cnt_lsb && (prev_pic_order_cnt_lsb - poc_lsb) >= (max_poc_lsb / 2))
        poc_msb = prev_pic_order_cnt_msb + max_poc_lsb;
      else if (poc_lsb > prev_pic_order_cnt_lsb && (poc_lsb - prev_pic_order_cnt_lsb) > (max_poc_lsb / 2))
        poc_msb = prev_pic_order_cnt_msb - max_poc_lsb;
      else
        poc_msb = prev_pic_order_cnt_msb;

      frame_itr->m_presentation_order = poc_lsb + poc_msb;

      if ((HEVC_NALU_TYPE_RADL_N != idr.type) && (HEVC_NALU_TYPE_RADL_R != idr.type) && (HEVC_NALU_TYPE_RASL_N != idr.type) && (HEVC_NALU_TYPE_RASL_R != idr.type)) {
        prev_pic_order_cnt_lsb = poc_lsb;
        prev_pic_order_cnt_msb = poc_msb;
      }
    }

    frame_itr->m_decode_order = idx;

    ++frame_itr;
    ++idx;
  }
}

std::vector<int64_t>
es_parser_c::calculate_provided_timestamps_to_use() {
  auto frame_idx                     = 0u;
  auto provided_timestamps_idx       = 0u;
  auto const num_frames              = m_frames.size();
  auto const num_provided_timestamps = m_provided_timestamps.size();

  std::vector<int64_t> provided_timestamps_to_use;
  provided_timestamps_to_use.reserve(num_frames);

  while (   (frame_idx               < num_frames)
         && (provided_timestamps_idx < num_provided_timestamps)) {
    timestamp_c timestamp_to_use;
    auto &frame = m_frames[frame_idx];

    while (   (provided_timestamps_idx < num_provided_timestamps)
           && (frame.m_position >= m_provided_timestamps[provided_timestamps_idx].second)) {
      timestamp_to_use = timestamp_c::ns(m_provided_timestamps[provided_timestamps_idx].first);
      ++provided_timestamps_idx;
    }

    if (timestamp_to_use.valid()) {
      provided_timestamps_to_use.emplace_back(timestamp_to_use.to_ns());
      frame.m_has_provided_timestamp = true;
    }

    ++frame_idx;
  }

  mxdebug_if(m_debug_timestamps,
             boost::format("cleanup; num frames %1% num provided timestamps available %2% num provided timestamps to use %3%\n"
                           "  frames:\n%4%"
                           "  provided timestamps (available):\n%5%"
                           "  provided timestamps (to use):\n%6%")
             % num_frames % num_provided_timestamps % provided_timestamps_to_use.size()
             % boost::accumulate(m_frames, std::string{}, [](auto const &str, auto const &frame) {
                 return str + (boost::format("    pos %1% size %2% key? %3%\n") % frame.m_position % frame.m_data->get_size() % frame.m_keyframe).str();
               })
             % boost::accumulate(m_provided_timestamps, std::string{}, [](auto const &str, auto const &provided_timestamp) {
                 return str + (boost::format("    pos %1% timestamp %2%\n") % provided_timestamp.second % format_timestamp(provided_timestamp.first)).str();
               })
             % boost::accumulate(provided_timestamps_to_use, std::string{}, [](auto const &str, auto const &provided_timestamp) {
                 return str + (boost::format("    timestamp %1%\n") % format_timestamp(provided_timestamp)).str();
               }));

  m_provided_timestamps.erase(m_provided_timestamps.begin(), m_provided_timestamps.begin() + provided_timestamps_idx);

  brng::sort(provided_timestamps_to_use);

  return provided_timestamps_to_use;
}

void
es_parser_c::calculate_frame_timestamps() {
  auto provided_timestamps_to_use = calculate_provided_timestamps_to_use();

  if (!m_simple_picture_order)
    brng::sort(m_frames, [](const frame_t &f1, const frame_t &f2) { return f1.m_presentation_order < f2.m_presentation_order; });

  auto frames_begin           = m_frames.begin();
  auto frames_end             = m_frames.end();
  auto previous_frame_itr     = frames_begin;
  auto provided_timestamp_itr = provided_timestamps_to_use.begin();

  for (auto frame_itr = frames_begin; frames_end != frame_itr; ++frame_itr) {
    if (frame_itr->m_has_provided_timestamp) {
      frame_itr->m_start = *provided_timestamp_itr;
      ++provided_timestamp_itr;

      if (frames_begin != frame_itr)
        previous_frame_itr->m_end = frame_itr->m_start;

    } else {
      frame_itr->m_start = frames_begin == frame_itr ? m_max_timestamp : previous_frame_itr->m_end;
      ++m_stats.num_timestamps_generated;
    }

    frame_itr->m_end = frame_itr->m_start + duration_for(frame_itr->m_si);

    previous_frame_itr = frame_itr;
  }

  m_max_timestamp = m_frames.back().m_end;

  mxdebug_if(m_debug_timestamps, boost::format("CLEANUP frames <pres_ord dec_ord has_prov_ts tc dur>: %1%\n")
             % boost::accumulate(m_frames, std::string(""), [](std::string const &accu, frame_t const &frame) {
                 return accu + (boost::format(" <%1% %2% %3% %4% %5%>") % frame.m_presentation_order % frame.m_decode_order % frame.m_has_provided_timestamp % frame.m_start % (frame.m_end - frame.m_start)).str();
               }));

  if (!m_simple_picture_order)
    brng::sort(m_frames, [](const frame_t &f1, const frame_t &f2) { return f1.m_decode_order < f2.m_decode_order; });
}

void
es_parser_c::calculate_frame_references_and_update_stats() {
  auto frames_begin       = m_frames.begin();
  auto frames_end         = m_frames.end();
  auto previous_frame_itr = frames_begin;

  for (auto frame_itr = frames_begin; frames_end != frame_itr; ++frame_itr) {
    if (frames_begin != frame_itr)
      frame_itr->m_ref1 = previous_frame_itr->m_start - frame_itr->m_start;

    previous_frame_itr = frame_itr;
    m_duration_frequency[frame_itr->m_end - frame_itr->m_start]++;

    ++m_stats.num_field_slices;
  }
}

void
es_parser_c::cleanup() {
  if (m_frames.empty())
    return;

  if (m_discard_actual_frames) {
    m_stats.num_frames_discarded    += m_frames.size();
    m_stats.num_timestamps_discarded += m_provided_timestamps.size();

    m_frames.clear();
    m_provided_timestamps.clear();

    return;
  }

  calculate_frame_order();
  calculate_frame_timestamps();
  calculate_frame_references_and_update_stats();

  if (m_first_cleanup && !m_frames.front().m_keyframe) {
    // Drop all frames before the first key frames as they cannot be
    // decoded anyway.
    m_stats.num_frames_discarded += m_frames.size();
    m_frames.clear();

    return;
  }

  m_first_cleanup = false;

  m_stats.num_frames_out += m_frames.size();
  m_frames_out.insert(m_frames_out.end(), m_frames.begin(), m_frames.end());
  m_frames.clear();
}

memory_cptr
es_parser_c::create_nalu_with_size(const memory_cptr &src,
                                   bool add_extra_data) {
  auto nalu = mtx::mpeg::create_nalu_with_size(src, m_nalu_size_length, add_extra_data ? m_extra_data : std::vector<memory_cptr>{} );

  if (add_extra_data)
    m_extra_data.clear();

  return nalu;
}

memory_cptr
es_parser_c::get_hevcc()
  const {
  return hevcc_c{static_cast<unsigned int>(m_nalu_size_length), m_vps_list, m_sps_list, m_pps_list, m_user_data, m_codec_private}.pack();
}

bool
es_parser_c::has_par_been_found()
  const {
  assert(m_hevcc_ready);
  return m_par_found;
}

int64_rational_c const &
es_parser_c::get_par()
  const {
  assert(m_hevcc_ready && m_par_found);
  return m_par;
}

std::pair<int64_t, int64_t> const
es_parser_c::get_display_dimensions(int width,
                                    int height)
  const {
  assert(m_hevcc_ready && m_par_found);

  if (0 >= width)
    width = get_width();
  if (0 >= height)
    height = get_height();

  return std::make_pair<int64_t, int64_t>(1 <= m_par ? std::llround(width * boost::rational_cast<double>(m_par)) : width,
                                          1 <= m_par ? height                                                    : std::llround(height / boost::rational_cast<double>(m_par)));
}

size_t
es_parser_c::get_num_field_slices()
  const {
  return m_stats.num_field_slices;
}

size_t
es_parser_c::get_num_frame_slices()
  const {
  return m_stats.num_frame_slices;
}

void
es_parser_c::dump_info()
  const {
  mxinfo("Dumping m_frames_out:\n");
  for (auto &frame : m_frames_out) {
    mxinfo(boost::format("size %1% key %2% start %3% end %4% ref1 %5% adler32 0x%|6$08x|\n")
           % frame.m_data->get_size()
           % frame.m_keyframe
           % format_timestamp(frame.m_start)
           % format_timestamp(frame.m_end)
           % format_timestamp(frame.m_ref1)
           % mtx::checksum::calculate_as_uint(mtx::checksum::algorithm_e::adler32, *frame.m_data));
  }
}

std::string
es_parser_c::get_nalu_type_name(int type) {
  init_nalu_names();
  auto name = ms_nalu_names_by_type.find(type);
  return name == ms_nalu_names_by_type.end() ? "unknown" : name->second;
}

void
es_parser_c::init_nalu_names() {
  if (!ms_nalu_names_by_type.empty())
    return;

  ms_nalu_names_by_type = std::unordered_map<int, std::string>{
    { HEVC_NALU_TYPE_TRAIL_N,       "trail_n"       },
    { HEVC_NALU_TYPE_TRAIL_R,       "trail_r"       },
    { HEVC_NALU_TYPE_TSA_N,         "tsa_n"         },
    { HEVC_NALU_TYPE_TSA_R,         "tsa_r"         },
    { HEVC_NALU_TYPE_STSA_N,        "stsa_n"        },
    { HEVC_NALU_TYPE_STSA_R,        "stsa_r"        },
    { HEVC_NALU_TYPE_RADL_N,        "radl_n"        },
    { HEVC_NALU_TYPE_RADL_R,        "radl_r"        },
    { HEVC_NALU_TYPE_RASL_N,        "rasl_n"        },
    { HEVC_NALU_TYPE_RASL_R,        "rasl_r"        },
    { HEVC_NALU_TYPE_RSV_VCL_N10,   "rsv_vcl_n10"   },
    { HEVC_NALU_TYPE_RSV_VCL_N12,   "rsv_vcl_n12"   },
    { HEVC_NALU_TYPE_RSV_VCL_N14,   "rsv_vcl_n14"   },
    { HEVC_NALU_TYPE_RSV_VCL_R11,   "rsv_vcl_r11"   },
    { HEVC_NALU_TYPE_RSV_VCL_R13,   "rsv_vcl_r13"   },
    { HEVC_NALU_TYPE_RSV_VCL_R15,   "rsv_vcl_r15"   },
    { HEVC_NALU_TYPE_BLA_W_LP,      "bla_w_lp"      },
    { HEVC_NALU_TYPE_BLA_W_RADL,    "bla_w_radl"    },
    { HEVC_NALU_TYPE_BLA_N_LP,      "bla_n_lp"      },
    { HEVC_NALU_TYPE_IDR_W_RADL,    "idr_w_radl"    },
    { HEVC_NALU_TYPE_IDR_N_LP,      "idr_n_lp"      },
    { HEVC_NALU_TYPE_CRA_NUT,       "cra_nut"       },
    { HEVC_NALU_TYPE_RSV_RAP_VCL22, "rsv_rap_vcl22" },
    { HEVC_NALU_TYPE_RSV_RAP_VCL23, "rsv_rap_vcl23" },
    { HEVC_NALU_TYPE_RSV_VCL24,     "rsv_vcl24"     },
    { HEVC_NALU_TYPE_RSV_VCL25,     "rsv_vcl25"     },
    { HEVC_NALU_TYPE_RSV_VCL26,     "rsv_vcl26"     },
    { HEVC_NALU_TYPE_RSV_VCL27,     "rsv_vcl27"     },
    { HEVC_NALU_TYPE_RSV_VCL28,     "rsv_vcl28"     },
    { HEVC_NALU_TYPE_RSV_VCL29,     "rsv_vcl29"     },
    { HEVC_NALU_TYPE_RSV_VCL30,     "rsv_vcl30"     },
    { HEVC_NALU_TYPE_RSV_VCL31,     "rsv_vcl31"     },
    { HEVC_NALU_TYPE_VIDEO_PARAM,   "video_param"   },
    { HEVC_NALU_TYPE_SEQ_PARAM,     "seq_param"     },
    { HEVC_NALU_TYPE_PIC_PARAM,     "pic_param"     },
    { HEVC_NALU_TYPE_ACCESS_UNIT,   "access_unit"   },
    { HEVC_NALU_TYPE_END_OF_SEQ,    "end_of_seq"    },
    { HEVC_NALU_TYPE_END_OF_STREAM, "end_of_stream" },
    { HEVC_NALU_TYPE_FILLER_DATA,   "filler_data"   },
    { HEVC_NALU_TYPE_PREFIX_SEI,    "prefix_sei"    },
    { HEVC_NALU_TYPE_SUFFIX_SEI,    "suffix_sei"    },
    { HEVC_NALU_TYPE_RSV_NVCL41,    "rsv_nvcl41"    },
    { HEVC_NALU_TYPE_RSV_NVCL42,    "rsv_nvcl42"    },
    { HEVC_NALU_TYPE_RSV_NVCL43,    "rsv_nvcl43"    },
    { HEVC_NALU_TYPE_RSV_NVCL44,    "rsv_nvcl44"    },
    { HEVC_NALU_TYPE_RSV_NVCL45,    "rsv_nvcl45"    },
    { HEVC_NALU_TYPE_RSV_NVCL46,    "rsv_nvcl46"    },
    { HEVC_NALU_TYPE_RSV_NVCL47,    "rsv_nvcl47"    },
    { HEVC_NALU_TYPE_UNSPEC48,      "unspec48"      },
    { HEVC_NALU_TYPE_UNSPEC49,      "unspec49"      },
    { HEVC_NALU_TYPE_UNSPEC50,      "unspec50"      },
    { HEVC_NALU_TYPE_UNSPEC51,      "unspec51"      },
    { HEVC_NALU_TYPE_UNSPEC52,      "unspec52"      },
    { HEVC_NALU_TYPE_UNSPEC53,      "unspec53"      },
    { HEVC_NALU_TYPE_UNSPEC54,      "unspec54"      },
    { HEVC_NALU_TYPE_UNSPEC55,      "unspec55"      },
    { HEVC_NALU_TYPE_UNSPEC56,      "unspec56"      },
    { HEVC_NALU_TYPE_UNSPEC57,      "unspec57"      },
    { HEVC_NALU_TYPE_UNSPEC58,      "unspec58"      },
    { HEVC_NALU_TYPE_UNSPEC59,      "unspec59"      },
    { HEVC_NALU_TYPE_UNSPEC60,      "unspec60"      },
    { HEVC_NALU_TYPE_UNSPEC61,      "unspec61"      },
    { HEVC_NALU_TYPE_UNSPEC62,      "unspec62"      },
    { HEVC_NALU_TYPE_UNSPEC63,      "unspec63"      },
  };
}

}}                              // namespace mtx::hevc
