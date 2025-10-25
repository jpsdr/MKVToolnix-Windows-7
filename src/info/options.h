/*
   mkvinfo -- info tracks from Matroska files into other files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

class options_c {
public:
  std::string m_file_name;
  bool m_calc_checksums{}, m_continue_at_cluster{}, m_show_summary{}, m_show_hexdump{}, m_show_size{}, m_show_track_info{}, m_show_all_elements{};
  int m_hexdump_max_size{16}, m_verbose{};
  std::optional<bool> m_hex_positions;
};
