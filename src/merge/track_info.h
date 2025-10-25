/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definition for the track info parameters class

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/audio_emphasis.h"
#include "common/bcp47.h"
#include "common/compression.h"
#include "common/math.h"
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
  int64_t displacement{};
  mtx_mp_rational_t factor{1, 1};
};

struct display_properties_t {
  double aspect_ratio;
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

template<typename T>
struct property_pair_t {
  std::optional<T> hori, vert;

  property_pair_t() = default;
  property_pair_t(std::optional<T> const &p_hori, std::optional<T> const &p_vert)
  : hori{p_hori}
  , vert{p_vert}
  {
  }
  property_pair_t(T p_hori, T p_vert)
  : hori{p_hori}
  , vert{p_vert}
  {
  }
  property_pair_t(std::vector<T> data)
  : hori{data[0]}
  , vert{data[1]}
  {
  }
  static int const num_properties = 2;
};

using subsample_or_siting_t     = property_pair_t<uint64_t>;
using white_color_coordinates_t = property_pair_t<double>;

struct chroma_coordinates_t {
  std::optional<double> red_x, red_y, green_x, green_y, blue_x, blue_y;

  chroma_coordinates_t() = default;
  chroma_coordinates_t(std::optional<double> const &p_red_x,   std::optional<double> const &p_red_y,
                       std::optional<double> const &p_green_x, std::optional<double> const &p_green_y,
                       std::optional<double> const &p_blue_x,  std::optional<double> const &p_blue_y)
  : red_x{p_red_x}
  , red_y{p_red_y}
  , green_x{p_green_x}
  , green_y{p_green_y}
  , blue_x{p_blue_x}
  , blue_y{p_blue_y}
  {
  }
  chroma_coordinates_t(double p_red_x,   double p_red_y,
                       double p_green_x, double p_green_y,
                       double p_blue_x,  double p_blue_y)
  : red_x{p_red_x}
  , red_y{p_red_y}
  , green_x{p_green_x}
  , green_y{p_green_y}
  , blue_x{p_blue_x}
  , blue_y{p_blue_y}
  {
  }
  chroma_coordinates_t(std::vector<double> data)
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

enum attach_mode_e {
  ATTACH_MODE_SKIP,
  ATTACH_MODE_TO_FIRST_FILE,
  ATTACH_MODE_TO_ALL_FILES,
};

class track_info_c {
protected:
  bool m_initialized;

public:
  enum special_track_id_e {
    all_tracks_id    = -1,
    chapter_track_id = -2,
  };

  // The track ID.
  int64_t m_id;

  // Options used by the readers.
  std::string m_fname, m_title;
  item_selector_c<bool> m_atracks, m_vtracks, m_stracks, m_btracks, m_track_tags;
  bool m_disable_multi_file;

  // Options used by the packetizers.
  memory_cptr m_private_data;

  std::map<int64_t, std::string> m_all_fourccs;
  std::string m_fourcc;
  std::map<int64_t, display_properties_t> m_display_properties;
  double m_aspect_ratio;
  int m_display_width, m_display_height, m_display_unit;
  bool m_aspect_ratio_given, m_aspect_ratio_is_factor, m_aspect_ratio_factor_applied, m_display_dimensions_given;
  option_source_e m_display_dimensions_source;

  std::map<int64_t, timestamp_sync_t> m_timestamp_syncs; // As given on the command line
  timestamp_sync_t m_tcsync;                       // For this very track

  std::map<int64_t, bool> m_reset_timestamps_specs;
  bool m_reset_timestamps;

  std::map<int64_t, cue_strategy_e> m_cue_creations; // As given on the command line
  cue_strategy_e m_cues;          // For this very track

  std::map<int64_t, bool> m_default_track_flags; // As given on the command line
  option_with_source_c<bool> m_default_track;    // For this very track

  std::map<int64_t, bool> m_fix_bitstream_frame_rate_flags; // As given on the command line
  std::optional<bool> m_fix_bitstream_frame_rate;         // For this very track

  std::map<int64_t, bool> m_forced_track_flags; // As given on the command line
  option_with_source_c<bool> m_forced_track;    // For this very track

  std::map<int64_t, bool> m_enabled_track_flags; // As given on the command line
  option_with_source_c<bool> m_enabled_track;    // For this very track

  std::map<int64_t, bool> m_hearing_impaired_flags;
  option_with_source_c<bool> m_hearing_impaired_flag;

  std::map<int64_t, bool> m_visual_impaired_flags;
  option_with_source_c<bool> m_visual_impaired_flag;

  std::map<int64_t, bool> m_text_descriptions_flags;
  option_with_source_c<bool> m_text_descriptions_flag;

  std::map<int64_t, bool> m_original_flags;
  option_with_source_c<bool> m_original_flag;

  std::map<int64_t, bool> m_commentary_flags;
  option_with_source_c<bool> m_commentary_flag;

