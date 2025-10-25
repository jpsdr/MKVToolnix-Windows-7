/*
   mkvinfo -- utility for gathering information about Matroska files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   retrieves and displays information about a Matroska file

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/bcp47.h"
#include "common/command_line.h"
#include "common/fs_sys_helpers.h"
#include "common/kax_info.h"
#include "common/version.h"
#include "info/info_cli_parser.h"

void
setup(char const *argv0) {
  mtx_common_init("mkvinfo", argv0);

  mtx::bcp47::language_c::set_normalization_mode(mtx::bcp47::normalization_mode_e::none);
}

int
main(int argc,
     char **argv) {
  setup(argv[0]);

  auto options = info_cli_parser_c(mtx::cli::args_in_utf8(argc, argv)).run();

  mtx::sys::set_process_priority(-1);

  if (options.m_file_name.empty())
    mxerror(Y("No file name given.\n"));

  mtx::kax_info_c info;

  info.set_show_all_elements(  (2 <= options.m_verbose) || options.m_show_all_elements);
  info.set_show_positions(     (2 <= options.m_verbose) || options.m_hex_positions);
  info.set_continue_at_cluster((1 <= options.m_verbose) || options.m_continue_at_cluster || options.m_show_all_elements);

  info.set_calc_checksums(options.m_calc_checksums);
  info.set_show_summary(options.m_show_summary);
  info.set_show_hexdump(options.m_show_hexdump);
  info.set_show_size(options.m_show_size);
  info.set_show_track_info(options.m_show_track_info);
  info.set_hexdump_max_size(options.m_hexdump_max_size);

  if (options.m_hex_positions)
    info.set_hex_positions(*options.m_hex_positions);

  try {
    info.open_and_process_file(options.m_file_name);
  } catch (mtx::kax_info::exception &ex) {
    mxinfo(ex.what());
    mxexit(2);
  }

  mxexit();

  return 0;
}
