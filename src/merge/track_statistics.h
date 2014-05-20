/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   The cluster helper groups frames into blocks groups and those
   into clusters, sets the durations, renders the clusters etc.

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#ifndef MTX_MERGE_TRACK_STATISTICS_H
#define MTX_MERGE_TRACK_STATISTICS_H

#include "common/common_pch.h"

#include <boost/optional.hpp>

class track_statistics_c {
private:
  boost::optional<int64_t> m_min_timecode, m_max_timecode_and_duration;
  uint64_t m_num_bytes, m_num_frames;

public:
  track_statistics_c()
    : m_min_timecode{}
    , m_max_timecode_and_duration{}
    , m_num_bytes{}
    , m_num_frames{}
  {
  }

  void reset() {
    *this = track_statistics_c{};
  }

  bool is_valid() const {
    return m_min_timecode && m_max_timecode_and_duration;
  }

  uint64_t get_num_bytes() const {
    return m_num_bytes;
  }

  uint64_t get_num_frames() const {
    return m_num_frames;
  }

  boost::optional<int64_t> get_duration() const {
    return is_valid() ? *m_max_timecode_and_duration - *m_min_timecode : boost::optional<int64_t>{};
  }

  boost::optional<int64_t> get_bits_per_second() const {
    auto duration = get_duration();
    return duration && (*duration != 0) ? ((m_num_bytes * 8000) / (*duration / 1000000)) : boost::optional<int64_t>{};
  }

  void process(packet_t const &pack) {
    m_num_frames++;
    m_num_bytes                 += pack.data->get_size();
    m_min_timecode               = std::min(pack.assigned_timecode,                       m_min_timecode              ? *m_min_timecode              : std::numeric_limits<int64_t>::max());
    m_max_timecode_and_duration  = std::max(pack.assigned_timecode + pack.get_duration(), m_max_timecode_and_duration ? *m_max_timecode_and_duration : std::numeric_limits<int64_t>::min());
  }

  std::string to_string() const {
    auto duration = get_duration();
    auto bps      = get_bits_per_second();
    return (boost::format("<#b:%1% #f:%2% min:%3% max:%4% dur:%5% bps:%6%>")
            % m_num_bytes
            % m_num_frames
            % (m_min_timecode              ? *m_min_timecode              : -1)
            % (m_max_timecode_and_duration ? *m_max_timecode_and_duration : -1)
            % (duration                    ? *duration                    : -1)
            % (bps                         ? *bps                         : -1)).str();
  }
};

#endif // MTX_MERGE_TRACK_STATISTICS_H
