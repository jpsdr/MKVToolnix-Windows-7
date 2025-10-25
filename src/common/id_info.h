/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/json.h"
#include "common/strings/editing.h"

namespace mtx::id {

char const * const aac_is_sbr                      = "aac_is_sbr";                      // track ascii-string format:^(true|unknown)$
char const * const alpha_mode                      = "alpha_mode";                      // track unsigned-integer
char const * const audio_bits_per_sample           = "audio_bits_per_sample";           // track unsigned-integer
char const * const audio_channels                  = "audio_channels";                  // track unsigned-integer
char const * const audio_emphasis                  = "audio_emphasis";                  // track unsigned-integer
char const * const audio_output_sampling_frequency = "audio_output_sampling_frequency"; // track unsigned-integer
char const * const audio_sampling_frequency        = "audio_sampling_frequency";        // track unsigned-integer
char const * const cb_subsample                    = "cb_subsample";                    // track ascii-string format:^-?[0-9]+,-?[0-9]+$
char const * const chroma_siting                   = "chroma_siting";                   // track ascii-string format:^-?[0-9]+,-?[0-9]+$
char const * const chroma_subsample                = "chroma_subsample";                // track ascii-string format:^-?[0-9]+,-?[0-9]+$
char const * const chromaticity_coordinates        = "chromaticity_coordinates";        // track ascii-string format:^-?[0-9]+(\.[0-9]+)?,-?[0-9]+(\.[0-9]+)?,-?[0-9]+(\.[0-9]+)?,-?[0-9]+(\.[0-9]+)?,-?[0-9]+(\.[0-9]+)?,-?[0-9]+(\.[0-9]+)?$
char const * const codec_delay                     = "codec_delay";                     // track unsigned-integer
char const * const codec_id                        = "codec_id";                        // track unicoode-string
char const * const codec_name                      = "codec_name";                      // track unicoode-string
char const * const codec_private_data              = "codec_private_data";              // track binary
char const * const codec_private_length            = "codec_private_length";            // track unsigned-integer
char const * const color_bits_per_channel          = "color_bits_per_channel";          // track unsigned-integer
char const * const color_matrix_coefficients       = "color_matrix_coefficients";       // track unsigned-integer
char const * const color_primaries                 = "color_primaries";                 // track unsigned-integer
char const * const color_range                     = "color_range";                     // track unsigned-integer
char const * const color_transfer_characteristics  = "color_transfer_characteristics";  // track unsigned-integer
char const * const content_encoding_algorithms     = "content_encoding_algorithms";     // track ascii-string
char const * const cropping                        = "cropping";                        // track ascii-string format:
char const * const date_local                      = "date_local";                      // container ascii-string format:^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}([+-]\d{2}:\d{2}|Z)$
char const * const date_utc                        = "date_utc";                        // container ascii-string format:^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}([+-]\d{2}:\d{2}|Z)$
char const * const default_duration                = "default_duration";                // track unsigned-integer
char const * const default_track                   = "default_track";                   // track boolean
char const * const display_dimensions              = "display_dimensions";              // track ascii-string format:^\d+x\d+$
char const * const display_unit                    = "display_unit";                    // track unsigned-integer
char const * const duration                        = "duration";                        // container unsigned-integer
char const * const enabled_track                   = "enabled_track";                   // track boolean
char const * const encoding                        = "encoding";                        // track ascii-string
char const * const flag_commentary                 = "flag_commentary";                 // track boolean
char const * const flag_hearing_impaired           = "flag_hearing_impaired";           // track boolean
char const * const flag_original                   = "flag_original";                   // track boolean
char const * const flag_text_descriptions          = "flag_text_descriptions";          // track boolean
char const * const flag_visual_impaired            = "flag_visual_impaired";            // track boolean
char const * const forced_track                    = "forced_track";                    // track boolean
char const * const language                        = "language";                        // track ascii-string format:^\w{3}$
char const * const language_ietf                   = "language_ietf";                   // track ascii-string
char const * const max_content_light               = "max_content_light";               // track unsigned-integer
char const * const max_frame_light                 = "max_frame_light";                 // track unsigned-integer
char const * const max_luminance                   = "max_luminance";                   // track floating-point
char const * const min_luminance                   = "min_luminance";                   // track floating-point
char const * const minimum_timestamp               = "minimum_timestamp";               // track unsigned-integer
char const * const mpeg4_p10_es_video              = "mpeg4_p10_es_video";              // track boolean
char const * const mpeg4_p10_video                 = "mpeg4_p10_video";                 // track boolean
char const * const mpegh_p2_es_video               = "mpegh_p2_es_video";               // track boolean
char const * const mpegh_p2_video                  = "mpegh_p2_video";                  // track boolean
char const * const multiplexed_tracks              = "multiplexed_tracks";              // track [ unsigned-integer ]
char const * const muxing_application              = "muxing_application";              // container unicode-string
char const * const next_segment_uid                = "next_segment_uid";                // container ascii-string format:^[0-9A-F]{32}$
char const * const num_index_entries               = "num_index_entries";               // track unsigned-integer
char const * const number                          = "number";                          // container unsigned-integer
char const * const other_file                      = "other_file";                      // container unicoode-string
char const * const packetizer                      = "packetizer";                      // track unicoode-string
char const * const pixel_dimensions                = "pixel_dimensions";                // track ascii-string format:^\d+x\d+$
char const * const playlist                        = "playlist";                        // container boolean
char const * const playlist_chapters               = "playlist_chapters";               // container unsigned-integer
char const * const playlist_duration               = "playlist_duration";               // container unsigned-integer
char const * const playlist_file                   = "playlist_file";                   // container unicode-string
char const * const playlist_size                   = "playlist_size";                   // container unsigned-integer
char const * const previous_segment_uid            = "previous_segment_uid";            // container ascii-string format:^[0-9A-F]{32}$
char const * const program_number                  = "program_number";                  // track unsigned-integer, container unsigned-integer
char const * const programs                        = "programs";                        // container array
char const * const projection_pose_pitch           = "projection_pose_pitch";           // track floating-point
char const * const projection_pose_roll            = "projection_pose_roll";            // track floating-point
char const * const projection_pose_yaw             = "projection_pose_yaw";             // track floating-point
char const * const projection_private              = "projection_private";              // track ascii-string format:^([0-9A-F]{2})*$
char const * const projection_type                 = "projection_type";                 // track unsigned-integer
char const * const segment_uid                     = "segment_uid";                     // container ascii-string format:^[0-9A-F]{32}$
char const * const service_name                    = "service_name";                    // container unicode-string
char const * const service_provider                = "service_provider";                // container unicode-string
char const * const stereo_mode                     = "stereo_mode";                     // track unsigned-integer
char const * const stream_id                       = "stream_id";                       // track unsigned-integer
char const * const sub_stream_id                   = "sub_stream_id";                   // track unsigned-integer
char const * const teletext_page                   = "teletext_page";                   // track unsigned-integer
char const * const text_subtitles                  = "text_subtitles";                  // track boolean
char const * const timestamp_scale                 = "timestamp_scale";                 // container unsigned-integer
char const * const title                           = "title";                           // container unicoode-string
char const * const track_name                      = "track_name";                      // track unicoode-string
char const * const uid                             = "uid";                             // track attachments unsigned-integer
char const * const white_color_coordinates         = "white_color_coordinates";         // track ascii-string format:^-?[0-9]+(\.[0-9]+)?,-?[0-9]+(\.[0-9]+)?$
char const * const writing_application             = "writing_application";             // container unicode-string

using verbose_info_t = std::vector<std::pair<std::string, nlohmann::json>>;

class info_c {
protected:
  verbose_info_t m_info;

private:
  template<typename... Args>
  bool
  any(Args... args) {
    return (... || args);
  }

