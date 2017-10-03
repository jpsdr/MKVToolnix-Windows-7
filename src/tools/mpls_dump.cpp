/*
   mpls_dump - A tool for dumping MPLS structures

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include "common/command_line.h"
#include "common/mpls.h"

static void
show_help() {
  mxinfo("mpls_dump [options] input_file_name\n"
         "\n"
         "General options:\n"
         "\n"
         "  -h, --help             This help text\n"
         "  -V, --version          Print version information\n");
  mxexit();
}

static void
show_version() {
  mxinfo("mpls_dump v" PACKAGE_VERSION "\n");
  mxexit();
}

static std::string
parse_args(std::vector<std::string> &args) {
  std::string file_name;

  for (auto & arg: args) {
    if ((arg == "-h") || (arg == "--help"))
      show_help();

    else if ((arg == "-V") || (arg == "--version"))
      show_version();

    else if (!file_name.empty())
      mxerror(Y("More than one source file given.\n"));

    else
      file_name = arg;
  }

  if (file_name.empty())
    mxerror(Y("No file name given\n"));

  return file_name;
}

static void
parse_file(const std::string &file_name) {
  auto in     = mm_file_io_c{file_name};
  auto parser = mtx::bluray::mpls::parser_c{};

  if (!parser.parse(&in))
    mxerror("MPLS file could not be parsed.\n");

  parser.dump();
}

int
main(int argc,
     char **argv) {
  mtx_common_init("mpls_dump", argv[0]);

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
