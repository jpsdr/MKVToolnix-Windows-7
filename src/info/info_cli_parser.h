/*
   mkvinfo -- info tracks from Matroska files into other files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/cli_parser.h"
#include "info/options.h"

class info_cli_parser_c: public mtx::cli::parser_c {
protected:
  options_c m_options;

public:
  info_cli_parser_c(const std::vector<std::string> &args);

  options_c run();

protected:
  void init_parser();

  void set_checksum();
  void set_check_mode();
  void set_continue_at_cluster();
  void set_summary();
  void set_hexdump();
  void set_full_hexdump();
  void set_size();
  void set_file_name();
  void set_track_info();
  void set_dec_positions();
  void set_hex_positions();
  void set_show_all_elements();
};
