/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   filelist_t

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#ifndef MTX_MERGE_FILELIST_H
#define MTX_MERGE_FILELIST_H

#include "common/common_pch.h"

#include "common/file_types.h"
#include "common/mm_mpls_multi_file_io_fwd.h"
#include "merge/output_control.h"

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

  file_type_e type{FILE_TYPE_IS_UNKNOWN};

  packet_cptr pack;

  std::unique_ptr<generic_reader_c> reader;

  std::unique_ptr<track_info_c> ti;
  bool appending{}, appended_to{}, done{};

  int num_unfinished_packetizers{}, old_num_unfinished_packetizers{};
  std::vector<deferred_connection_t> deferred_connections;
  int64_t deferred_max_timecode_seen{-1};

  bool is_playlist{};
  std::vector<generic_reader_c *> playlist_readers;
  size_t playlist_index{}, playlist_previous_filelist_id{};
  mm_mpls_multi_file_io_cptr playlist_mpls_in;

  timestamp_c restricted_timecode_min, restricted_timecode_max;

  filelist_t()
  {
  }
};
using filelist_cptr = std::shared_ptr<filelist_t>;

extern std::vector<filelist_cptr> g_files;

#endif  // MTX_MERGE_FILELIST_H
