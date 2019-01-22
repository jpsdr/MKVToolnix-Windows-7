/*
   bluray_dump - A tool for dumping various files found on Blu-rays

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include "common/bluray/clpi.h"
#include "common/bluray/index.h"
#include "common/bluray/mpls.h"
#include "common/command_line.h"
#include "common/mm_file_io.h"

static char const * const s_program_name = "bluray_dump";

static void
setup(char const *argv0) {
  mtx_common_init(s_program_name, argv0);

  mtx::cli::g_version_info = fmt::format("{0} v{1}", s_program_name, PACKAGE_VERSION);
  mtx::cli::g_usage_text   = fmt::format("{0} [options] input_file_name\n"
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
parse_file(std::string const &file_name) {
  if (boost::regex_search(file_name, boost::regex{"\\.clpi$", boost::regex::perl}))
    parse_clpi_file(file_name);

  else if (boost::regex_search(file_name, boost::regex{"\\.mpls$", boost::regex::perl}))
    parse_mpls_file(file_name);

  else if (boost::regex_search(file_name, boost::regex{"index\\.bdmv", boost::regex::perl}))
    parse_index_file(file_name);

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
