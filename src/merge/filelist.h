/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   filelist_t

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/file_types.h"
#include "merge/output_control.h"
#include "merge/probe_range_info.h"

class generic_reader_c;
class track_info_c;

struct filelist_t {
  struct deferred_connection_t {
    append_spec_t amap;
    packetizer_t *ptzr;
  };

  std::string name;
  std::vector<std::string> all_names;
  int64_t size{};
  size_t id{};

  packet_cptr pack;

  std::unique_ptr<generic_reader_c> reader;

  std::unique_ptr<track_info_c> ti;
  bool appending{}, appended_to{}, done{};

  int num_unfinished_packetizers{}, old_num_unfinished_packetizers{};
  std::vector<deferred_connection_t> deferred_connections;
  int64_t deferred_max_timestamp_seen{-1};

  bool is_playlist{};
  std::vector<generic_reader_c *> playlist_readers;
  size_t playlist_index{}, playlist_previous_filelist_id{};
  mm_mpls_multi_file_io_cptr playlist_mpls_in;

  timestamp_c restricted_timestamp_min, restricted_timestamp_max;

  probe_range_info_t probe_range_info{};

  filelist_t()
  {
  }
};
using filelist_cptr = std::shared_ptr<filelist_t>;

extern std::vector<filelist_cptr> g_files;
