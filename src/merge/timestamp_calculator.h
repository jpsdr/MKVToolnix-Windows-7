/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definition for the timestamp calculator

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/debugging.h"
#include "common/samples_to_timestamp_converter.h"
#include "common/timestamp.h"

class packet_t;
using packet_cptr = std::shared_ptr<packet_t>;

class timestamp_calculator_c {
private:
  std::deque<std::pair<timestamp_c, std::optional<uint64_t>>> m_available_timestamps;
  timestamp_c m_reference_timestamp, m_last_timestamp_returned;
  int64_t m_samples_per_second, m_samples_since_reference_timestamp;
  samples_to_timestamp_converter_c m_samples_to_timestamp;
  bool m_allow_smaller_timestamps;
  debugging_option_c m_debug;

public:
  timestamp_calculator_c(int64_t samples_per_second);

  void add_timestamp(std::optional<int64_t> const &timestamp, std::optional<uint64_t> stream_position = std::nullopt);
  void add_timestamp(timestamp_c const &timestamp, std::optional<uint64_t> stream_position = std::nullopt);
  void add_timestamp(int64_t timestamp, std::optional<uint64_t> stream_position = std::nullopt);
  void add_timestamp(packet_cptr const &packet, std::optional<uint64_t> stream_position = std::nullopt);

  void drop_timestamps_before_position(uint64_t stream_position);

  timestamp_c get_next_timestamp(int64_t samples_in_frame, std::optional<uint64_t> stream_position = std::nullopt);
  timestamp_c get_duration(int64_t samples);

  void set_samples_per_second(int64_t samples_per_second);
  void set_allow_smaller_timestamps(bool allow);

protected:
  timestamp_c fetch_next_available_timestamp(int64_t samples_in_frame);
  timestamp_c calculate_next_timestamp(int64_t samples_in_frame);
};
