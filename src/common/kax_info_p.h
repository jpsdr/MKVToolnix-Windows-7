/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   retrieves and displays information about a Matroska file

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

namespace mtx::kax_info {

struct track_t {
  uint64_t tnum{}, tuid{};
  char type{' '};
  int64_t default_duration{};
  std::size_t mkvmerge_track_id{};
  std::string codec_id, fourcc;
};

struct track_info_t {
  int64_t m_size{}, m_blocks{}, m_blocks_by_ref_num[3]{0, 0, 0}, m_add_duration_for_n_packets{};
  std::optional<int64_t> m_min_timestamp, m_max_timestamp;
};

class private_c {
public:
  std::vector<std::shared_ptr<track_t>> m_tracks;
  std::unordered_map<unsigned int, std::shared_ptr<track_t>> m_tracks_by_number;
  std::unordered_map<unsigned int, track_info_t> m_track_info;
  std::vector<std::shared_ptr<libebml::EbmlElement>> m_retained_elements;
  std::unordered_map<libebml::EbmlElement *, std::shared_ptr<track_t>> m_track_by_element;
  uint64_t m_ts_scale{TIMESTAMP_SCALE}, m_file_size{};
  std::size_t m_mkvmerge_track_id{};
  std::shared_ptr<libebml::EbmlStream> m_es;
  mm_io_cptr m_in, m_out{g_mm_stdio};
  std::string m_source_file_name, m_destination_file_name;
  int m_level{};
  std::vector<std::string> m_summary;
  std::shared_ptr<track_t> m_track;
  libmatroska::KaxCluster *m_cluster{};
  std::vector<int> m_frame_sizes;
  std::vector<uint32_t> m_frame_adlers;
  std::vector<std::string> m_frame_hexdumps;
  int64_t m_num_references{}, m_lf_timestamp{}, m_lf_tnum{};
  std::optional<int64_t> m_block_duration;
  std::optional<uint64_t> m_block_add_id_type;
  memory_cptr m_block_add_id_extra_data;

  bool m_use_gui{}, m_calc_checksums{}, m_show_summary{}, m_show_hexdump{}, m_show_size{}, m_show_positions{}, m_show_track_info{}, m_hex_positions{}, m_retain_elements{}, m_continue_at_cluster{}, m_show_all_elements{};
  int m_hexdump_max_size{};

  bool m_abort{};

  std::unordered_map<uint32_t, std::function<std::string(libebml::EbmlElement &)>> m_custom_element_value_formatters;
  std::unordered_map<uint32_t, std::function<bool(libebml::EbmlElement &)>> m_custom_element_pre_processors;
  std::unordered_map<uint32_t, std::function<void(libebml::EbmlElement &)>> m_custom_element_post_processors;

public:
  private_c() = default;
  virtual ~private_c() = default;
};

}
