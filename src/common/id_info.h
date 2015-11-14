/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#ifndef MTX_INPUT_ID_INFO_H
#define MTX_INPUT_ID_INFO_H

#include "common/common_pch.h"

#include "common/strings/editing.h"
#include "nlohmann-json/src/json.hpp"

namespace mtx { namespace id {

char const * const aac_is_sbr                      = "aac_is_sbr";                      // ascii-string format:^(true|unknown)$
char const * const audio_bits_per_sample           = "audio_bits_per_sample";           // unsigned-integer
char const * const audio_channels                  = "audio_channels";                  // unsigned-integer
char const * const audio_output_sampling_frequency = "audio_output_sampling_frequency"; // unsigned-integer
char const * const audio_sampling_frequency        = "audio_sampling_frequency";        // unsigned-integer
char const * const codec_id                        = "codec_id";                        // unicoode-string
char const * const codec_private_data              = "codec_private_data";              // binary
char const * const codec_private_length            = "codec_private_length";            // unsigned-integer
char const * const content_encoding_algorithms     = "content_encoding_algorithms";     // ascii-string
char const * const cropping                        = "cropping";                        // ascii-string format:
char const * const default_duration                = "default_duration";                // unsigned-integer
char const * const default_track                   = "default_track";                   // boolean
char const * const display_dimensions              = "display_dimensions";              // ascii-string format:^\d+x\d+$
char const * const duration                        = "duration";                        // unsigned-integer
char const * const enabled_track                   = "enabled_track";                   // boolean
char const * const forced_track                    = "forced_track";                    // boolean
char const * const language                        = "language";                        // ascii-string format:^\w{3}$
char const * const mpeg4_p10_es_video              = "mpeg4_p10_es_video";              // boolean
char const * const mpeg4_p10_video                 = "mpeg4_p10_video";                 // boolean
char const * const mpegh_p2_es_video               = "mpegh_p2_es_video";               // boolean
char const * const mpegh_p2_video                  = "mpegh_p2_video";                  // boolean
char const * const next_segment_uid                = "next_segment_uid";                // ascii-string format:^[0-9A-F]{32}$
char const * const number                          = "number";                          // unsigned-integer
char const * const other_file                      = "other_file";                      // unicoode-string
char const * const packetizer                      = "packetizer";                      // unicoode-string
char const * const pixel_dimensions                = "pixel_dimensions";                // ascii-string format:^\d+x\d+$
char const * const playlist                        = "playlist";                        // boolean
char const * const playlist_chapters               = "playlist_chapters";               // unsigned-integer
char const * const playlist_duration               = "playlist_duration";               // unsigned-integer
char const * const playlist_file                   = "playlist_file";                   // uinstr
char const * const playlist_size                   = "playlist_size";                   // unsigned-integer
char const * const previous_segment_uid            = "previous_segment_uid";            // ascii-string format:^[0-9A-F]{32}$
char const * const segment_uid                     = "segment_uid";                     // ascii-string format:^[0-9A-F]{32}$
char const * const stereo_mode                     = "stereo_mode";                     // unsigned-integer
char const * const stream_id                       = "stream_id";                       // unsigned-integer
char const * const sub_stream_id                   = "sub_stream_id";                   // unsigned-integer
char const * const text_subtitles                  = "text_subtitles";                  // boolean
char const * const title                           = "title";                           // unicoode-string
char const * const track_name                      = "track_name";                      // unicoode-string
char const * const ts_pid                          = "ts_pid";                          // unsigned-integer
char const * const uid                             = "uid";                             // unsigned-integer

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

#endif // MTX_INPUT_ID_INFO_H
