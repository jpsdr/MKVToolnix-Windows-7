/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   AV1 parser code

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include "common/at_scope_exit.h"
#include "common/av1.h"
#include "common/bit_reader.h"
#include "common/bit_writer.h"
#include "common/byte_buffer.h"
#include "common/debugging.h"
#include "common/endian.h"
#include "common/list_utils.h"
#include "common/strings/formatting.h"

namespace mtx::av1 {

class parser_private_c {
  friend class parser_c;

  mtx::bits::reader_c r;

  bool frame_found{}, reduced_still_picture_header{}, parse_sequence_header_obus_only{};

  unsigned int obu_type{};
  bool obu_extension_flag{}, obu_has_size_field{}, seen_frame_header{}, frame_id_numbers_present_flag{}, equal_picture_interval{}, seq_tier_0{};
  bool current_frame_contains_sequence_header{};
  std::optional<unsigned int> temporal_id, spatial_id;
  unsigned int operating_point_idc{}, seq_profile{}, seq_level_idx_0{}, max_frame_width{}, max_frame_height{}, buffer_delay_length{1};

  color_config_t color_config{};

  mtx::bytes::buffer_c buffer;
  frame_t current_frame;
  std::deque<frame_t> frames;
  uint64_t frame_number{}, num_units_in_display_tick{}, time_scale{}, num_ticks_per_picture{}, input_pos{};
  mtx_mp_rational_t forced_default_duration, bitstream_default_duration = mtx::rational(1'000'000'000ull, 25);

  memory_cptr sequence_header_obu;
  std::vector<memory_cptr> metadata_obus;

  debugging_option_c debug_is_keyframe{"av1|av1_is_keyframe"}, debug_obu_types{"av1|av1_obu_types"}, debug_timing_info{"av1|av1_timing_info"}, debug_parser{"av1|av1_parser"};
};

parser_c::parser_c()
  : p{std::make_unique<parser_private_c>()}
{
}

parser_c::~parser_c() = default;

char const *
parser_c::get_obu_type_name(unsigned int obu_type) {
  static std::unordered_map<unsigned int, char const *> s_type_names;

  if (s_type_names.empty()) {
    s_type_names[OBU_SEQUENCE_HEADER]        = "sequence_header";
    s_type_names[OBU_TEMPORAL_DELIMITER]     = "temporal_delimiter";
    s_type_names[OBU_FRAME_HEADER]           = "frame_header";
    s_type_names[OBU_TILE_GROUP]             = "tile_group";
    s_type_names[OBU_METADATA]               = "metadata";
    s_type_names[OBU_FRAME]                  = "frame";
    s_type_names[OBU_REDUNDANT_FRAME_HEADER] = "redundant_frame_header";
    s_type_names[OBU_TILE_LIST]              = "tile_list";
    s_type_names[OBU_PADDING]                = "padding";
  }

  auto itr = s_type_names.find(obu_type);
  return itr != s_type_names.end() ? itr->second : "unknown";
}

void
parser_c::set_default_duration(mtx_mp_rational_t default_duration) {
  p->forced_default_duration = default_duration;
}

void
parser_c::set_parse_sequence_header_obus_only(bool parse_sequence_header_obus_only) {
  p->parse_sequence_header_obus_only = parse_sequence_header_obus_only;
}

uint64_t
parser_c::get_next_timestamp() {
  auto duration = p->forced_default_duration ? p->forced_default_duration : p->bitstream_default_duration;

  return mtx::to_int(duration * p->frame_number++);
}

uint64_t
parser_c::read_leb128(mtx::bits::reader_c &r) {
  return r.get_leb128();
}

uint64_t
parser_c::read_uvlc(mtx::bits::reader_c &r) {
  auto leading_zeros = 0u;

  while (!r.get_bit())
    ++leading_zeros;

  if (leading_zeros >= 32)
    return (1llu << 32) - 1;

  auto value = r.get_bits(leading_zeros);
  return value + (1llu << leading_zeros) - 1;
}

std::optional<uint64_t>
parser_c::parse_obu_common_data(memory_c const &buffer) {
  return parse_obu_common_data(buffer.get_buffer(), buffer.get_size());
}

std::optional<uint64_t>
parser_c::parse_obu_common_data(uint8_t const *buffer,
                                uint64_t buffer_size) {
  p->r.init(buffer, buffer_size);
  return parse_obu_common_data();
}

std::optional<uint64_t>
parser_c::parse_obu_common_data() {
  auto &r = p->r;

  // obu_header
  if (r.get_bit())              // obu_forbidden_bit
    throw obu_invalid_structure_x{};

  p->obu_type           = r.get_bits(4);
  p->obu_extension_flag = r.get_bit();
  p->obu_has_size_field = r.get_bit();
  r.skip_bits(1);               // obu_reserved_bit

  if (p->obu_extension_flag) {
    // obu_extension_header
    p->temporal_id = r.get_bits(3);
    p->spatial_id  = r.get_bits(2);
    r.skip_bits(3);             // extension_header_reserved_3bits

  } else {
    p->temporal_id = std::nullopt;
    p->spatial_id  = std::nullopt;
  }

  if (p->obu_has_size_field)
    return read_leb128(r);

  return {};
}

void
parser_c::parse_color_config(mtx::bits::reader_c &r) {
  auto &cc = p->color_config;

  cc.high_bitdepth = r.get_bit();
  auto bit_depth   = 0u;

  if ((p->seq_profile == 2) && cc.high_bitdepth) {
    cc.twelve_bit = r.get_bit();
    bit_depth     = cc.twelve_bit ? 12 : 10;
  } else
    bit_depth     = cc.high_bitdepth ? 10 :  8;

  cc.mono_chrome = (p->seq_profile != 1) ? r.get_bit() : false;

  if (r.get_bit()) {            // color_description_present_flag
    cc.color_primaries          = r.get_bits(8);
    cc.transfer_characteristics = r.get_bits(8);
    cc.matrix_coefficients      = r.get_bits(8);
  } else {
    cc.color_primaries          = CP_UNSPECIFIED;
    cc.transfer_characteristics = TC_UNSPECIFIED;
    cc.matrix_coefficients      = MC_UNSPECIFIED;
  }

  if (cc.mono_chrome) {
    cc.video_full_range_flag = r.get_bit();
    return;

  } else if (   (cc.color_primaries          == CP_BT_709)
             && (cc.transfer_characteristics == TC_SRGB)
             && (cc.matrix_coefficients      == MC_IDENTITY)) {
  } else {
    cc.video_full_range_flag = r.get_bit();

    cc.chroma_subsampling_x  = false;
    cc.chroma_subsampling_y  = false;

    if (p->seq_profile == 0) {
      cc.chroma_subsampling_x = true;
      cc.chroma_subsampling_y = true;

    } else if (p->seq_profile > 1) {
      if (bit_depth == 12) {
        cc.chroma_subsampling_x = r.get_bit();
        if (cc.chroma_subsampling_x)
          cc.chroma_subsampling_y = r.get_bit();
      } else
        cc.chroma_subsampling_x = true;
    }

    if (cc.chroma_subsampling_x && cc.chroma_subsampling_y)
      cc.chroma_sample_position = r.get_bits(2);
  }

  r.skip_bits(1);               // separate_uv_delta_q
}

void
parser_c::parse_timing_info(mtx::bits::reader_c &r) {
  p->num_units_in_display_tick = r.get_bits(32);
  p->time_scale                = r.get_bits(32);
  p->equal_picture_interval    = r.get_bit();

  if (p->equal_picture_interval)
    p->num_ticks_per_picture = read_uvlc(r) + 1;
  else
    p->num_ticks_per_picture = 1;

  if (p->num_units_in_display_tick && p->time_scale && p->num_ticks_per_picture)
    p->bitstream_default_duration = mtx::rational(1'000'000'000ull * p->num_units_in_display_tick * p->num_ticks_per_picture, p->time_scale);

  mxdebug_if(p->debug_timing_info,
             fmt::format("parse_timing_info: num_units_in_display_tick {0} time_scale {1} equal_picture_interval {2} num_ticks_per_picture {3} bitstream_default_duration {4}\n",
                         p->num_units_in_display_tick,
                         p->time_scale,
                         p->equal_picture_interval,
                         p->num_ticks_per_picture,
                         mtx::string::format_timestamp(p->bitstream_default_duration)));
}

void
parser_c::parse_decoder_model_info(mtx::bits::reader_c &r) {
  p->buffer_delay_length = r.get_bits(5) + 1;
  r.skip_bits(  32              // num_units_in_decoding_tick
              +  5              // buffer_removal_time_length_minus_1
              +  5);            // frame_presentation_time_length_minus_1
}

void
parser_c::parse_operating_parameters_info(mtx::bits::reader_c &r) {
  r.skip_bits(2 * p->buffer_delay_length); // decoder_buffer_delay, encoder_buffer_delay
  r.skip_bits(1);                          // low_delay_mode_flag
}

void
parser_c::parse_sequence_header_obu(mtx::bits::reader_c &r) {
  p->seq_profile                  = r.get_bits(3);
  p->reduced_still_picture_header = r.skip_get_bits(1, 1); // still_picture

  if (p->reduced_still_picture_header)
    p->seq_level_idx_0 = r.get_bits(5);

  else {
    auto timing_info_present_flag        = r.get_bit();
    auto decoder_model_info_present_flag = false;

    if (timing_info_present_flag) {
      parse_timing_info(r);

      decoder_model_info_present_flag = r.get_bit();
      if (decoder_model_info_present_flag)
        parse_decoder_model_info(r);
    } else
      mxdebug_if(p->debug_timing_info, fmt::format("parse_timing_info: no timing info in sequence header\n"));

    auto initial_display_delay_present_flag = r.get_bit();
    auto operating_points_count             = r.get_bits(5) + 1;

    for (auto idx = 0u; idx < operating_points_count; ++idx) {
      auto operating_point_idc = r.get_bits(12);
      auto seq_level_idx       = r.get_bits(5);
      auto seq_tier            = seq_level_idx > 7 ? r.get_bit() : false;

      if (!idx) {
        p->operating_point_idc = operating_point_idc;
        p->seq_level_idx_0     = seq_level_idx;
        p->seq_tier_0          = seq_tier;
      }

      if (decoder_model_info_present_flag) {
        auto decoder_model_present_for_this_op = r.get_bit();
        if (decoder_model_present_for_this_op)
          parse_operating_parameters_info(r);
      }

      if (initial_display_delay_present_flag) {
        auto initial_display_delay_present_for_this_op = r.get_bit();
        if (initial_display_delay_present_for_this_op)
          r.skip_bits(4);       // initial_display_delay_minus_1[idx]
      }
    }
  }

  auto frame_width_bits  = r.get_bits(4) + 1;
  auto frame_height_bits = r.get_bits(4) + 1;

  p->max_frame_width     = r.get_bits(frame_width_bits)  + 1;
  p->max_frame_height    = r.get_bits(frame_height_bits) + 1;

  if (!p->reduced_still_picture_header) {
    p->frame_id_numbers_present_flag = r.get_bit();
    if (p->frame_id_numbers_present_flag)
      r.skip_bits(4 + 3);       // delta_frame_id_length_minus2, additional_frame_id_length_minus1
  }

  r.skip_bits(3);               // use_12x128_superblock, enable_filter_intra, enable_intra_edge_filter

  if (!p->reduced_still_picture_header) {
    r.skip_bits(4);             // enable_interintra_compound, enable_masked_compound, enable_warped_motion, enable_dual_filter
    auto enable_order_hint = r.get_bit();
    if (enable_order_hint)
      r.skip_bits(2);           // enable_jnt_comp, enable_ref_frame_mvs

    auto seq_force_screen_content_tools = r.get_bit() ? SELECT_SCREEN_CONTENT_TOOLS : r.get_bits(1);

    if (seq_force_screen_content_tools > 0) {
      if (!r.get_bit())         // seq_choose_integer_mv
        r.skip_bits(1);         // seq_force_integer_mv
    }

    if (enable_order_hint)
      r.skip_bits(3);           // order_hint_bits_minus1
  }

  r.skip_bits(3);               // enable_superres, enable_cdef, enable_restoration

  parse_color_config(r);

  r.skip_bits(1);               // film_grain_params_present

  mxdebug_if(p->debug_parser, fmt::format("debug_parser:     remaining bits at end of sequence header parsing: {0}\n", r.get_remaining_bits()));
}

void
parser_c::parse_frame_header_obu(mtx::bits::reader_c &r) {
  if (p->reduced_still_picture_header) {
    p->current_frame.is_keyframe = true;
    mxdebug_if(p->debug_is_keyframe, "is_keyframe:   true due to reduced_still_picture_header == 1\n");

    return;
  }

  if (r.get_bit()) { // show_existing_frame
    p->current_frame.is_keyframe = false;
    mxdebug_if(p->debug_is_keyframe, "is_keyframe:   false due to show_existing_frame == 1\n");

    return;
  }

  auto frame_type              = r.get_bits(2);
  p->current_frame.is_keyframe = (frame_type == FRAME_TYPE_KEY); // || (frame_type == FRAME_TYPE_INTRA_ONLY);
  mxdebug_if(p->debug_is_keyframe, fmt::format("is_keyframe:   {0}\n", p->current_frame.is_keyframe));
}

void
parser_c::parse_metadata_obu(mtx::bits::reader_c &r) {
  auto metadata_type = r.get_leb128();

  switch (metadata_type) {
    case METADATA_TYPE_ITUT_T35:
      parse_metadata_type_itu_t_t35(r);
      break;
  }
}

void
parser_c::parse(memory_c const &buffer) {
  parse(buffer.get_buffer(), buffer.get_size());
}

void
parser_c::parse(uint8_t const *buffer,
                uint64_t buffer_size) {
  auto &r = p->r;

  p->buffer.add(buffer, buffer_size);

  r.init(p->buffer.get_buffer(), p->buffer.get_size());

  mxdebug_if(p->debug_parser, fmt::format("debug_parser: start at {0} size {1}\n", p->input_pos, buffer_size));

  auto last_good_bit_position = r.get_bit_position();

  while (r.get_remaining_bits() > 0)
    try {
      if (!parse_obu())
        break;
      last_good_bit_position = r.get_bit_position();

    } catch (mtx::mm_io::end_of_file_x const &ex) {
      mxdebug_if(p->debug_parser, fmt::format("debug_parser: end of file exception: {0}\n", ex.what()));
      r.set_bit_position(last_good_bit_position);
      break;

    } catch (mtx::exception const &ex) {
      mxdebug_if(p->debug_parser, fmt::format("debug_parser: exception: {0}\n", ex.what()));
      break;
    }

  p->buffer.remove((r.get_bit_position() + 7) / 8);
}

bool
parser_c::parse_obu() {
  auto &r                 = p->r;
  auto start_bit_position = r.get_bit_position();
  auto obu_size           = parse_obu_common_data();
  auto keep_obu           = !p->parse_sequence_header_obus_only;

  if (!obu_size)
    throw obu_without_size_unsupported_x{};

  mxdebug_if(p->debug_parser,
             fmt::format("debug_parser:   at absolute {0} relative {1} type {2} ({3}) size {4} remaining {5}\n",
                         p->input_pos,
                         start_bit_position / 8,
                         p->obu_type,
                         get_obu_type_name(p->obu_type),
                         *obu_size,
                         r.get_remaining_bits() / 8));

  if (mtx::included_in(p->obu_type, OBU_FRAME, OBU_FRAME_HEADER))
    p->frame_found = true;

  auto next_obu_bit_position = r.get_bit_position() + (*obu_size * 8);
  if ((*obu_size * 8) > static_cast<uint64_t>(r.get_remaining_bits())) {
    r.set_bit_position(start_bit_position);
    return false;
  }

  auto obu = memory_c::borrow(p->buffer.get_buffer() + (start_bit_position / 8), (next_obu_bit_position - start_bit_position) / 8);
  mtx::bits::reader_c sub_r{obu->get_buffer(), obu->get_size()};
  sub_r.set_bit_position(r.get_bit_position() - start_bit_position);

  at_scope_exit_c copy_current_and_seek_to_next_obu([this, start_bit_position, next_obu_bit_position, &obu, &keep_obu]() {
    p->r.set_bit_position(next_obu_bit_position);
    p->input_pos += (next_obu_bit_position - start_bit_position) / 8;

    if (!keep_obu)
      return;

    if (p->current_frame.mem)
      p->current_frame.mem->add(*obu);
    else
      p->current_frame.mem = obu->clone();

    if (p->obu_type == OBU_SEQUENCE_HEADER)
      p->current_frame_contains_sequence_header = true;
  });

  if (   (p->obu_type            != OBU_SEQUENCE_HEADER)
      && (p->obu_type            != OBU_TEMPORAL_DELIMITER)
      && (p->operating_point_idc != 0)
      && p->obu_extension_flag) {
    auto in_temporal_layer = !!((p->operating_point_idc >> *p->temporal_id)      & 1);
    auto in_spatial_layer  = !!((p->operating_point_idc >> (*p->spatial_id + 8)) & 1);

    if (!in_temporal_layer || !in_spatial_layer) {
      mxdebug_if(p->debug_parser, fmt::format("debug_parser:     not keeping due to layer; in_temporal_layer {0} in_spatial_layer {1}\n", in_temporal_layer, in_spatial_layer));
      keep_obu = false;
      return true;
    }
  }

  if (mtx::included_in(p->obu_type, OBU_PADDING, OBU_REDUNDANT_FRAME_HEADER)) {
    mxdebug_if(p->debug_parser, fmt::format("debug_parser:     not keeping due to type {0}\n", p->obu_type));
    keep_obu = false;
    return true;
  }

  if (p->obu_type == OBU_TEMPORAL_DELIMITER) {
    mxdebug_if(p->debug_parser, fmt::format("debug_parser:     not keeping due to type being temporal delimiter\n"));
    flush();
    p->seen_frame_header = false;
    keep_obu             = false;
    return true;
  }

  if (p->obu_type == OBU_SEQUENCE_HEADER) {
    parse_sequence_header_obu(sub_r);
    p->sequence_header_obu = obu->clone();

    return true;
  }

  if (mtx::included_in(p->obu_type, OBU_FRAME, OBU_FRAME_HEADER)) {
    if (p->sequence_header_obu)
      parse_frame_header_obu(sub_r);

    return !!p->sequence_header_obu;
  }

  if (p->parse_sequence_header_obus_only)
    return true;

  if (p->obu_type == OBU_METADATA) {
    if (!p->frame_found) {
      p->metadata_obus.emplace_back(obu->clone());
      parse_metadata_obu(sub_r);
    }

    return true;
  }

  return true;
}

std::pair<unsigned int, unsigned int>
parser_c::get_pixel_dimensions()
  const {
  return { p->max_frame_width, p->max_frame_height };
}

mtx_mp_rational_t
parser_c::get_frame_duration()
  const {
  if (!p->sequence_header_obu || !p->time_scale)
    return {};

  return mtx::rational(p->num_units_in_display_tick, p->time_scale) * 1'000'000'000ll;
}

bool
parser_c::headers_parsed()
  const {
  return p->sequence_header_obu && p->frame_found;
}

bool
parser_c::frame_available()
  const {
  return !p->frames.empty();
}

frame_t
parser_c::get_next_frame() {
  auto frame = p->frames.front();
  p->frames.pop_front();

  return frame;
}

memory_cptr
parser_c::get_av1c()
  const {
  // See https://github.com/Matroska-Org/matroska-specification/blob/master/codec/av1.md#codecprivate-1

  auto size = 4 + (p->sequence_header_obu ? p->sequence_header_obu->get_size() : 0);
  for (auto const &obu : p->metadata_obus)
    size += obu->get_size();

  auto av1c      = memory_c::alloc(size);
  auto const &cc = p->color_config;

  mtx::bits::writer_c w{av1c->get_buffer(), 4};

  w.put_bits(1, 1);             // marker
  w.put_bits(7, 1);             // version

  w.put_bits(3, p->seq_profile);
  w.put_bits(5, p->seq_level_idx_0);

  w.put_bits(1, p->seq_tier_0);
  w.put_bits(1, cc.high_bitdepth);
  w.put_bits(1, cc.twelve_bit);
  w.put_bits(1, cc.mono_chrome);
  w.put_bits(1, cc.chroma_subsampling_x);
  w.put_bits(1, cc.chroma_subsampling_y);
  w.put_bits(2, cc.chroma_sample_position);

  w.put_bits(3, 0);             // reserved
  w.put_bits(1, 0);             // initial_presentation_delay_present
  w.put_bits(4, 0);             // initial_presentation_delay_minus_one

  auto dst = av1c->get_buffer() + 4;
  if (p->sequence_header_obu) {
    std::memcpy(dst, p->sequence_header_obu->get_buffer(), p->sequence_header_obu->get_size());
    dst += p->sequence_header_obu->get_size();
  }

  for (auto const &obu : p->metadata_obus) {
    std::memcpy(dst, obu->get_buffer(), obu->get_size());
    dst += obu->get_size();
  }

  return av1c;
}

color_config_t
parser_c::get_color_config()
  const {
  return p->color_config;
}

void
parser_c::flush() {
  auto &frame = p->current_frame;

  if (frame.mem) {
    frame.timestamp = get_next_timestamp();

    if (frame.is_keyframe && !p->current_frame_contains_sequence_header)
      frame.mem->prepend(p->sequence_header_obu);

    mtx::av1::maybe_shrink_size_fields(*frame.mem);

    p->frames.push_back(frame);
  }

  frame                                     = frame_t{};
  p->current_frame_contains_sequence_header = false;
}

bool
parser_c::is_keyframe(memory_c const &buffer) {
  return is_keyframe(buffer.get_buffer(), buffer.get_size());
}

bool
parser_c::is_keyframe(uint8_t const *buffer,
                      uint64_t buffer_size) {
  mxdebug_if(p->debug_is_keyframe, fmt::format("is_keyframe: start on size {0}\n", buffer_size));

  p->r.init(buffer, buffer_size);

  try {
    while (true) {
      auto position = p->r.get_bit_position() / 8;
      auto obu_size = parse_obu_common_data();

      mxdebug_if(p->debug_is_keyframe,
                 fmt::format("is_keyframe:   at {0} type {1} ({2}) size {3}\n",
                             position,
                             p->obu_type,
                             get_obu_type_name(p->obu_type),
                             obu_size ? static_cast<int>(*obu_size) : -1));

      if (!mtx::included_in(p->obu_type, OBU_FRAME, OBU_FRAME_HEADER)) {
        if (!obu_size) {
          mxdebug_if(p->debug_is_keyframe, "is_keyframe:   false due to OBU with unknown size\n");
          return false;
        }

        p->r.skip_bits(*obu_size * 8);

        continue;
      }

      if (p->reduced_still_picture_header) {
        mxdebug_if(p->debug_is_keyframe, "is_keyframe:   true due to reduced_still_picture_header == 1\n");
        return true;

      } else if (p->r.get_bit()) {     // show_existing_frame
        mxdebug_if(p->debug_is_keyframe, "is_keyframe:   false due to show_existing_frame == 1\n");
        return false;
      }

      auto frame_type = p->r.get_bits(2);
      auto result     = (frame_type == FRAME_TYPE_KEY); // || (frame_type == FRAME_TYPE_INTRA_ONLY);

      mxdebug_if(p->debug_is_keyframe,
                 fmt::format("is_keyframe:   {0} due to frame_type == {1} ({2})\n",
                             result ? "true" : "false",
                             frame_type,
                               frame_type == FRAME_TYPE_KEY        ? "key"
                             : frame_type == FRAME_TYPE_INTER      ? "inter"
                             : frame_type == FRAME_TYPE_INTRA_ONLY ? "intra-only"
                             :                                       "switch"));

      return result;
    }

  } catch (mtx::mm_io::exception &) {
    mxdebug_if(p->debug_is_keyframe, "is_keyframe:   false due to I/O exception\n");
  }

  return false;
}

void
parser_c::debug_obu_types(memory_c const &buffer) {
  debug_obu_types(buffer.get_buffer(), buffer.get_size());
}

void
parser_c::debug_obu_types(uint8_t const *buffer,
                          uint64_t buffer_size) {
  if (!p->debug_obu_types)
    return;

  mxdebug_if(p->debug_obu_types, fmt::format("debug_obu_types: start on size {0}\n", buffer_size));

  p->r.init(buffer, buffer_size);

  try {
    while (p->r.get_remaining_bits() > 0) {
      auto position = p->r.get_bit_position() / 8;
      auto obu_size = parse_obu_common_data();

      mxdebug_if(p->debug_obu_types,
                 fmt::format("debug_obu_types:   at {0} type {1} ({2}) size {3}\n",
                             position,
                             p->obu_type,
                             get_obu_type_name(p->obu_type),
                             obu_size ? static_cast<int>(*obu_size) : -1));

      if (!obu_size)
        return;

      p->r.skip_bits(*obu_size * 8);
    }

  } catch (mtx::mm_io::exception &) {
    mxdebug_if(p->debug_obu_types, "debug_obu_types:   false due to I/O exception\n");
  }
}

void
maybe_shrink_size_fields(memory_c &mem) {
  static debugging_option_c s_debug{"av1_shrink_size_field"};

  if (mem.get_size() < 2)
    return;

  mxdebug_if(s_debug, fmt::format("start\n"));

  auto buffer      = mem.get_buffer();
  auto buffer_size = mem.get_size();
  auto idx_src     = 0u;
  auto idx_dest    = 0u;

  while ((idx_src + 2) < buffer_size) {
    try {
      mxdebug_if(s_debug, fmt::format("  idx_src {0} idx_dest {1} buffer_size {2}\n", idx_src, idx_dest, buffer_size));

      mtx::bits::reader_c r{&buffer[idx_src], buffer_size - idx_src};
      // See 5.3.2 OBU header syntax
      r.skip_bits(1 + 4 + 1);

      if (!r.get_bit()) {
        mxdebug_if(s_debug, fmt::format("  ouff no size field\n"));
        // copy rest
        std::memmove(&buffer[idx_dest], &buffer[idx_src], buffer_size - idx_src);
        idx_dest += buffer_size - idx_src;

        break;
      }

      r.skip_bit();

      auto obu_size   = r.get_leb128();
      auto field_size = r.get_bit_position() / 8 - 1u;

      uint8_t new_size_field[9];
      mtx::bits::writer_c w{new_size_field, 9};

      w.put_leb128(obu_size);

      auto new_field_size = w.get_bit_position() / 8;

      mxdebug_if(s_debug, fmt::format("  obu_size {0} field_size {1} new_field_size {2}\n", obu_size, field_size, new_field_size));

      if ((1 + idx_src + field_size + obu_size) > buffer_size)
        return;

      auto full_obu_size = 1 + field_size + obu_size;

      if (field_size <= new_field_size) {
        if (idx_src != idx_dest)
          std::memmove(&buffer[idx_dest], &buffer[idx_src], full_obu_size);

        idx_src  += full_obu_size;
        idx_dest += full_obu_size;

        continue;
      }

      buffer[idx_dest] = buffer[idx_src];
      std::memcpy(&buffer[idx_dest + 1], new_size_field, new_field_size);
      std::memmove(&buffer[idx_dest + 1 + new_field_size], &buffer[idx_src + 1 + field_size], obu_size);

      idx_src  += 1 +     field_size + obu_size;
      idx_dest += 1 + new_field_size + obu_size;

    } catch (mtx::mm_io::end_of_file_x &) {
    }
  }

  if (idx_src == idx_dest)
    return;

  mem.resize(idx_dest);
}

void
parser_c::parse_metadata_type_itu_t_t35(mtx::bits::reader_c &r) {
  auto itu_t_t35_country_code = r.get_bits(8);

  if (itu_t_t35_country_code == 0xFF) {
    r.skip_bits(8); // itu_t_t35_country_code_extension_byte
  }

  auto payload_bytes_size      = r.get_remaining_bits() / 8;
  auto itu_t_t35_payload_bytes = memory_c::alloc(payload_bytes_size);
  auto buffer                  = itu_t_t35_payload_bytes->get_buffer();
  r.get_bytes(buffer, payload_bytes_size);

  // Check for Dolby Vision header
  auto dovi_rpu_header_size = sizeof(ITU_T_T35_DOVI_RPU_PAYLOAD_HEADER);

  if (   (itu_t_t35_payload_bytes->get_size() <= dovi_rpu_header_size)
      || std::memcmp(buffer, &ITU_T_T35_DOVI_RPU_PAYLOAD_HEADER, dovi_rpu_header_size))
    return;

  itu_t_t35_payload_bytes->set_offset(dovi_rpu_header_size);
  handle_itu_t_t35_dovi_rpu_payload(itu_t_t35_payload_bytes);
}

void
parser_c::handle_itu_t_t35_dovi_rpu_payload(const memory_cptr payload_mem) {
  auto buffer      = payload_mem->get_buffer();
  auto buffer_size = payload_mem->get_size();

  size_t rpu_size{};

  // Convert into regular Dolby Vision RPU
  if (buffer[1] & 0x10) {
    if (buffer[2] & 0x08) {
      // RPU size > 512 bytes
      return;
    }

    rpu_size  = 0x100;
    rpu_size |= (buffer[1] & 0x0F) << 4;
    rpu_size |= (buffer[2] >> 4) & 0x0F;

    if (rpu_size + 2 >= buffer_size) {
      return;
    }

    for (size_t i = 0; i < rpu_size; i++) {
      auto converted_byte = (buffer[i + 2] & 0x07) << 5;
      converted_byte     |= (buffer[i + 3] >> 3) & 0x1F;

      buffer[i + 1] = converted_byte;
    }
  } else {
    rpu_size  = (buffer[0] & 0x1F) << 3;
    rpu_size |= (buffer[1] >> 5) & 0x07;

    if (rpu_size + 1 >= buffer_size) {
      return;
    }

    for (size_t i = 0; i < rpu_size; i++) {
      auto converted_byte = (buffer[i + 1] & 0x0F) << 4;
      converted_byte |= (buffer[i + 2] >> 4) & 0x0F;

      buffer[i + 1] = converted_byte;
    }
  }

  // Set prefix
  buffer[0] = 0x19;
  rpu_size += 1;

  mtx::bits::reader_c r(buffer, rpu_size);
  mtx::dovi::parse_dovi_rpu(r, m_dovi_rpu_data_header);
}

}
