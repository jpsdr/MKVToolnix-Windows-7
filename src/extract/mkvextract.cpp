/*
   mkvextract -- extract tracks from Matroska files into other files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   command line parsing, setup

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include <cassert>
#include <iostream>

#include "common/chapters/chapters.h"
#include "common/command_line.h"
#include "common/fs_sys_helpers.h"
#include "common/list_utils.h"
#include "common/mm_io_x.h"
#include "common/mm_proxy_io.h"
#include "common/mm_write_buffer_io.h"
#include "common/strings/parsing.h"
#include "common/translation.h"
#include "common/version.h"
#include "extract/extract_cli_parser.h"
#include "extract/mkvextract.h"

enum operation_mode_e {
  MODE_TRACKS,
  MODE_TAGS,
  MODE_ATTACHMENTS,
  MODE_CHAPTERS,
  MODE_CUESHEET,
  MODE_TIMESTAMPS_V2,
};

kax_analyzer_cptr
open_and_analyze(std::string const &file_name,
                 kax_analyzer_c::parse_mode_e parse_mode,
                 bool exit_on_error) {
  // open input file
  try {
    auto analyzer = std::make_shared<kax_analyzer_c>(file_name);
    auto ok       = analyzer
      ->set_parse_mode(parse_mode)
      .set_open_mode(libebml::MODE_READ)
      .set_throw_on_error(exit_on_error)
      .process();

    return ok ? analyzer : kax_analyzer_cptr{};

  } catch (mtx::mm_io::exception &ex) {
    show_error(fmt::format(FY("The file '{0}' could not be opened for reading: {1}.\n"), file_name, ex));
    return {};

  } catch (mtx::kax_analyzer_x &ex) {
    show_error(fmt::format(FY("The file '{0}' could not be opened for reading: {1}.\n"), file_name, ex));
    return {};

  } catch (...) {
    show_error(Y("This file could not be opened or parsed.\n"));
    return {};

  }
}

mm_io_cptr
open_output_file(std::string const &file_name) {
  if (mtx::included_in(file_name, "", "-"))
    return g_mm_stdio;

  try {
    return mm_write_buffer_io_c::open(file_name, 128 * 1024);

  } catch (mtx::mm_io::exception &ex) {
    mxerror(fmt::format(FY("The file '{0}' could not be opened for writing: {1}.\n"), file_name, ex));
  }

  // Shut up the compiler.
  return {};
}

void
show_element(libebml::EbmlElement *l,
             int level,
             const std::string &info) {
  if (9 < level)
    mxerror(fmt::format(FY("mkvextract.cpp/show_element(): level > 9: {0}"), level));

  if (0 == verbose)
    return;

  char level_buffer[10];
  memset(&level_buffer[1], ' ', 9);
  level_buffer[0]     = '|';
  level_buffer[level] = 0;

  mxinfo(fmt::format("(mkvextract) {0}+ {1}", level_buffer, info));
  if (l)
    mxinfo(fmt::format(FY(" at {0}"), l->GetElementPosition()));
  mxinfo("\n");
}

void
show_error(const std::string &error) {
  mxerror(fmt::format("(mkvextract) {0}\n", error));
}

static void
setup(char **argv) {
  mtx_common_init("mkvextract", argv[0]);

  mtx::sys::set_process_priority(-1);

  mtx::bcp47::language_c::set_normalization_mode(mtx::bcp47::normalization_mode_e::none);

  verbose = 0;
}

int
main(int argc,
     char **argv) {
  setup(argv);

  options_c options = extract_cli_parser_c(mtx::cli::args_in_utf8(argc, argv)).run();
  auto first_mode   = options.m_modes.back().m_extraction_mode;

  if (!mtx::included_in(first_mode, options_c::em_tracks, options_c::em_tags, options_c::em_attachments, options_c::em_chapters, options_c::em_cues, options_c::em_cuesheet, options_c::em_timestamps_v2))
    mtx::cli::display_usage(2);

  auto analyzer       = open_and_analyze(options.m_file_name, options.m_parse_mode);
  auto done_something = false;

  for (auto &mode_options : options.m_modes) {
    auto done_something_here = false;

    if (options_c::em_tracks == mode_options.m_extraction_mode)
      done_something_here = extract_tracks(*analyzer, mode_options);

    else if (options_c::em_tags == mode_options.m_extraction_mode)
      done_something_here = extract_tags(*analyzer, mode_options);

    else if (options_c::em_attachments == mode_options.m_extraction_mode)
     done_something_here = extract_attachments(*analyzer, mode_options);

    else if (options_c::em_chapters == mode_options.m_extraction_mode)
      done_something_here = extract_chapters(*analyzer, mode_options);

    else if (options_c::em_cues == mode_options.m_extraction_mode)
      done_something_here = extract_cues(*analyzer, mode_options);

    else if (options_c::em_cuesheet == mode_options.m_extraction_mode)
      done_something_here = extract_cuesheet(*analyzer, mode_options);

    if (done_something_here)
      done_something = true;
  }

  if (!done_something)
    mxerror(Y("Nothing to do.\n"));

  mxexit();
}
