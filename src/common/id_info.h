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

namespace mtx { namespace id {

char const * const aac_is_sbr                      = "aac_is_sbr";
char const * const audio_bits_per_sample           = "audio_bits_per_sample";
char const * const audio_channels                  = "audio_channels";
char const * const audio_output_sampling_frequency = "audio_output_sampling_frequency";
char const * const audio_sampling_frequency        = "audio_sampling_frequency";
char const * const codec_id                        = "codec_id";
char const * const codec_private_data              = "codec_private_data";
char const * const codec_private_length            = "codec_private_length";
char const * const content_encoding_algorithms     = "content_encoding_algorithms";
char const * const cropping                        = "cropping";
char const * const default_duration                = "default_duration";
char const * const default_track                   = "default_track";
char const * const display_dimensions              = "display_dimensions";
char const * const duration                        = "duration";
char const * const enabled_track                   = "enabled_track";
char const * const forced_track                    = "forced_track";
char const * const language                        = "language";
char const * const mpeg4_p10_es_video              = "mpeg4_p10_es_video";
char const * const mpeg4_p10_video                 = "mpeg4_p10_video";
char const * const mpegh_p2_es_video               = "mpegh_p2_es_video";
char const * const mpegh_p2_video                  = "mpegh_p2_video";
char const * const next_segment_uid                = "next_segment_uid";
char const * const number                          = "number";
char const * const other_file                      = "other_file";
char const * const packetizer                      = "packetizer";
char const * const pixel_dimensions                = "pixel_dimensions";
char const * const playlist                        = "playlist";
char const * const playlist_chapters               = "playlist_chapters";
char const * const playlist_duration               = "playlist_duration";
char const * const playlist_file                   = "playlist_file";
char const * const playlist_size                   = "playlist_size";
char const * const previous_segment_uid            = "previous_segment_uid";
char const * const segment_uid                     = "segment_uid";
char const * const stereo_mode                     = "stereo_mode";
char const * const stream_id                       = "stream_id";
char const * const sub_stream_id                   = "sub_stream_id";
char const * const text_subtitles                  = "text_subtitles";
char const * const title                           = "title";
char const * const track_name                      = "track_name";
char const * const ts_pid                          = "ts_pid";
char const * const uid                             = "uid";

class info_c {
protected:
  std::vector<std::string> m_info;

public:
  template<typename T> info_c &
  set(std::string const &key,
      T const &value) {
    m_info.emplace_back((boost::format("%1%:%2%") % escape(key) % escape((boost::format("%1%") % value).str())).str());
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

  std::vector<std::string> const &get() const {
    return m_info;
  }
};

}}

#endif // MTX_INPUT_ID_INFO_H
