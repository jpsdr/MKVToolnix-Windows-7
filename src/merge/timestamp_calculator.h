/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   class definition for the timecode calculator

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#ifndef MTX_MERGE_TIMESTAMP_CALCULATOR_H
#define MTX_MERGE_TIMESTAMP_CALCULATOR_H

#include "common/common_pch.h"

#include "common/debugging.h"
#include "common/samples_to_timestamp_converter.h"
#include "common/timestamp.h"

class packet_t;
using packet_cptr = std::shared_ptr<packet_t>;

class timestamp_calculator_c {
private:
  std::deque<timestamp_c> m_available_timestamps;
  timestamp_c m_reference_timestamp, m_last_timestamp_returned;
  int64_t m_samples_per_second, m_samples_since_reference_timestamp;
  samples_to_timestamp_converter_c m_samples_to_timestamp;
  debugging_option_c m_debug;

public:
  timestamp_calculator_c(int64_t samples_per_second);

  void add_timestamp(timestamp_c const &timestamp);
  void add_timestamp(int64_t timestamp);
  void add_timestamp(packet_cptr const &packet);

  timestamp_c get_next_timestamp(int64_t samples_in_frame);
  timestamp_c get_duration(int64_t samples);

  void set_samples_per_second(int64_t samples_per_second);
};

#endif  // MTX_MERGE_TIMESTAMP_CALCULATOR_H