  std::map<int64_t, mtx::bcp47::language_c> m_languages; // As given on the command line
  mtx::bcp47::language_c m_language;                     // For this very track

  std::map<int64_t, std::string> m_sub_charsets; // As given on the command line
  std::string m_sub_charset;           // For this very track

  std::map<int64_t, std::string> m_all_tags;     // As given on the command line
  std::string m_tags_file_name;        // For this very track
  std::shared_ptr<libmatroska::KaxTags> m_tags;     // For this very track

  std::map<int64_t, bool> m_all_aac_is_sbr; // For AAC+/HE-AAC/SBR

  std::map<int64_t, compression_method_e> m_compression_list; // As given on the cmd line
  compression_method_e m_compression; // For this very track

  std::map<int64_t, std::string> m_track_names; // As given on the command line
  std::string m_track_name;            // For this very track

  std::map<int64_t, std::string> m_all_ext_timestamps; // As given on the command line
  std::string m_ext_timestamps;         // For this very track

  std::map<int64_t, audio_emphasis_c::mode_e> m_audio_emphasis_list; // As given on the command line
  option_with_source_c<audio_emphasis_c::mode_e> m_audio_emphasis;   // For this very track

  std::map<int64_t, pixel_crop_t> m_pixel_crop_list; // As given on the command line
  option_with_source_c<pixel_crop_t> m_pixel_cropping;  // For this very track

  std::map<int64_t, stereo_mode_c::mode> m_stereo_mode_list; // As given on the command line
  option_with_source_c<stereo_mode_c::mode> m_stereo_mode;   // For this very track

  std::map<int64_t, bool> m_alpha_mode_list;
  option_with_source_c<bool> m_alpha_mode;

  std::map<int64_t, uint64_t> m_field_order_list; // As given on the command line
  option_with_source_c<uint64_t> m_field_order;   // For this very track

  std::map<int64_t, uint64_t> m_color_matrix_coeff_list; // As given on the command line
  option_with_source_c<uint64_t> m_color_matrix_coeff; // For this very track

  std::map<int64_t, uint64_t> m_bits_per_channel_list; // As given on the command line
  option_with_source_c<uint64_t> m_bits_per_channel; // For this very track

  std::map<int64_t, subsample_or_siting_t> m_chroma_subsample_list; // As given on the command line
  option_with_source_c<subsample_or_siting_t> m_chroma_subsample; // For this very track

  std::map<int64_t, subsample_or_siting_t> m_cb_subsample_list; // As given on the command line
  option_with_source_c<subsample_or_siting_t> m_cb_subsample; // For this very track

  std::map<int64_t, subsample_or_siting_t> m_chroma_siting_list; // As given on the command line
  option_with_source_c<subsample_or_siting_t> m_chroma_siting; // For this very track

  std::map<int64_t, uint64_t> m_color_range_list; // As given on the command line
  option_with_source_c<uint64_t> m_color_range; // For this very track

  std::map<int64_t, uint64_t> m_color_transfer_list; // As given on the command line
  option_with_source_c<uint64_t> m_color_transfer; // For this very track

  std::map<int64_t, uint64_t> m_color_primaries_list; // As given on the command line
  option_with_source_c<uint64_t> m_color_primaries; // For this very track

  std::map<int64_t, uint64_t> m_max_cll_list; // As given on the command line
  option_with_source_c<uint64_t> m_max_cll; // For this very track

  std::map<int64_t, uint64_t> m_max_fall_list; // As given on the command line
  option_with_source_c<uint64_t> m_max_fall; // For this very track

  std::map<int64_t, chroma_coordinates_t> m_chroma_coordinates_list; // As given on the command line
  option_with_source_c<chroma_coordinates_t> m_chroma_coordinates; // For this very track

  std::map<int64_t, white_color_coordinates_t> m_white_coordinates_list; // As given on the command line
  option_with_source_c<white_color_coordinates_t> m_white_coordinates; // For this very track

  std::map<int64_t, double> m_max_luminance_list; // As given on the command line
  option_with_source_c<double> m_max_luminance; // For this very track

  std::map<int64_t, double> m_min_luminance_list; // As given on the command line
  option_with_source_c<double> m_min_luminance; // For this very track

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

  std::map<int64_t, memory_cptr> m_color_space_list; // As given on the command line
  option_with_source_c<memory_cptr> m_color_space; // For this very track

  std::map<int64_t, std::pair<int64_t, bool>> m_default_durations; // As given on the command line

  item_selector_c<attach_mode_e> m_attach_mode_list; // As given on the command line

  std::map<int64_t, bool> m_reduce_to_core, m_remove_dialog_normalization_gain;

  bool m_no_chapters, m_no_global_tags, m_regenerate_track_uids;

  // Some file formats can contain chapters, but for some the charset
  // cannot be identified unambiguously (*cough* OGM *cough*).
  std::string m_chapter_charset;
  mtx::bcp47::language_c m_chapter_language;

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
