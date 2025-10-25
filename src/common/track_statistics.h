/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   The cluster helper groups frames into blocks groups and those
   into clusters, sets the durations, renders the clusters etc.

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include <QDateTime>

namespace libmatroska {
class KaxTags;
class KaxTag;
}

class track_statistics_c {
private:
  std::optional<int64_t> m_min_timestamp, m_max_timestamp_and_duration;
  uint64_t m_num_bytes{}, m_num_frames{}, m_track_uid{};
  std::string m_source_id;

public:
  track_statistics_c()
  {
  }

  track_statistics_c(uint64_t track_uid)
    : m_track_uid{track_uid}
  {
  }

  void reset() {
    *this = track_statistics_c{m_track_uid};
  }

  track_statistics_c &set_track_uid(uint64_t track_uid) {
    m_track_uid = track_uid;
    return *this;
  }

  track_statistics_c &set_source_id(std::string const &source_id) {
    m_source_id = source_id;
    return *this;
  }

  bool is_valid() const {
    return m_min_timestamp && m_max_timestamp_and_duration;
  }

  uint64_t get_num_bytes() const {
    return m_num_bytes;
  }

  uint64_t get_num_frames() const {
    return m_num_frames;
  }

  std::optional<int64_t> get_duration() const {
    return is_valid() ? *m_max_timestamp_and_duration - *m_min_timestamp : std::optional<int64_t>{};
  }

  std::optional<int64_t> get_bits_per_second() const {
    auto duration = get_duration();
    return duration && (1'000'000 < *duration) ? ((m_num_bytes * 8000) / (*duration / 1'000'000)) : std::optional<int64_t>{};
  }

  void account(int64_t timestamp, int64_t duration, uint64_t num_bytes) {
    ++m_num_frames;
    m_num_bytes                 += num_bytes;
    m_min_timestamp               = std::min(timestamp,            m_min_timestamp              ? *m_min_timestamp              : std::numeric_limits<int64_t>::max());
    m_max_timestamp_and_duration  = std::max(timestamp + duration, m_max_timestamp_and_duration ? *m_max_timestamp_and_duration : std::numeric_limits<int64_t>::min());
  }

  std::string to_string() const {
    auto duration = get_duration();
    auto bps      = get_bits_per_second();
    return fmt::format("<#b:{0} #f:{1} min:{2} max:{3} dur:{4} bps:{5}>",
                       m_num_bytes,
                       m_num_frames,
                       m_min_timestamp              ? *m_min_timestamp              : -1,
                       m_max_timestamp_and_duration ? *m_max_timestamp_and_duration : -1,
                       duration                     ? *duration                     : -1,
                       bps                          ? *bps                          : -1);
  }

  void create_tags(libmatroska::KaxTags &tags, std::string const &writing_app, std::optional<QDateTime> const &writing_date) const;

protected:
  libmatroska::KaxTag *find_or_create_tag(libmatroska::KaxTags &tags) const;
};
