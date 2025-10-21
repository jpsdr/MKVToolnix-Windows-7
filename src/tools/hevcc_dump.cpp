/*
   hevcc_dump - A tool for dumping the HEVCC element from the middle of a file

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/bit_reader.h"
#include "common/command_line.h"
#include "common/hevc/util.h"
#include "common/mm_io_x.h"
#include "common/mm_file_io.h"
#include "common/mpeg.h"
#include "common/strings/parsing.h"
#include "common/version.h"

static void
v(std::string::size_type indent,
  std::string const &label) {
  mxinfo(fmt::format("{0}{1}\n", std::string(indent, ' '), label));
}

static void
v(std::string::size_type indent,
  std::string const &label,
  std::string const &value) {
  auto width = 60 - indent;
  mxinfo(fmt::format("{0}{1:<{2}} {3}\n", std::string(indent, ' '), label + ':', width, value));
}

static void
v(std::string::size_type indent,
  std::string const &label,
  uint64_t value) {
  v(indent, label, fmt::format("{0}", value));
}

static void
v(std::string::size_type indent,
  std::size_t sub_idx,
  std::string const &label,
  std::string const &value) {
  v(indent, fmt::format("{0}[{1}]", label, sub_idx), value);
}

static void
v(std::string::size_type indent,
  std::size_t sub_idx,
  std::string const &label,
  uint64_t value) {
  v(indent, sub_idx, label, fmt::format("{0}", value));
}

static void
v(std::string::size_type indent,
  std::size_t sub_idx1,
  std::size_t sub_idx2,
  std::string const &label,
  std::string const &value) {
  v(indent, fmt::format("{0}[{1}][{2}]", label, sub_idx1, sub_idx2), value);
}

static void
v(std::string::size_type indent,
  std::size_t sub_idx1,
  std::size_t sub_idx2,
  std::string const &label,
  uint64_t value) {
  v(indent, sub_idx1, sub_idx2, label, fmt::format("{0}", value));
}

static void
v(std::string::size_type indent,
  std::size_t sub_idx1,
  std::size_t sub_idx2,
  std::size_t sub_idx3,
  std::string const &label,
  std::string const &value) {
  v(indent, fmt::format("{0}[{1}][{2}][{3}]", label, sub_idx1, sub_idx2, sub_idx3), value);
}

static void
v(std::string::size_type indent,
  std::size_t sub_idx1,
  std::size_t sub_idx2,
  std::size_t sub_idx3,
  std::string const &label,
  uint64_t value) {
  v(indent, sub_idx1, sub_idx2, sub_idx3, label, fmt::format("{0}", value));
}

static std::string
x(std::size_t width,
  uint64_t value) {
  return fmt::format("0x{0:0{1}x}", value, width);
}

static void
setup() {
  mtx::cli::g_usage_text =
    "hevcc_dump [options] input_file_name position_in_file size\n"
    "\n"
    "General options:\n"
    "\n"
    "  -h, --help             This help text\n"
    "  -V, --version          Print version information\n";
}

static std::tuple<std::string, uint64_t, uint64_t>
parse_args(std::vector<std::string> &args) {
  std::string file_name;
  int64_t file_pos = 0;
  uint64_t size    = 0;

  for (auto const &arg: args) {
    if (file_name.empty())
      file_name = arg;

    else if (size != 0)
      mxerror("Superfluous arguments given.\n");

    else if (!file_pos) {
      if (!mtx::string::parse_number(arg, file_pos))
        mxerror("The file position is not a valid number.\n");

    } else if (!size) {
      if (!mtx::string::parse_number(arg, size))
        mxerror("The size is not a valid number.\n");
    }
  }

  if (file_name.empty())
    mxerror("No file name given\n");

  if (!file_pos)
    mxerror("No file position given\n");

  if (!size)
    mxerror("No size given\n");

  if (file_pos < 0)
    file_pos = file_pos * -1 - size;

  return std::make_tuple(file_name, static_cast<uint64_t>(file_pos), size);
}

static void
parse_profile_tier_level(std::size_t indent,
                         mtx::bits::reader_c &r,
                         bool profile_present_flag,
                         unsigned int max_num_sub_layers_minus_1) {
  v(indent, "profile tier level");
  indent += 2;

  if (profile_present_flag) {
    unsigned int general_profile_idc, general_profile_compatibility_flag;

    v(indent, "general_profile_space",              r.get_bits(2));
    v(indent, "general_tier_flag",                  r.get_bits(1));
    v(indent, "general_profile_idc",                general_profile_idc = r.get_bits(5));
    v(indent, "general_profile_compatibility_flag", x(8, general_profile_compatibility_flag = r.get_bits(32)));
    v(indent, "general_progressive_source_flag",    r.get_bits(1));
    v(indent, "general_interlaced_source_flag",     r.get_bits(1));
    v(indent, "general_non_packed_constraint_flag", r.get_bits(1));
    v(indent, "general_frame_only_constraint_flag", r.get_bits(1));

    // 3   2    2   1    1   1
    // 1   7    3   9    5   1    7   3
    // 00000000 00000000 00000000 00000000
    // flags 4...10 == bits 27...21
    // bits 27...24 == 0x0f
    // bits 23...21 == 0xe0
    if (   ((general_profile_idc >= 4) && (general_profile_idc <= 10))
        || (general_profile_compatibility_flag & 0x0fe00000) != 0) {
      v(indent, "general_max_12bit_constraint_flag",        r.get_bits(1));
      v(indent, "general_max_10bit_constraint_flag",        r.get_bits(1));
      v(indent, "general_max_8bit_constraint_flag",         r.get_bits(1));
      v(indent, "general_max_422chroma_constraint_flag",    r.get_bits(1));
      v(indent, "general_max_420chroma_constraint_flag",    r.get_bits(1));
      v(indent, "general_max_monochrome_constraint_flag",   r.get_bits(1));
      v(indent, "general_intra_constraint_flag",            r.get_bits(1));
      v(indent, "general_one_picture_only_constraint_flag", r.get_bits(1));
      v(indent, "general_lower_bit_rate_constraint_flag",   r.get_bits(1));

      // flags 5, 9, 10 == bits 26, 22, 21
      // bit 26 == 0x04
      // bits 22, 21 == 0x60
      if (   (general_profile_idc == 5)
          || (general_profile_idc == 9)
          || (general_profile_idc == 10)
          || (general_profile_compatibility_flag & 0x04600000) != 0) {
        v(indent, "general_max_14bit_constraint_flag", r.get_bits(1));
        v(indent, "general_reserved_zero_33bits",      x(9, r.get_bits(33)));
      } else
        v(indent, "general_reserved_zero_34bits", x(9, r.get_bits(34)));
    } else
      v(indent, "general_reserved_zero_43bits", x(11, r.get_bits(43)));

    // flags 1, 2, 3, 4, 5, 9 == bits 30..26, 22
    // bits 30..26 == 0x7c
    // bit 22      == 0x40
    if (   ((general_profile_idc >= 1) && (general_profile_idc <= 5))
        || (general_profile_idc == 9)
        || (general_profile_compatibility_flag & 0x7c400000) != 0)
      v(indent, "general_inbld_flag", r.get_bits(1));
    else
      v(indent, "general_reserved_zero_bit", r.get_bits(1));
  }

  v(indent, "general_level_idc", r.get_bits(8));

  std::vector<unsigned int> sub_layer_profile_present_flag, sub_layer_level_present_flag;
  for (auto i = 0u; i < max_num_sub_layers_minus_1; i++) {
    v(indent, i, "sub_layer_profile_present_flag", sub_layer_profile_present_flag[i] = r.get_bits(1));
    v(indent, i, "sub_layer_level_present_flag",   sub_layer_level_present_flag[i]   = r.get_bits(1));
  }

  if (max_num_sub_layers_minus_1 > 0)
    for (auto i = max_num_sub_layers_minus_1; i < 8; ++i)
      v(indent, i, "reserved_zero_2bits", r.get_bits(2));

  for (auto i = 0u; i < max_num_sub_layers_minus_1; i++) {
    if (sub_layer_profile_present_flag[i]) {
      unsigned int sub_layer_profile_idc, sub_layer_profile_compatibility_flag;

      v(indent, i, "sub_layer_profile_space",              r.get_bits(2));
      v(indent, i, "sub_layer_tier_flag",                  r.get_bits(1));
      v(indent, i, "sub_layer_profile_idc",                (sub_layer_profile_idc = r.get_bits(5)));
      v(indent, i, "sub_layer_profile_compatibility_flag", x(8, sub_layer_profile_compatibility_flag = r.get_bits(32)));

      // Flags 4...10 == bits 27...21
      // bits 27...24 == 0x0f
      // bits 23...21 == 0xe0
      if (   ((sub_layer_profile_idc >= 4) && (sub_layer_profile_idc <= 10))
          || (sub_layer_profile_compatibility_flag & 0x0fe00000) != 0) {
        v(indent, i, "sub_layer_max_12bit_constraint_flag",        r.get_bits(1));
        v(indent, i, "sub_layer_max_10bit_constraint_flag",        r.get_bits(1));
        v(indent, i, "sub_layer_max_8bit_constraint_flag",         r.get_bits(1));
        v(indent, i, "sub_layer_max_422chroma_constraint_flag",    r.get_bits(1));
        v(indent, i, "sub_layer_max_420chroma_constraint_flag",    r.get_bits(1));
        v(indent, i, "sub_layer_max_monochrome_constraint_flag",   r.get_bits(1));
        v(indent, i, "sub_layer_intra_constraint_flag",            r.get_bits(1));
        v(indent, i, "sub_layer_one_picture_only_constraint_flag", r.get_bits(1));
        v(indent, i, "sub_layer_lower_bit_rate_constraint_flag",   r.get_bits(1));

        // flags 5 == bit 26
        // bit 26 == 0x04
        if (   (sub_layer_profile_idc == 5)
            || (sub_layer_profile_compatibility_flag & 0x04000000) != 0) {
          v(indent, i, "sub_layer_max_14bit_constraint_flag", r.get_bits(1));
          v(indent, i, "sub_layer_reserved_zero_33bits",      x(9, r.get_bits(33)));
        } else
          v(indent, i, "sub_layer_reserved_zero_34bits", x(9, r.get_bits(34)));
      } else
        v(indent, i, "sub_layer_reserved_zero_43bits", x(11, r.get_bits(43)));

      // flags 1, 2, 3, 4, 5, 9 == bits 30..26, 22
      // bits 30..26 == 0x7c
      // bit 22      == 0x40
      if (   ((sub_layer_profile_idc >= 1) && (sub_layer_profile_idc <= 5))
          || (sub_layer_profile_idc == 9)
          || (sub_layer_profile_compatibility_flag & 0x7c400000) != 0)
        v(indent, i, "sub_layer_inbld_flag", r.get_bits(1));
      else
        v(indent, i, "sub_layer_reserved_zero_bit", r.get_bits(1));
    }

    if (sub_layer_level_present_flag[i])
      v(indent, i, "sub_layer_level_idc", r.get_bits(8));
  }
}

static void
parse_scaling_list_data(std::size_t indent,
                        mtx::bits::reader_c &r) {
  v(indent, "scaling list data");
  indent += 2;

  for (auto size_id = 0u; size_id < 4u; size_id++) {
    for (auto matrix_id = 0u; matrix_id < 6u; matrix_id += (size_id == 3u) ? 3 : 1) {
      unsigned int scaling_list_pred_mode_flag;

      v(indent, size_id, matrix_id, "scaling_list_pred_mode_flag", scaling_list_pred_mode_flag = r.get_bits(1));

      if (!scaling_list_pred_mode_flag)
        v(indent, size_id, matrix_id, "scaling_list_pred_matrix_id_delta", r.get_unsigned_golomb());

      else {
        if (size_id > 1)
          v(indent, size_id - 2, matrix_id, "scaling_list_dc_coef_minus8", r.get_signed_golomb());

        auto coeff_num = std::min<unsigned int>(64u, (1 << (4 + (size_id << 1))));
        for (auto i = 0u; i < coeff_num; ++i)
          v(indent, size_id, matrix_id, i, "scaling_list_delta_coef", r.get_signed_golomb());
      }
    }
  }
}

static void
parse_short_term_reference_picture_set(std::size_t indent,
                                       mtx::bits::reader_c &r,
                                       unsigned int pic_set_idx,
                                       unsigned int num_short_term_ref_pic_sets) {
  v(indent, fmt::format("short-term reference picture set {0}", pic_set_idx));
  indent += 2;

  mxerror("parse_short_term_reference_picture_set NOT IMPLEMENTED YET!\n");

  unsigned int inter_ref_pic_set_prediction_flag = 0;

  if (pic_set_idx != 0)
    v(indent, "inter_ref_pic_set_prediction_flag", inter_ref_pic_set_prediction_flag = r.get_bits(1));

  if (inter_ref_pic_set_prediction_flag) {
    if (pic_set_idx == num_short_term_ref_pic_sets)
      v(indent, "delta_idx_minus1", r.get_unsigned_golomb());

    v(indent, "delta_rps_sign",       r.get_bits(1));
    v(indent, "abs_delta_rps_minus1", r.get_unsigned_golomb());
  }
}

static void
parse_sub_layer_hrd_parameters(std::size_t indent,
                               mtx::bits::reader_c &r,
                               unsigned int sub_layer_id,
                               unsigned int cpb_cnt,
                               bool sub_pic_hrd_params_present_flag) {
  for (unsigned int i = 0; i < cpb_cnt; ++i) {
    v(indent, fmt::format("bit_rate_value_minus1[{0}][{1}]", sub_layer_id, i), r.get_unsigned_golomb());
    v(indent, fmt::format("cpb_size_value_minus1[{0}][{1}]", sub_layer_id, i), r.get_unsigned_golomb());

    if (sub_pic_hrd_params_present_flag) {
      v(indent, fmt::format("cpb_size_du_value_minus1[{0}][{1}]", sub_layer_id, i), r.get_unsigned_golomb());
      v(indent, fmt::format("bit_rate_du_value_minus1[{0}][{1}]", sub_layer_id, i), r.get_unsigned_golomb());
    }

    v(indent, fmt::format("cbr_flag[{0}][{1}]", sub_layer_id, i), r.get_bit());
  }
}

static void
parse_hrd_parameters(std::size_t indent,
                     mtx::bits::reader_c &r,
                     bool common_inf_present_flag,
                     unsigned int sps_max_sub_layers_minus1) {
  v(indent, "HRD parameters");
  indent += 2;

  auto nal_hrd_parameters_present_flag = false;
  auto vcl_hrd_parameters_present_flag = false;
  auto sub_pic_hrd_params_present_flag = false;

  if (common_inf_present_flag) {
    nal_hrd_parameters_present_flag = r.get_bit();
    vcl_hrd_parameters_present_flag = r.get_bit();

    v(indent, "nal_hrd_parameters_present_flag", nal_hrd_parameters_present_flag);
    v(indent, "vcl_hrd_parameters_present_flag", vcl_hrd_parameters_present_flag);

    if (nal_hrd_parameters_present_flag || vcl_hrd_parameters_present_flag) {
      sub_pic_hrd_params_present_flag = r.get_bit();

      v(indent, "sub_pic_hrd_params_present_flag", sub_pic_hrd_params_present_flag);

      if (sub_pic_hrd_params_present_flag) {
        v(indent, "tick_divisor_minus2",                          r.get_bits(8));
        v(indent, "du_cpb_removal_delay_increment_length_minus1", r.get_bits(5));
        v(indent, "sub_pic_cpb_params_in_pic_timing_sei_flag",    r.get_bit());
        v(indent, "dpb_output_delay_du_length_minus1",            r.get_bits(5));
      }

      v(indent, "bit_rate_scale", r.get_bits(4));
      v(indent, "cpb_size_scale", r.get_bits(4));

      if (sub_pic_hrd_params_present_flag)
        v(indent, "cpb_size_du_scale", r.get_bits(4));

      v(indent, "initial_cpb_removal_delay_length_minus1", r.get_bits(5));
      v(indent, "au_cpb_removal_delay_length_minus1",      r.get_bits(5));
      v(indent, "dpb_output_delay_length_minus1",          r.get_bits(5));
    }
  }

  for (unsigned int i = 0; i <= sps_max_sub_layers_minus1; ++i) {
    auto fixed_pic_rate_general_flag = r.get_bit();

    v(indent, fmt::format("fixed_pic_rate_general_flag[{0}]", i), fixed_pic_rate_general_flag);

    auto fixed_pic_rate_within_cvs_flag = false;

    if (!fixed_pic_rate_general_flag) {
      fixed_pic_rate_within_cvs_flag = r.get_bit();
      v(indent, fmt::format("fixed_pic_rate_within_cvs_flag[{0}]", i), fixed_pic_rate_within_cvs_flag);
    }

    auto low_delay_hrd_flag = false;
    if (fixed_pic_rate_within_cvs_flag)
      v(indent, fmt::format("elemental_duration_in_tc_minus1[{0}]", i), r.get_unsigned_golomb());

    else {
      low_delay_hrd_flag = r.get_bit();
      v(indent, fmt::format("low_delay_hrd_flag[{0}]", i), low_delay_hrd_flag);
    }

    auto cpb_cnt = 1u;
    if (!low_delay_hrd_flag) {
      cpb_cnt = r.get_unsigned_golomb() + 1;
      v(indent, fmt::format("cpb_cnt_minus1[{0}]", i), cpb_cnt - 1);
    }

    if (nal_hrd_parameters_present_flag)
      parse_sub_layer_hrd_parameters(indent, r, i, cpb_cnt, sub_pic_hrd_params_present_flag);

    if (vcl_hrd_parameters_present_flag)
      parse_sub_layer_hrd_parameters(indent, r, i, cpb_cnt, sub_pic_hrd_params_present_flag);
  }
}

static void
parse_vui_parameters(std::size_t indent,
                     mtx::bits::reader_c &r,
                     unsigned int sps_max_sub_layers_minus1) {
  unsigned int aspect_ratio_info_present_flag, overscan_info_present_flag, video_signal_type_present_flag, chroma_loc_info_present_flag, vui_timing_info_present_flag, bitstream_restriction_flag;
  unsigned int default_display_window_flag{};

  v(indent, "VUI parameters");
  indent += 2;

  v(indent, "aspect_ratio_info_present_flag", aspect_ratio_info_present_flag = r.get_bits(1));

  if (aspect_ratio_info_present_flag) {
    unsigned int aspect_ratio_idc;

    v(indent, "aspect_ratio_idc", aspect_ratio_idc = r.get_bits(8));
    if (aspect_ratio_idc == mtx::hevc::EXTENDED_SAR) {
      v(indent, "sar_width",  r.get_bits(16));
      v(indent, "sar_height", r.get_bits(16));
    }
  }

  v(indent, "overscan_info_present_flag", overscan_info_present_flag = r.get_bits(1));
  if (overscan_info_present_flag)
    v(indent, "overscan_appropriate_flag", r.get_bits(1));

  v(indent, "video_signal_type_present_flag", video_signal_type_present_flag = r.get_bits(1));
  if (video_signal_type_present_flag) {
    unsigned int color_description_present_flag;

    v(indent, "video_format",                   r.get_bits(3));
    v(indent, "video_full_range_flag",          r.get_bits(1));
    v(indent, "color_description_present_flag", color_description_present_flag = r.get_bits(1));

    if (color_description_present_flag) {
      v(indent, "color_primaries",          r.get_bits(8));
      v(indent, "transfer_characteristics", r.get_bits(8));
      v(indent, "matrix_coeffs",            r.get_bits(8));
    }
  }

  v(indent, "chroma_loc_info_present_flag", chroma_loc_info_present_flag = r.get_bits(1));
  if (chroma_loc_info_present_flag) {
    v(indent, "chroma_sample_loc_type_top_field",    r.get_unsigned_golomb());
    v(indent, "chroma_sample_loc_type_bottom_field", r.get_unsigned_golomb());
  }

  v(indent, "neutral_chroma_indication_flag", r.get_bits(1));
  v(indent, "field_seq_flag",                 r.get_bits(1));
  v(indent, "frame_field_info_present_flag",  r.get_bits(1));

  if (   (r.get_remaining_bits() >= 68)
      && (r.peek_bits(21) == 0x100000))
    v(indent, "default_display_window_flag", "(invalid default display window)"s);
  else
    v(indent, "default_display_window_flag",    default_display_window_flag = r.get_bits(1));

  if (default_display_window_flag) {
    v(indent, "def_disp_win_left_offset",   r.get_unsigned_golomb());
    v(indent, "def_disp_win_right_offset",  r.get_unsigned_golomb());
    v(indent, "def_disp_win_top_offset",    r.get_unsigned_golomb());
    v(indent, "def_disp_win_bottom_offset", r.get_unsigned_golomb());
  }

  v(indent, "vui_timing_info_present_flag", vui_timing_info_present_flag = r.get_bits(1));
  if (vui_timing_info_present_flag) {
    unsigned int vui_poc_proportional_to_timing_flag, vui_hrd_parameters_present_flag;

    v(indent, "vui_num_units_in_tick", r.get_bits(32));
    v(indent, "vui_time_scale",        r.get_bits(32));
    v(indent, "vui_poc_proportional_to_timing_flag", vui_poc_proportional_to_timing_flag = r.get_bits(1));

    if (vui_poc_proportional_to_timing_flag)
      v(indent, "vui_num_ticks_poc_diff_one_minus1", r.get_unsigned_golomb());

    v(indent, "vui_hrd_parameters_present_flag", vui_hrd_parameters_present_flag = r.get_bits(1));
    if (vui_hrd_parameters_present_flag)
      parse_hrd_parameters(indent, r, true, sps_max_sub_layers_minus1);
  }

  v(indent, "bitstream_restriction_flag", bitstream_restriction_flag = r.get_bits(1));
  if (bitstream_restriction_flag) {
    v(indent, "tiles_fixed_structure_flag",              r.get_bits(1));
    v(indent, "motion_vectors_over_pic_boundaries_flag", r.get_bits(1));
    v(indent, "restricted_ref_pic_lists_flag",           r.get_bits(1));
    v(indent, "min_spatial_segmentation_idc",            r.get_unsigned_golomb());
    v(indent, "max_bytes_per_pic_denom",                 r.get_unsigned_golomb());
    v(indent, "max_bits_per_min_cu_denom",               r.get_unsigned_golomb());
    v(indent, "log2_max_mv_length_horizontal",           r.get_unsigned_golomb());
    v(indent, "log2_max_mv_length_vertical",             r.get_unsigned_golomb());
  }
}

static void
parse_sps_range_extension(std::size_t indent,
                          mtx::bits::reader_c &) {
  v(indent, "range extension");
  indent += 2;

  mxerror("parse_sps_range_extension NOT IMPLEMENTED YET!\n");
}

static void
parse_sps_multilayer_extension(std::size_t indent,
                          mtx::bits::reader_c &) {
  v(indent, "multilayer extension");
  indent += 2;

  mxerror("parse_sps_multilayer_extension NOT IMPLEMENTED YET!\n");
}

static void
parse_sps_3d_extension(std::size_t indent,
                          mtx::bits::reader_c &) {
  v(indent, "3d extension");
  indent += 2;

  mxerror("parse_sps_3d_extension NOT IMPLEMENTED YET!\n");
}

static void
parse_sps_scc_extension(std::size_t indent,
                          mtx::bits::reader_c &) {
  v(indent, "scc extension");
  indent += 2;

  mxerror("parse_sps_scc_extension NOT IMPLEMENTED YET!\n");
}

static void
parse_sps_extension_4bits(std::size_t indent,
                          mtx::bits::reader_c &) {
  v(indent, "extension 4bits");
  indent += 2;

  mxerror("parse_sps_extension_4bits NOT IMPLEMENTED YET!\n");
}

static void
parse_sps(mtx::bits::reader_c &r) {
  unsigned int sps_max_sub_layers_minus1, chroma_format_idc, conformance_window_flag, sps_sub_layer_ordering_info_present_flag, scaling_list_enabled_flag, pcm_enabled_flag, num_short_term_ref_pic_sets, long_term_ref_pics_present_flag;
  unsigned int vui_parameters_present_flag, sps_extension_present_flag, log2_max_pic_order_cnt_lsb_minus4;
  unsigned int sps_range_extension_flag{}, sps_multilayer_extension_flag{}, sps_3d_extension_flag{}, sps_scc_extension_flag{}, sps_extension_4bits{};

  v(4, "sequence parameter set");
  v(6, "forbidden_zero_bit",           r.get_bits(1));
  v(6, "nal_unit_type",                r.get_bits(6));
  v(6, "nuh_layer_id",                 r.get_bits(6));
  v(6, "nuh_temporal_id_plus1",        r.get_bits(3));
  v(6, "sps_video_parameter_set_id",   r.get_bits(4));
  v(6, "sps_max_sub_layers_minus1",    sps_max_sub_layers_minus1 = r.get_bits(3));
  v(6, "sps_temporal_id_nesting_flag", r.get_bits(1));

  parse_profile_tier_level(6, r, true, sps_max_sub_layers_minus1);

  v(6, "sps_seq_parameter_set_id", r.get_unsigned_golomb());
  v(6, "chroma_format_idc",        chroma_format_idc = r.get_unsigned_golomb());

  if (chroma_format_idc == 3)
    v(6, "separate_color_plane_flag", r.get_bits(1));

  v(6, "pic_width_in_luma_samples",  r.get_unsigned_golomb());
  v(6, "pic_height_in_luma_samples", r.get_unsigned_golomb());
  v(6, "conformance_window_flag",    conformance_window_flag = r.get_bits(1));

  if (conformance_window_flag) {
    v(6, "conf_win_left_offset",   r.get_unsigned_golomb());
    v(6, "conf_win_right_offset",  r.get_unsigned_golomb());
    v(6, "conf_win_top_offset",    r.get_unsigned_golomb());
    v(6, "conf_win_bottom_offset", r.get_unsigned_golomb());
  }

  v(6, "bit_depth_luma_minus8",                    r.get_unsigned_golomb());
  v(6, "bit_depth_chroma_minus8",                  r.get_unsigned_golomb());
  v(6, "log2_max_pic_order_cnt_lsb_minus4",        log2_max_pic_order_cnt_lsb_minus4 = r.get_unsigned_golomb());
  v(6, "sps_sub_layer_ordering_info_present_flag", sps_sub_layer_ordering_info_present_flag = r.get_bits(1));

  for (auto i = (sps_sub_layer_ordering_info_present_flag ? 0 : sps_max_sub_layers_minus1); i <= sps_max_sub_layers_minus1; ++i) {
    v(6, i, "sps_max_dec_pic_buffering_minus1", r.get_unsigned_golomb());
    v(6, i, "sps_max_num_reorder_pics",         r.get_unsigned_golomb());
    v(6, i, "sps_max_latency_increase_plus1",   r.get_unsigned_golomb());
  }

  v(6, "log2_min_luma_coding_block_size_minus3",      r.get_unsigned_golomb());
  v(6, "log2_diff_max_min_luma_coding_block_size",    r.get_unsigned_golomb());
  v(6, "log2_min_luma_transform_block_size_minus2",   r.get_unsigned_golomb());
  v(6, "log2_diff_max_min_luma_transform_block_size", r.get_unsigned_golomb());
  v(6, "max_transform_hierarchy_depth_inter",         r.get_unsigned_golomb());
  v(6, "max_transform_hierarchy_depth_intra",         r.get_unsigned_golomb());
  v(6, "scaling_list_enabled_flag",                   scaling_list_enabled_flag = r.get_bits(1));

  if (scaling_list_enabled_flag) {
    unsigned int sps_scaling_list_data_present_flag;
    v(6, "sps_scaling_list_data_present_flag", sps_scaling_list_data_present_flag = r.get_bits(1));

    if (sps_scaling_list_data_present_flag)
      parse_scaling_list_data(6, r);
  }

  v(6, "amp_enabled_flag",                    r.get_bits(1));
  v(6, "sample_adaptive_offset_enabled_flag", r.get_bits(1));
  v(6, "pcm_enabled_flag",                    pcm_enabled_flag = r.get_bits(1));

  if (pcm_enabled_flag) {
    v(6, "pcm_sample_bit_depth_luma_minus1",             r.get_bits(4));
    v(6, "pcm_sample_bit_depth_chroma_minus1",           r.get_bits(4));
    v(6, "log2_min_pcm_luma_coding_block_size_minus3",   r.get_unsigned_golomb());
    v(6, "log2_diff_max_min_pcm_luma_coding_block_size", r.get_unsigned_golomb());
    v(6, "pcm_loop_filter_disabled_flag",                r.get_bits(1));
  }

  v(6, "num_short_term_ref_pic_sets", num_short_term_ref_pic_sets = r.get_unsigned_golomb());
  for (auto i = 0u; i < num_short_term_ref_pic_sets; ++i)
    parse_short_term_reference_picture_set(6, r, i, num_short_term_ref_pic_sets);

  v(6, "long_term_ref_pics_present_flag", long_term_ref_pics_present_flag = r.get_bits(1));
  if (long_term_ref_pics_present_flag) {
    unsigned int num_long_term_ref_pics_sps;
    v(6, "num_long_term_ref_pics_sps", num_long_term_ref_pics_sps = r.get_unsigned_golomb());

    for (auto i = 0u; i < num_long_term_ref_pics_sps; ++i) {
      v(6, i, "lt_ref_pic_poc_lsb_sps",       r.get_bits(log2_max_pic_order_cnt_lsb_minus4 + 4));
      v(6, i, "used_by_curr_pic_lt_sps_flag", r.get_bits(1));
    }
  }

  v(6, "sps_temporal_mvp_enabled_flag",       r.get_bits(1));
  v(6, "strong_intra_smoothing_enabled_flag", r.get_bits(1));
  v(6, "vui_parameters_present_flag",         vui_parameters_present_flag = r.get_bits(1));

  if (vui_parameters_present_flag)
    parse_vui_parameters(6, r, sps_max_sub_layers_minus1);

  v(6, "sps_extension_present_flag", sps_extension_present_flag = r.get_bits(1));
  if (sps_extension_present_flag) {
    v(6, "sps_range_extension_flag",      sps_range_extension_flag      = r.get_bits(1));
    v(6, "sps_multilayer_extension_flag", sps_multilayer_extension_flag = r.get_bits(1));
    v(6, "sps_3d_extension_flag",         sps_3d_extension_flag         = r.get_bits(1));
    v(6, "sps_scc_extension_flag",        sps_scc_extension_flag        = r.get_bits(1));
    v(6, "sps_extension_4bits",           sps_extension_4bits           = r.get_bits(4));
  }

  if (sps_range_extension_flag)
    parse_sps_range_extension(6, r);
  if (sps_multilayer_extension_flag)
    parse_sps_multilayer_extension(6, r);
  if (sps_3d_extension_flag)
    parse_sps_3d_extension(6, r);
  if (sps_scc_extension_flag)
    parse_sps_scc_extension(6, r);
  if (sps_extension_4bits)
    parse_sps_extension_4bits(6, r);

  v(6, "rbsp_stop_one_bit", r.get_bits(1));
  v(4, fmt::format("Remaining bits: {0}", r.get_remaining_bits()));
}

static void
parse_hevcc(mtx::bits::reader_c &r) {
  unsigned int num_arrays;

  v(0, "configuration_version",              r.get_bits(8));
  v(0, "general_profile_space",              r.get_bits(2));
  v(0, "general_tier_flag",                  r.get_bits(1));
  v(0, "general_profile_idc",                r.get_bits(5));
  v(0, "general_profile_compatibility_flag", x(8,  r.get_bits(32)));
  v(0, "general_progressive_source_flag",    r.get_bits(1));
  v(0, "general_interlace_source_flag",      r.get_bits(1));
  v(0, "general_nonpacked_constraint_flag",  r.get_bits(1));
  v(0, "general_frame_only_constraint_flag", r.get_bits(1));
  v(0, "reserved",                           x(11, r.get_bits(44)));
  v(0, "general_level_idc",                  r.get_bits(8));
  v(0, "reserved",                           r.get_bits(4));
  v(0, "min_spatial_segmentation_idc",       r.get_bits(12));
  v(0, "reserved",                           r.get_bits(6));
  v(0, "parallelism_type",                   r.get_bits(2));
  v(0, "reserved",                           r.get_bits(6));
  v(0, "chroma_format_idc",                  r.get_bits(2));
  v(0, "reserved",                           r.get_bits(5));
  v(0, "bit_depth_luma_minus8",              r.get_bits(3));
  v(0, "reserved",                           r.get_bits(5));
  v(0, "bit_depth_chroma_minus8",            r.get_bits(3));
  v(0, "reserved",                           x(4,  r.get_bits(16)));
  v(0, "reserved",                           r.get_bits(2));
  v(0, "max_sub_layers",                     r.get_bits(3));
  v(0, "temporal_id_nesting_flag",           r.get_bits(1));
  v(0, "size_nalu_minus_one",                r.get_bits(2));
  v(0, "num_arrays",                         num_arrays = r.get_bits(8));

  for (auto array_idx = 0u; array_idx < num_arrays; ++array_idx) {
    auto byte           = r.get_bits(8);
    auto type           = byte & 0x3f;
    auto type_name      = type == mtx::hevc::NALU_TYPE_VIDEO_PARAM ? "video parameter set"
                        : type == mtx::hevc::NALU_TYPE_SEQ_PARAM   ? "sequence parameter set"
                        : type == mtx::hevc::NALU_TYPE_PIC_PARAM   ? "picture parameter set"
                        : type == mtx::hevc::NALU_TYPE_PREFIX_SEI  ? "supplemental enhancement information"
                        :                                      "unknown";
    auto nal_unit_count = r.get_bits(16);

    v(0, fmt::format("parameter set array {0}", array_idx));
    v(2, "array_completeness", (byte & 0x80) >> 7);
    v(2, "reserved",           (byte & 0x40) >> 6);
    v(2, "nal_unit_type",      fmt::format("{0} ({1})", type, type_name));
    v(2, "nal_unit_count",     nal_unit_count);

    for (auto nal_unit_idx = 0u; nal_unit_idx < nal_unit_count; ++nal_unit_idx) {
      auto nal_unit_size = r.get_bits(16);
      auto data          = memory_c::alloc(nal_unit_size);

      r.get_bytes(data->get_buffer(), nal_unit_size);

      data = mtx::mpeg::nalu_to_rbsp(data);

      v(2, fmt::format("NAL unit {0}", nal_unit_idx));
      v(4, "nal_unit_size", fmt::format("{0} (RBSP size: {1})", nal_unit_size, data->get_size()));

      mtx::bits::reader_c nalu_r{data->get_buffer(), data->get_size()};

      if (type == mtx::hevc::NALU_TYPE_SEQ_PARAM)
        parse_sps(nalu_r);
    }
  }

  v(0, fmt::format("Remaining bits: {0}", r.get_remaining_bits()));
}

static void
read_and_parse_hevcc(std::string const &file_name,
                     uint64_t file_pos,
                     uint64_t size) {
  mm_file_io_c in{file_name};

  in.setFilePointer(file_pos);
  auto data = in.read(size);
  auto r    = mtx::bits::reader_c{data->get_buffer(), data->get_size()};

  parse_hevcc(r);
}

int
main(int argc,
     char **argv) {
  mtx_common_init("hevcc_dump", argv[0]);

  setup();

  auto args = mtx::cli::args_in_utf8(argc, argv);
  while (mtx::cli::handle_common_args(args, "-r"))
    ;

  auto file_spec = parse_args(args);

  try {
    read_and_parse_hevcc(std::get<0>(file_spec), std::get<1>(file_spec), std::get<2>(file_spec));
  } catch (mtx::mm_io::open_x &) {
    mxerror("File not found\n");
  }

  mxexit();
}
