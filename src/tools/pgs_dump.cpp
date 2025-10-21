/*
   pgs_dump - A tool for dumping HEVC structures

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/checksums/base_fwd.h"
#include "common/command_line.h"
#include "common/hdmv_pgs.h"
#include "common/mm_io_x.h"
#include "common/mm_file_io.h"
#include "common/strings/formatting.h"
#include "common/timestamp.h"
#include "common/version.h"

static void
setup_help() {
  mtx::cli::g_usage_text = "pgs_dump [options] input_file_name\n"
                           "\n"
                           "General options:\n"
                           "\n"
                           "  -h, --help             This help text\n"
                           "  -V, --version          Print version information\n";
}

static std::string
parse_args(std::vector<std::string> &args) {
  std::string file_name;

  for (auto & arg: args) {
    if (!file_name.empty())
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
  mm_file_io_c in{file_name};
  auto file_size = static_cast<uint64_t>(in.get_size());

  while (in.getFilePointer() < file_size) {
    auto position = in.getFilePointer();
    auto magic    = in.read(2);

    if (magic->to_string() != "PG")
      mxerror(fmt::format("position {}: magic string not found\n", position));

    auto timestamp = timestamp_c::mpeg(in.read_uint32_be());

    in.skip(4);

    auto type         = in.read_uint8();
    auto segment_size = in.read_uint16_be();
    auto segment_data = in.read(segment_size);

    if (!segment_data || segment_data->get_size() < segment_size)
      mxerror(fmt::format("position {}: file truncated\n", position));

    auto adler32 = mtx::checksum::calculate_as_uint(mtx::checksum::algorithm_e::adler32, *segment_data);

    mxinfo(fmt::format("segment at {0:9d} size {1:5d} timestamp {2} checksum 0x{3:08x} type {4:3d} ({5})\n", position, segment_size, mtx::string::format_timestamp(timestamp), adler32, type, mtx::hdmv_pgs::name_for_type(type)));
  }
}

int
main(int argc,
     char **argv) {
  mtx_common_init("pgs_dump", argv[0]);
  setup_help();

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
