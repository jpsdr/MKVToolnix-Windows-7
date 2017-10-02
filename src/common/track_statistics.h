/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   The cluster helper groups frames into blocks groups and those
   into clusters, sets the durations, renders the clusters etc.

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/optional.hpp>

namespace libmatroska {
class KaxTags;
}

class track_statistics_c {
private:
  boost::optional<int64_t> m_min_timestamp, m_max_timestamp_and_duration;
  uint64_t m_num_bytes{}, m_num_frames{}, m_track_uid{};

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

  bool is_valid() const {
    return m_min_timestamp && m_max_timestamp_and_duration;
  }

  uint64_t get_num_bytes() const {
    return m_num_bytes;
  }

  uint64_t get_num_frames() const {
    return m_num_frames;
  }

  boost::optional<int64_t> get_duration() const {
    return is_valid() ? *m_max_timestamp_and_duration - *m_min_timestamp : boost::optional<int64_t>{};
  }

  boost::optional<int64_t> get_bits_per_second() const {
    auto duration = get_duration();
    return duration && (*duration != 0) ? ((m_num_bytes * 8000) / (*duration / 1000000)) : boost::optional<int64_t>{};
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
    return (boost::format("<#b:%1% #f:%2% min:%3% max:%4% dur:%5% bps:%6%>")
            % m_num_bytes
            % m_num_frames
            % (m_min_timestamp              ? *m_min_timestamp              : -1)
            % (m_max_timestamp_and_duration ? *m_max_timestamp_and_duration : -1)
            % (duration                     ? *duration                     : -1)
            % (bps                          ? *bps                          : -1)).str();
  }

  void create_tags(KaxTags &tags, std::string const &writing_app, boost::posix_time::ptime const &writing_date) const;
};
