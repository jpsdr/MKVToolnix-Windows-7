/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   the track info params class

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/ebml.h"
#include "merge/generic_packetizer.h"
#include "merge/track_info.h"

track_info_c::track_info_c()
  : m_id{}
  , m_disable_multi_file{}
  , m_aspect_ratio{}
  , m_display_width{}
  , m_display_height{}
  , m_display_unit{generic_packetizer_c::ddu_pixels}
  , m_aspect_ratio_given{}
  , m_aspect_ratio_is_factor{}
  , m_aspect_ratio_factor_applied{}
  , m_display_dimensions_given{}
  , m_display_dimensions_source{option_source_e::none}
  , m_reset_timestamps{}
  , m_cues{CUE_STRATEGY_UNSPECIFIED}
  , m_compression{COMPRESSION_UNSPECIFIED}
  , m_no_chapters{}
  , m_no_global_tags{}
  , m_regenerate_track_uids{}
  , m_avi_audio_sync_enabled{}
  , m_avi_audio_data_rate{}
{
}

track_info_c &
track_info_c::operator =(const track_info_c &src) {
  m_id                               = src.m_id;
  m_fname                            = src.m_fname;

  m_atracks                          = src.m_atracks;
  m_btracks                          = src.m_btracks;
  m_stracks                          = src.m_stracks;
  m_vtracks                          = src.m_vtracks;
  m_track_tags                       = src.m_track_tags;
  m_disable_multi_file               = src.m_disable_multi_file;

  m_private_data                     = src.m_private_data ? src.m_private_data->clone() : src.m_private_data;

  m_all_fourccs                      = src.m_all_fourccs;

  m_display_properties               = src.m_display_properties;
  m_aspect_ratio                     = src.m_aspect_ratio;
  m_aspect_ratio_given               = false;
  m_aspect_ratio_is_factor           = false;
  m_aspect_ratio_factor_applied      = src.m_aspect_ratio_factor_applied;
  m_display_dimensions_given         = false;
  m_display_dimensions_source        = src.m_display_dimensions_source;

  m_timestamp_syncs                  = src.m_timestamp_syncs;
  m_tcsync                           = src.m_tcsync;

  m_reset_timestamps_specs           = src.m_reset_timestamps_specs;
  m_reset_timestamps                 = src.m_reset_timestamps;

  m_cue_creations                    = src.m_cue_creations;
  m_cues                             = src.m_cues;

  m_default_track_flags              = src.m_default_track_flags;
  m_default_track                    = src.m_default_track;

  m_fix_bitstream_frame_rate_flags   = src.m_fix_bitstream_frame_rate_flags;
  m_fix_bitstream_frame_rate         = src.m_fix_bitstream_frame_rate;

  m_forced_track_flags               = src.m_forced_track_flags;
  m_forced_track                     = src.m_forced_track;

  m_reduce_to_core                   = src.m_reduce_to_core;
  m_remove_dialog_normalization_gain = src.m_remove_dialog_normalization_gain;

  m_enabled_track_flags              = src.m_enabled_track_flags;
  m_enabled_track                    = src.m_enabled_track;

  m_hearing_impaired_flags           = src.m_hearing_impaired_flags;
  m_hearing_impaired_flag            = src.m_hearing_impaired_flag;

  m_visual_impaired_flags            = src.m_visual_impaired_flags;
  m_visual_impaired_flag             = src.m_visual_impaired_flag;

  m_text_descriptions_flags          = src.m_text_descriptions_flags;
  m_text_descriptions_flag           = src.m_text_descriptions_flag;

  m_original_flags                   = src.m_original_flags;
  m_original_flag                    = src.m_original_flag;

  m_commentary_flags                 = src.m_commentary_flags;
  m_commentary_flag                  = src.m_commentary_flag;

  m_languages                        = src.m_languages;
  m_language                         = src.m_language;

  m_sub_charsets                     = src.m_sub_charsets;
  m_sub_charset                      = src.m_sub_charset;

  m_all_tags                         = src.m_all_tags;
  m_tags_file_name                   = src.m_tags_file_name;
  m_tags                             = src.m_tags ? clone(src.m_tags) : std::shared_ptr<libmatroska::KaxTags>{};

  m_all_aac_is_sbr                   = src.m_all_aac_is_sbr;

  m_compression_list                 = src.m_compression_list;
  m_compression                      = src.m_compression;

  m_track_names                      = src.m_track_names;
  m_track_name                       = src.m_track_name;

  m_all_ext_timestamps               = src.m_all_ext_timestamps;
  m_ext_timestamps                   = src.m_ext_timestamps;

  m_audio_emphasis_list              = src.m_audio_emphasis_list;
  m_audio_emphasis                   = src.m_audio_emphasis;

  m_pixel_crop_list                  = src.m_pixel_crop_list;
  m_pixel_cropping                   = src.m_pixel_cropping;

  m_color_matrix_coeff_list          = src.m_color_matrix_coeff_list;
  m_color_matrix_coeff               = src.m_color_matrix_coeff;

  m_bits_per_channel_list            = src.m_bits_per_channel_list;
  m_bits_per_channel                 = src.m_bits_per_channel;

  m_chroma_subsample_list            = src.m_chroma_subsample_list;
  m_chroma_subsample                 = src.m_chroma_subsample;

  m_cb_subsample_list                = src.m_cb_subsample_list;
  m_cb_subsample                     = src.m_cb_subsample;

  m_chroma_siting_list               = src.m_chroma_siting_list;
  m_chroma_siting                    = src.m_chroma_siting;

  m_color_range_list                 = src.m_color_range_list;
  m_color_range                      = src.m_color_range;

  m_color_transfer_list              = src.m_color_transfer_list;
  m_color_transfer                   = src.m_color_transfer;

  m_color_primaries_list             = src.m_color_primaries_list;
  m_color_primaries                  = src.m_color_primaries;

  m_max_cll_list                     = src.m_max_cll_list;
  m_max_cll                          = src.m_max_cll;

  m_max_fall_list                    = src.m_max_fall_list;
  m_max_fall                         = src.m_max_fall;

  m_chroma_coordinates_list          = src.m_chroma_coordinates_list;
  m_chroma_coordinates               = src.m_chroma_coordinates;

  m_white_coordinates_list           = src.m_white_coordinates_list;
  m_white_coordinates                = src.m_white_coordinates;

  m_max_luminance_list               = src.m_max_luminance_list;
  m_max_luminance                    = src.m_max_luminance;

  m_min_luminance_list               = src.m_min_luminance_list;
  m_min_luminance                    = src.m_min_luminance;

  m_projection_type_list             = src.m_projection_type_list;
  m_projection_type                  = src.m_projection_type;

  m_projection_private_list          = src.m_projection_private_list;
  m_projection_private               = src.m_projection_private;

  m_projection_pose_yaw_list         = src.m_projection_pose_yaw_list;
  m_projection_pose_yaw              = src.m_projection_pose_yaw;

  m_projection_pose_pitch_list       = src.m_projection_pose_pitch_list;
  m_projection_pose_pitch            = src.m_projection_pose_pitch;

  m_projection_pose_roll_list        = src.m_projection_pose_roll_list;
  m_projection_pose_roll             = src.m_projection_pose_roll;

  m_stereo_mode_list                 = src.m_stereo_mode_list;
  m_stereo_mode                      = src.m_stereo_mode;

  m_alpha_mode_list                  = src.m_alpha_mode_list;
  m_alpha_mode                       = src.m_alpha_mode;

  m_field_order_list                 = src.m_field_order_list;
  m_field_order                      = src.m_field_order;

  m_attach_mode_list                 = src.m_attach_mode_list;

  m_no_chapters                      = src.m_no_chapters;
  m_no_global_tags                   = src.m_no_global_tags;
  m_regenerate_track_uids            = src.m_regenerate_track_uids;

  m_chapter_charset                  = src.m_chapter_charset;
  m_chapter_language                 = src.m_chapter_language;

  m_avi_audio_sync_enabled           = false;
  m_avi_audio_data_rate              = src.m_avi_audio_data_rate;
  m_default_durations                = src.m_default_durations;

  return *this;
}

bool
track_info_c::display_dimensions_or_aspect_ratio_set() {
  return option_source_e::none != m_display_dimensions_source;
}
