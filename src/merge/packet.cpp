/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   packet structure implementation

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/track_statistics.h"
#include "merge/cluster_helper.h"
#include "merge/output_control.h"
#include "merge/packet.h"

void
packet_t::normalize_timestamps() {
  // Normalize the timestamps according to the timestamp scale.
  unmodified_assigned_timestamp = assigned_timestamp;
  unmodified_duration           = duration;
  timestamp                     = round_timestamp_scale(timestamp);
  assigned_timestamp            = round_timestamp_scale(assigned_timestamp);
  if (has_duration())
    duration                    = round_timestamp_scale(duration);
  if (has_bref())
    bref                        = round_timestamp_scale(bref);
  if (has_fref())
    fref                        = round_timestamp_scale(fref);
}

void
packet_t::add_extensions(std::vector<packet_extension_cptr> const &new_extensions) {
  std::copy(new_extensions.begin(), new_extensions.end(), std::back_inserter(extensions));
}

void
packet_t::account(track_statistics_c &statistics,
                  int64_t timestamp_offset) {
  statistics.account(assigned_timestamp - timestamp_offset, get_duration(), calculate_uncompressed_size());
}

uint64_t
packet_t::calculate_uncompressed_size() {
  if (!uncompressed_size) {
    uncompressed_size = data->get_size() + std::accumulate(data_adds.begin(), data_adds.end(), 0ull, [](auto const &sum, auto const &data_add) { return sum + data_add.data->get_size(); });
  }

  return *uncompressed_size;
}
