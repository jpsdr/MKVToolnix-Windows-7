/*
   hevc_dump - A tool for dumping HEVC structures

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include "common/command_line.h"
#include "common/hevc_es_parser.h"
#include "common/mm_io_x.h"

static void
show_help() {
  mxinfo("hevc_dump [options] input_file_name\n"
         "\n"
         "General options:\n"
         "\n"
         "  -h, --help             This help text\n"
         "  -V, --version          Print version information\n");
  mxexit();
}

static void
show_version() {
  mxinfo("hevc_dump v" PACKAGE_VERSION "\n");
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
      mxerror(Y("More than one source file was given.\n"));

    else
      file_name = arg;
  }

  if (file_name.empty())
    mxerror(Y("No file name given\n"));

  return file_name;
}

static void
parse_file(const std::string &file_name) {
  auto in   = mm_file_io_c{file_name};
  auto size = static_cast<uint64_t>(in.get_size());
  if (4 > size)
    return;

  auto marker               = static_cast<uint32_t>((1ull << 24) | in.read_uint24_be());
  auto marker_size          = 0;
  auto previous_marker_size = 0;
  auto previous_pos         = static_cast<int64_t>(-1);
  auto previous_type        = 0;

  while (true) {
    auto pos = in.getFilePointer();
    if (pos >= size)
      break;

    if (marker == NALU_START_CODE)
      marker_size = 4;

    else if ((marker & 0x00ffffff) == NALU_START_CODE)
      marker_size = 3;

    else {
      marker = (marker << 8) | in.read_uint8();
      continue;
    }

    pos -= marker_size;

    if (-1 != previous_pos)
      mxinfo(boost::format("NALU type 0x%|1$02x| (%2%) size %3% marker size %4% at %5%\n")
             % previous_type % mtx::hevc::es_parser_c::get_nalu_type_name(previous_type) % (pos - previous_pos - previous_marker_size) % previous_marker_size % previous_pos);

    previous_pos         = pos;
    previous_marker_size = marker_size;
    previous_type        = (in.read_uint8() >> 1) & 0x3f;
    marker               = (1ull << 24) | in.read_uint24_be();
  }
}

int
main(int argc,
     char **argv) {
  mtx_common_init("hevc_dump", argv[0]);

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
