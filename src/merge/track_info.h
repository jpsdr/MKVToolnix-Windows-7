/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   class definition for the track info parameters class

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/compression.h"
#include "common/option_with_source.h"
#include "common/stereo_mode.h"
#include "merge/item_selector.h"

namespace libmatroska {
class KaxTags;
}

// CUE_STRATEGY_* control the creation of cue entries for a track.
// UNSPECIFIED: is used for command line parsing.
// NONE:        don't create any cue entries.
// IFRAMES:     Create cue entries for all I frames.
// ALL:         Create cue entries for all frames (not really useful).
// SPARSE:      Create cue entries for I frames if no video track exists, but
//              create at most one cue entries every two seconds. Used for
//              audio only files.
enum cue_strategy_e {
  CUE_STRATEGY_UNSPECIFIED = -1,
  CUE_STRATEGY_NONE,
  CUE_STRATEGY_IFRAMES,
  CUE_STRATEGY_ALL,
  CUE_STRATEGY_SPARSE
};

struct timestamp_sync_t {
  int64_t displacement;
  double numerator, denominator;

  timestamp_sync_t()
  : displacement(0)
  , numerator(1.0)
  , denominator(1.0)
  {
  }

  double factor() {
    return numerator / denominator;
  }
};

enum default_track_priority_e {
  DEFAULT_TRACK_PRIOIRTY_NONE        =   0,
  DEFAULT_TRACK_PRIORITY_FROM_TYPE   =  10,
  DEFAULT_TRACK_PRIORITY_FROM_SOURCE =  50,
  DEFAULT_TRACK_PRIORITY_CMDLINE     = 255
};

struct display_properties_t {
  float aspect_ratio;
  bool ar_factor;
  int width, height;

  display_properties_t()
  : aspect_ratio(0)
  , ar_factor(false)
  , width(0)
  , height(0)
  {
  }
};

struct pixel_crop_t {
  int left, top, right, bottom;

  pixel_crop_t()
  : left{}
  , top{}
  , right{}
  , bottom{}
  {
  }

  pixel_crop_t(int p_left, int p_top, int p_right, int p_bottom)
  : left{p_left}
  , top{p_top}
  , right{p_right}
  , bottom{p_bottom}
  {
  }
};

struct chroma_subsample_t {
  int hori, vert;

  chroma_subsample_t()
  : hori{-1}
  , vert{-1}
  {
  }
  chroma_subsample_t(int p_hori, int p_vert)
  : hori{p_hori}
  , vert{p_vert}
  {
  }
  chroma_subsample_t(std::vector<int> data)
  : hori{data[0]}
  , vert{data[1]}
  {
  }
  static int const num_properties = 2;
};

struct cb_subsample_t {
  int hori, vert;

  cb_subsample_t()
  : hori{-1}
  , vert{-1}
  {
  }
  cb_subsample_t(int p_hori, int p_vert)
  : hori{p_hori}
  , vert{p_vert}
  {
  }
  cb_subsample_t(std::vector<int> data)
  : hori{data[0]}
  , vert{data[1]}
  {
  }
  static int const num_properties = 2;
};


struct chroma_siting_t {
  int hori, vert;

  chroma_siting_t()
  : hori{-1}
  , vert{-1}
  {
  }
  chroma_siting_t(int p_hori, int p_vert)
  : hori{p_hori}
  , vert{p_vert}
  {
  }
  chroma_siting_t(std::vector<int> data)
  : hori{data[0]}
  , vert{data[1]}
  {
  }
  static int const num_properties = 2;
};

struct chroma_coordinates_t {
  float red_x, red_y, green_x, green_y, blue_x, blue_y;

  chroma_coordinates_t()
  : red_x{-1.0}
  , red_y{-1.0}
  , green_x{-1.0}
  , green_y{-1.0}
  , blue_x{-1.0}
  , blue_y{-1.0}
  {
  }
  chroma_coordinates_t(float p_red_x,   float p_red_y,
                       float p_green_x, float p_green_y,
                       float p_blue_x,  float p_blue_y)
  : red_x{p_red_x}
  , red_y{p_red_y}
  , green_x{p_green_x}
  , green_y{p_green_y}
  , blue_x{p_blue_x}
  , blue_y{p_blue_y}
  {
  }
  chroma_coordinates_t(std::vector<float> data)
  : red_x{data[0]}
  , red_y{data[1]}
  , green_x{data[2]}
  , green_y{data[3]}
  , blue_x{data[4]}
  , blue_y{data[5]}
  {
  }
  static int const num_properties = 6;
};

