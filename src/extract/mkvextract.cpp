/*
   mkvextract -- extract tracks from Matroska files into other files

   Distributed under the GPL
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   command line parsing, setup

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include <cassert>
#include <iostream>
#include <stdlib.h>

#include "common/chapters/chapters.h"
#include "common/command_line.h"
#include "common/mm_io.h"
#include "common/mm_io_x.h"
#include "common/strings/parsing.h"
#include "common/translation.h"
#include "common/version.h"
#include "extract/extract_cli_parser.h"
#include "extract/mkvextract.h"

using namespace libmatroska;

#define NAME "mkvextract"

enum operation_mode_e {
  MODE_TRACKS,
  MODE_TAGS,
  MODE_ATTACHMENTS,
  MODE_CHAPTERS,
  MODE_CUESHEET,
  MODE_TIMECODES_V2,
};

kax_analyzer_cptr
open_and_analyze(std::string const &file_name,
                 kax_analyzer_c::parse_mode_e parse_mode,
                 bool exit_on_error) {
  // open input file
  try {
    auto analyzer = std::make_shared<kax_analyzer_c>(file_name);
    auto ok       = analyzer->process(parse_mode, MODE_READ, exit_on_error);

    return ok ? analyzer : kax_analyzer_cptr{};

  } catch (mtx::mm_io::exception &ex) {
    show_error(boost::format(Y("The file '%1%' could not be opened for reading: %2%.\n")) % file_name % ex);
    return {};

  } catch (mtx::kax_analyzer_x &ex) {
    show_error(boost::format(Y("The file '%1%' could not be opened for reading: %2%.\n")) % file_name % ex);
    return {};

  } catch (...) {
    show_error(Y("This file could not be opened or parsed.\n"));
    return {};

  }
}

void
show_element(EbmlElement *l,
             int level,
             const std::string &info) {
  if (9 < level)
    mxerror(boost::format(Y("mkvextract.cpp/show_element(): level > 9: %1%")) % level);

  if (0 == verbose)
    return;

  char level_buffer[10];
  memset(&level_buffer[1], ' ', 9);
  level_buffer[0]     = '|';
  level_buffer[level] = 0;

  mxinfo(boost::format("(%1%) %2%+ %3%") % NAME % level_buffer % info);
  if (l)
    mxinfo(boost::format(Y(" at %1%")) % l->GetElementPosition());
  mxinfo("\n");
}

void
show_error(const std::string &error) {
  mxerror(boost::format("(%1%) %2%\n") % NAME % error);
}

static void
setup(char **argv) {
  mtx_common_init("mkvextract", argv[0]);

  set_process_priority(-1);

  verbose      = 0;
  version_info = get_version_info("mkvextract", vif_full);
}

int
main(int argc,
     char **argv) {
  setup(argv);

  options_c options = extract_cli_parser_c(command_line_utf8(argc, argv)).run();

  if (options_c::em_tracks == options.m_extraction_mode) {
    extract_tracks(options.m_file_name, options.m_tracks, options.m_parse_mode);

    if (0 == verbose)
      mxinfo(Y("Progress: 100%\n"));

  } else if (options_c::em_tags == options.m_extraction_mode)
    extract_tags(options.m_file_name, options.m_parse_mode);

  else if (options_c::em_attachments == options.m_extraction_mode)
    extract_attachments(options.m_file_name, options.m_tracks, options.m_parse_mode);

  else if (options_c::em_chapters == options.m_extraction_mode)
    extract_chapters(options.m_file_name, options.m_simple_chapter_format, options.m_parse_mode);

  else if (options_c::em_cuesheet == options.m_extraction_mode)
    extract_cuesheet(options.m_file_name, options.m_parse_mode);

  else if (options_c::em_timecodes_v2 == options.m_extraction_mode)
    extract_timecodes(options.m_file_name, options.m_tracks, 2);

  else
    usage(2);

  mxexit(0);
}
