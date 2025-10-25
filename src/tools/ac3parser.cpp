/*
   ac3parser - A simple tool for parsing (E-)AC-3 files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/ac3.h"
#include "common/byte_buffer.h"
#include "common/checksums/base.h"
#include "common/command_line.h"
#include "common/endian.h"
#include "common/mm_io_x.h"
#include "common/mm_file_io.h"
#include "common/translation.h"
#include "common/version.h"

static bool g_opt_checksum      = false;
static bool g_opt_frame_headers = false;

static void
setup_help() {
  mtx::cli::g_usage_text = "ac3parser [options] input_file_name\n"
                           "\n"
                           "Options for output and information control:\n"
                           "\n"
                           "  -c, --checksum         Calculate and output checksums of each unit\n"
                           "  -f, --frame-headers    Show the content of frame headers\n"
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
    if ((arg == "-c") || (arg == "--checksum"))
      g_opt_checksum = true;

    else if ((arg == "-f") || (arg == "--frame-headers"))
      g_opt_frame_headers = true;

    else if (!file_name.empty())
      mxerror(Y("More than one source was file given.\n"));

    else
      file_name = arg;
  }

  if (file_name.empty())
    mxerror(Y("No file name given\n"));

  return file_name;
}

static void
parse_file(const std::string &file_name) {
  mm_file_io_c in(file_name);

  const size_t buf_size = 100000;
  int64_t size          = in.get_size();

  if (4 > size)
    mxerror(Y("File too small\n"));

  auto mem = memory_c::alloc(buf_size);
  auto ptr = mem->get_buffer();

  mtx::ac3::parser_c parser;

  size_t num_read;
  do {
    num_read = in.read(ptr, buf_size);

    parser.add_bytes(ptr, num_read);
    if (num_read < buf_size)
      parser.flush();

    while (parser.frame_available()) {
      auto frame  = parser.get_frame();
      auto output = frame.to_string(g_opt_frame_headers);

      if (g_opt_checksum) {
        uint32_t adler32  = mtx::checksum::calculate_as_uint(mtx::checksum::algorithm_e::adler32, *frame.m_data);
        output           += fmt::format(" checksum 0x{0:08x}", adler32);
      }

      mxinfo(fmt::format("{0}\n", output));
    }

  } while (num_read == buf_size);
}

int
main(int argc,
     char **argv) {
  mtx_common_init("ac3parser", argv[0]);
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