struct white_colour_coordinates_t {
  float x, y;

  white_colour_coordinates_t()
  : x{-1.0}
  , y{-1.0}
  {
  }
  white_colour_coordinates_t(float p_x, float p_y)
  : x{p_x}
  , y{p_y}
  {
  }
  white_colour_coordinates_t(std::vector<float> data)
  : x{data[0]}
  , y{data[1]}
  {
  }
  static int const num_properties = 2;
};

enum attach_mode_e {
  ATTACH_MODE_SKIP,
  ATTACH_MODE_TO_FIRST_FILE,
  ATTACH_MODE_TO_ALL_FILES,
};

class track_info_c {
protected:
  bool m_initialized;

public:
  // The track ID.
  int64_t m_id;

  // Options used by the readers.
  std::string m_fname;
  item_selector_c<bool> m_atracks, m_vtracks, m_stracks, m_btracks, m_track_tags;
  bool m_disable_multi_file;

  // Options used by the packetizers.
  memory_cptr m_private_data;

  std::map<int64_t, std::string> m_all_fourccs;
  std::string m_fourcc;
  std::map<int64_t, display_properties_t> m_display_properties;
  float m_aspect_ratio;
  int m_display_width, m_display_height;
  bool m_aspect_ratio_given, m_aspect_ratio_is_factor, m_display_dimensions_given;
  option_source_e m_display_dimensions_source;

  std::map<int64_t, timestamp_sync_t> m_timestamp_syncs; // As given on the command line
  timestamp_sync_t m_tcsync;                       // For this very track

  std::map<int64_t, bool> m_reset_timestamps_specs;
  bool m_reset_timestamps;

  std::map<int64_t, cue_strategy_e> m_cue_creations; // As given on the command line
  cue_strategy_e m_cues;          // For this very track

  std::map<int64_t, bool> m_default_track_flags; // As given on the command line
  boost::logic::tribool m_default_track;    // For this very track

  std::map<int64_t, bool> m_fix_bitstream_frame_rate_flags; // As given on the command line
  boost::logic::tribool m_fix_bitstream_frame_rate;         // For this very track

  std::map<int64_t, bool> m_forced_track_flags; // As given on the command line
  boost::logic::tribool m_forced_track;    // For this very track

  std::map<int64_t, bool> m_enabled_track_flags; // As given on the command line
  boost::logic::tribool m_enabled_track;    // For this very track

  std::map<int64_t, std::string> m_languages; // As given on the command line
  std::string m_language;              // For this very track

  std::map<int64_t, std::string> m_sub_charsets; // As given on the command line
  std::string m_sub_charset;           // For this very track

  std::map<int64_t, std::string> m_all_tags;     // As given on the command line
  std::string m_tags_file_name;        // For this very track
  std::shared_ptr<KaxTags> m_tags;     // For this very track

  std::map<int64_t, bool> m_all_aac_is_sbr; // For AAC+/HE-AAC/SBR

  std::map<int64_t, compression_method_e> m_compression_list; // As given on the cmd line
  compression_method_e m_compression; // For this very track

  std::map<int64_t, std::string> m_track_names; // As given on the command line
  std::string m_track_name;            // For this very track

  std::map<int64_t, std::string> m_all_ext_timestamps; // As given on the command line
  std::string m_ext_timestamps;         // For this very track

  std::map<int64_t, pixel_crop_t> m_pixel_crop_list; // As given on the command line
  option_with_source_c<pixel_crop_t> m_pixel_cropping;  // For this very track

  std::map<int64_t, stereo_mode_c::mode> m_stereo_mode_list; // As given on the command line
  option_with_source_c<stereo_mode_c::mode> m_stereo_mode;   // For this very track

  std::map<int64_t, uint64_t> m_field_order_list; // As given on the command line
  option_with_source_c<uint64_t> m_field_order;   // For this very track

  std::map<int64_t, int> m_colour_matrix_coeff_list; // As given on the command line
  option_with_source_c<int> m_colour_matrix_coeff; // For this very track