  template<typename T>
  std::string
  join([[maybe_unused]] std::string const &separator,
       T const &value) {
    return fmt::to_string(value);
  }

  template<typename Tfirst,
           typename... Trest>
  std::string
  join(std::string const &separator,
       Tfirst const &first,
       Trest... rest) {
    return fmt::to_string(first) + separator + join(separator, rest...);
  }

public:
  template<typename T> info_c &
  set(std::string const &key,
      T const &value) {
    m_info.emplace_back(key, value);
    return *this;
  }

  template<typename T> info_c &
  add(std::string const &key,
      T const &value,
      T const &unset_value = T{}) {
    if (value != unset_value)
      set(key, value);
    return *this;
  }

  info_c &add(std::string const &key, memory_c const *value);
  info_c &add(std::string const &key, memory_cptr const &value);

  info_c &
  add(std::string const &key,
      char const *value,
      std::string const &unset_value = {}) {
    if (std::string{value} != unset_value)
      set(key, value);
    return *this;
  }

  template<typename T>
  info_c &
  add(std::string const &key,
      std::optional<T> const &value) {
    if (value.has_value())
      set(key, *value);
    return *this;
  }

  template<typename... Tvalues>
  info_c &
  add_joined(std::string const &key,
             std::string const &separator,
             Tvalues... values) {
    if (any(values...))
      return add(key, join(separator, values...));

    return *this;
  }

  verbose_info_t const &get() const {
    return m_info;
  }
};

}
