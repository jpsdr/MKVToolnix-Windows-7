/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/json.h"
#include "common/strings/editing.h"

namespace mtx { namespace id {

char const * const aac_is_sbr                      = "aac_is_sbr";                      // track ascii-string format:^(true|unknown)$
char const * const audio_bits_per_sample           = "audio_bits_per_sample";           // track unsigned-integer
char const * const audio_channels                  = "audio_channels";                  // track unsigned-integer
char const * const audio_output_sampling_frequency = "audio_output_sampling_frequency"; // track unsigned-integer
char const * const audio_sampling_frequency        = "audio_sampling_frequency";        // track unsigned-integer
char const * const codec_delay                     = "codec_delay";                     // track unsigned-integer
char const * const codec_id                        = "codec_id";                        // track unicoode-string
char const * const codec_private_data              = "codec_private_data";              // track binary
char const * const codec_private_length            = "codec_private_length";            // track unsigned-integer
char const * const content_encoding_algorithms     = "content_encoding_algorithms";     // track ascii-string
char const * const cropping                        = "cropping";                        // track ascii-string format:
char const * const date_local                      = "date_local";                      // container ascii-string format:^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}([+-]\d{2}:\d{2}|Z)$
char const * const date_utc                        = "date_utc";                        // container ascii-string format:^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}([+-]\d{2}:\d{2}|Z)$
char const * const default_duration                = "default_duration";                // track unsigned-integer
char const * const default_track                   = "default_track";                   // track boolean
char const * const display_dimensions              = "display_dimensions";              // track ascii-string format:^\d+x\d+$
char const * const duration                        = "duration";                        // container unsigned-integer
char const * const encoding                        = "encoding";                        // track ascii-string
char const * const enabled_track                   = "enabled_track";                   // track boolean
char const * const forced_track                    = "forced_track";                    // track boolean
char const * const language                        = "language";                        // track ascii-string format:^\w{3}$
char const * const minimum_timestamp               = "minimum_timestamp";               // track unsigned-integer
char const * const mpeg4_p10_es_video              = "mpeg4_p10_es_video";              // track boolean
char const * const mpeg4_p10_video                 = "mpeg4_p10_video";                 // track boolean
char const * const mpegh_p2_es_video               = "mpegh_p2_es_video";               // track boolean
char const * const mpegh_p2_video                  = "mpegh_p2_video";                  // track boolean
char const * const muxing_application              = "muxing_application";              // container unicode-string
char const * const multiplexed_tracks              = "multiplexed_tracks";              // track [ unsigned-integer ]
char const * const next_segment_uid                = "next_segment_uid";                // container ascii-string format:^[0-9A-F]{32}$
char const * const number                          = "number";                          // container unsigned-integer
char const * const other_file                      = "other_file";                      // container unicoode-string
char const * const packetizer                      = "packetizer";                      // track unicoode-string
char const * const pixel_dimensions                = "pixel_dimensions";                // track ascii-string format:^\d+x\d+$
char const * const playlist                        = "playlist";                        // container boolean
char const * const playlist_chapters               = "playlist_chapters";               // container unsigned-integer
char const * const playlist_duration               = "playlist_duration";               // container unsigned-integer
char const * const playlist_file                   = "playlist_file";                   // container uinstr
char const * const playlist_size                   = "playlist_size";                   // container unsigned-integer
char const * const previous_segment_uid            = "previous_segment_uid";            // container ascii-string format:^[0-9A-F]{32}$
char const * const program_number                  = "program_number";                  // track unsigned-integer, container unsigned-integer
char const * const programs                        = "programs";                        // container array
char const * const segment_uid                     = "segment_uid";                     // container ascii-string format:^[0-9A-F]{32}$
char const * const service_name                    = "service_name";                    // container unicode-string
char const * const service_provider                = "service_provider";                // container unicode-string
char const * const stereo_mode                     = "stereo_mode";                     // track unsigned-integer
char const * const stream_id                       = "stream_id";                       // track unsigned-integer
char const * const sub_stream_id                   = "sub_stream_id";                   // track unsigned-integer
char const * const teletext_page                   = "teletext_page";                   // track unsigned-integer
char const * const text_subtitles                  = "text_subtitles";                  // track boolean
char const * const title                           = "title";                           // container unicoode-string
char const * const track_name                      = "track_name";                      // track unicoode-string
char const * const uid                             = "uid";                             // track attachments unsigned-integer
char const * const writing_application             = "writing_application";             // container unicode-string

using verbose_info_t = std::vector<std::pair<std::string, nlohmann::json>>;

class info_c {
protected:
  verbose_info_t m_info;

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

  info_c &
  add(std::string const &key,
      char const *value,
      std::string const &unset_value = {}) {
    if (std::string{value} != unset_value)
      set(key, value);
    return *this;
  }

  info_c &
  add(std::string const &key,
      boost::format const &value) {
    return add(key, value.str());
  }

  verbose_info_t const &get() const {
    return m_info;
  }
};

}}