  std::map<int64_t, int> m_bits_per_channel_list; // As given on the command line
  option_with_source_c<int> m_bits_per_channel; // For this very track

  std::map<int64_t, chroma_subsample_t> m_chroma_subsample_list; // As given on the command line
  option_with_source_c<chroma_subsample_t> m_chroma_subsample; // For this very track

  std::map<int64_t, cb_subsample_t> m_cb_subsample_list; // As given on the command line
  option_with_source_c<cb_subsample_t> m_cb_subsample; // For this very track

  std::map<int64_t, chroma_siting_t> m_chroma_siting_list; // As given on the command line
  option_with_source_c<chroma_siting_t> m_chroma_siting; // For this very track

  std::map<int64_t, int> m_colour_range_list; // As given on the command line
  option_with_source_c<int> m_colour_range; // For this very track

  std::map<int64_t, int> m_colour_transfer_list; // As given on the command line
  option_with_source_c<int> m_colour_transfer; // For this very track

  std::map<int64_t, int> m_colour_primaries_list; // As given on the command line
  option_with_source_c<int> m_colour_primaries; // For this very track

  std::map<int64_t, int> m_max_cll_list; // As given on the command line
  option_with_source_c<int> m_max_cll; // For this very track

  std::map<int64_t, int> m_max_fall_list; // As given on the command line
  option_with_source_c<int> m_max_fall; // For this very track

  std::map<int64_t, chroma_coordinates_t> m_chroma_coordinates_list; // As given on the command line
  option_with_source_c<chroma_coordinates_t> m_chroma_coordinates; // For this very track

  std::map<int64_t, white_colour_coordinates_t> m_white_coordinates_list; // As given on the command line
  option_with_source_c<white_colour_coordinates_t> m_white_coordinates; // For this very track

  std::map<int64_t, float> m_max_luminance_list; // As given on the command line
  option_with_source_c<float> m_max_luminance; // For this very track

  std::map<int64_t, float> m_min_luminance_list; // As given on the command line
  option_with_source_c<float> m_min_luminance; // For this very track

  std::map<int64_t, uint64_t> m_projection_type_list; // As given on the command line
  option_with_source_c<uint64_t> m_projection_type; // For this very track

  std::map<int64_t, memory_cptr> m_projection_private_list; // As given on the command line
  option_with_source_c<memory_cptr> m_projection_private; // For this very track

  std::map<int64_t, double> m_projection_pose_yaw_list; // As given on the command line
  option_with_source_c<double> m_projection_pose_yaw; // For this very track

  std::map<int64_t, double> m_projection_pose_pitch_list; // As given on the command line
  option_with_source_c<double> m_projection_pose_pitch; // For this very track

  std::map<int64_t, double> m_projection_pose_roll_list; // As given on the command line
  option_with_source_c<double> m_projection_pose_roll; // For this very track

  std::map<int64_t, std::pair<int64_t, bool>> m_default_durations; // As given on the command line
  std::map<int64_t, int> m_max_blockadd_ids; // As given on the command line

  std::map<int64_t, int> m_nalu_size_lengths;
  int m_nalu_size_length;

  item_selector_c<attach_mode_e> m_attach_mode_list; // As given on the command line

  std::map<int64_t, bool> m_reduce_to_core;

  bool m_no_chapters, m_no_global_tags;

  // Some file formats can contain chapters, but for some the charset
  // cannot be identified unambiguously (*cough* OGM *cough*).
  std::string m_chapter_charset, m_chapter_language;

  bool m_avi_audio_sync_enabled;
  int64_t m_avi_audio_data_rate;

public:
  track_info_c();
  track_info_c(const track_info_c &src) {
    *this = src;
  }
  virtual ~track_info_c() {
  }

  track_info_c &operator =(const track_info_c &src);
  virtual bool display_dimensions_or_aspect_ratio_set();
};

template<typename T>
typename T::mapped_type
get_option_for_track(T const &options,
                     int64_t track_id,
                     typename T::mapped_type const &default_value = typename T::mapped_type{}) {
  auto end = options.end();
  auto itr = options.find(track_id);

  if (itr != end)
    return itr->second;

  itr = options.find(-1ll);
  if (itr != end)
    return itr->second;

  return default_value;
}
