/*
   bluray_dump - A tool for dumping various files found on Blu-rays

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include <QRegularExpression>

#include "common/bluray/clpi.h"
#include "common/bluray/disc_library.h"
#include "common/bluray/index.h"
#include "common/bluray/mpls.h"
#include "common/bluray/track_chapter_names.h"
#include "common/command_line.h"
#include "common/mm_file_io.h"
#include "common/qt.h"
#include "common/version.h"

static char const * const s_program_name = "bluray_dump";

static void
setup(char const *argv0) {
  mtx_common_init(s_program_name, argv0);

  mtx::cli::g_usage_text = fmt::format("{0} [options] input_file_name\n"
                                       "\n"
                                       "General options:\n"
                                       "\n"
                                       "  -h, --help             This help text\n"
                                       "  -V, --version          Print version information",
                                       s_program_name);
}

static std::string
parse_args(std::vector<std::string> &args) {
  std::string file_name;

  for (auto & arg: args) {
    if (!file_name.empty())
      mxerror(Y("More than one source file given.\n"));

    else
      file_name = arg;
  }

  if (file_name.empty())
    mxerror(Y("No file name given\n"));

  return file_name;
}

static void
parse_clpi_file(std::string const &file_name) {
  auto parser = mtx::bluray::clpi::parser_c{file_name};

  if (!parser.parse())
    mxerror("CLPI file could not be parsed.\n");

  parser.dump();
}

static void
parse_index_file(std::string const &file_name) {
  mtx::bluray::index::parser_c parser{file_name};

  if (!parser.parse())
    mxerror("index.bdmv file could not be parsed.\n");

  parser.dump();
}

static void
parse_mpls_file(std::string const &file_name) {
  mm_file_io_c in{file_name};
  auto parser = mtx::bluray::mpls::parser_c{};

  if (!parser.parse(in))
    mxerror("MPLS file could not be parsed.\n");

  parser.dump();
}

static void
parse_disc_library_file(std::string const &file_name) {
  auto disc_library = mtx::bluray::disc_library::locate_and_parse(file_name);
  if (!disc_library)
    mxerror("Disc library could no be parsed.\n");

  disc_library->dump();
}

static void
parse_track_chapter_names_file(std::string const &file_name) {
  auto matches = QRegularExpression{"tnmt_[a-z]{3}_(.{5})\\.xml$"}.match(Q(file_name));
  if (!matches.hasMatch())
    mxerror("Could not parse tnmt file name.\n");

  auto names = mtx::bluray::track_chapter_names::locate_and_parse_for_title(file_name, to_utf8(matches.captured(1)));
  if (names.empty())
    mxerror("Track/chapter names could no be parsed.\n");

  mtx::bluray::track_chapter_names::dump(names);
}

static void
parse_file(std::string const &file_name) {
  if (Q(file_name).contains(QRegularExpression{"\\.clpi$"}))
    parse_clpi_file(file_name);

  else if (Q(file_name).contains(QRegularExpression{"\\.mpls$"}))
    parse_mpls_file(file_name);

  else if (Q(file_name).contains(QRegularExpression{"index\\.bdmv"}))
    parse_index_file(file_name);

  else if (Q(file_name).contains(QRegularExpression{"bdmt_[a-z]{3}\\.xml$"}))
    parse_disc_library_file(file_name);

  else if (Q(file_name).contains(QRegularExpression{"tnmt_[a-z]{3}_.{5}\\.xml$"}))
    parse_track_chapter_names_file(file_name);

  else
    mxerror("Unknown/unsupported file format\n");
}

int
main(int argc,
     char **argv) {
  setup(argv[0]);

  auto args = mtx::cli::args_in_utf8(argc, argv);
  while (mtx::cli::handle_common_args(args, "-r"))
    ;

  auto file_name = parse_args(args);

  try {
    parse_file(file_name);
  } catch (mtx::mm_io::exception &) {
    mxerror(Y("File not found\n"));
  }

  mxexit();
}
