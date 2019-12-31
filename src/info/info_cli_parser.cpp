/** \brief command line parsing

   mkvinfo -- info tracks from Matroska files into other files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   \file

   \author Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include "common/ebml.h"
#include "common/strings/formatting.h"
#include "common/strings/parsing.h"
#include "common/translation.h"
#include "info/info_cli_parser.h"
#include "info/options.h"

info_cli_parser_c::info_cli_parser_c(const std::vector<std::string> &args)
  : mtx::cli::parser_c{args}
{
  verbose = 0;
}

#define OPT(spec, func, description) add_option(spec, std::bind(&info_cli_parser_c::func, this), description)

void
info_cli_parser_c::init_parser() {
  add_information(YT("mkvinfo [options] <inname>"));

  add_section_header(YT("Options"));

  OPT("a|all",           set_show_all_elements,   YT("Show all sub-elements (including cues & seek heads entries) and don't stop at the first cluster."));
  OPT("c|checksum",      set_checksum,            YT("Calculate and display checksums of frame contents."));
  OPT("C|check-mode",    set_check_mode,          YT("Calculate and display checksums and use verbosity level 4."));
  OPT("o|continue",      set_continue_at_cluster, YT("Don't stop processing at the first cluster."));
  OPT("P|positions",     set_dec_positions,       YT("Show the position of each element in decimal."));
  OPT("p|hex-positions", set_hex_positions,       YT("Show the position of each element in hexadecimal."));
  OPT("s|summary",       set_summary,             YT("Only show summaries of the contents, not each element."));
  OPT("t|track-info",    set_track_info,          YT("Show statistics for each track in verbose mode."));
  OPT("x|hexdump",       set_hexdump,             YT("Show the first 16 bytes of each frame as a hex dump."));
  OPT("X|full-hexdump",  set_full_hexdump,        YT("Show all bytes of each frame as a hex dump."));
  OPT("z|size",          set_size,                YT("Show the size of each element including its header."));

  add_common_options();

  add_hook(mtx::cli::parser_c::ht_unknown_option, std::bind(&info_cli_parser_c::set_file_name, this));
}

#undef OPT

void
info_cli_parser_c::set_show_all_elements() {
  m_options.m_show_all_elements = true;
}

void
info_cli_parser_c::set_checksum() {
  m_options.m_calc_checksums = true;
}

void
info_cli_parser_c::set_check_mode() {
  m_options.m_calc_checksums = true;
  verbose                    = 4;
}

void
info_cli_parser_c::set_continue_at_cluster() {
  m_options.m_continue_at_cluster = true;
}

void
info_cli_parser_c::set_summary() {
  m_options.m_calc_checksums = true;
  m_options.m_show_summary   = true;
}


void
info_cli_parser_c::set_hexdump() {
  m_options.m_show_hexdump = true;
}

void
info_cli_parser_c::set_full_hexdump() {
  m_options.m_show_hexdump     = true;
  m_options.m_hexdump_max_size = INT_MAX;
}

void
info_cli_parser_c::set_size() {
  m_options.m_show_size = true;
}

void
info_cli_parser_c::set_track_info() {
  m_options.m_show_track_info     = true;
  m_options.m_continue_at_cluster = true;
}

void
info_cli_parser_c::set_file_name() {
  if (!m_options.m_file_name.empty())
    mxerror(Y("Only one source file is allowed.\n"));

  m_options.m_file_name = m_current_arg;
}

void
info_cli_parser_c::set_dec_positions() {
  m_options.m_hex_positions = false;
}

void
info_cli_parser_c::set_hex_positions() {
  m_options.m_hex_positions = true;
}

options_c
info_cli_parser_c::run() {
  init_parser();
  parse_args();

  m_options.m_verbose = verbose;
  verbose             = 0;

  return m_options;
}
