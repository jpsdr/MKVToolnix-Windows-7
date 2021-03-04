/*
   hevc_dump - A tool for dumping HEVC structures

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include "common/command_line.h"
#include "common/hevc_es_parser.h"
#include "common/mm_io_x.h"
#include "common/mm_file_io.h"
#include "common/version.h"

static bool s_is_framed{};

static void
show_help() {
  mxinfo("hevc_dump [options] input_file_name\n"
         "\n"
         "General options:\n"
         "\n"
         "  -f, --framed   Source is NALUs-with-size-fields as found in MP4 & Matroska\n"
         "                 files instead of an ITU-T H.265 Annex B bitstream.\n"
         "  -h, --help     This help text\n"
         "  -V, --version  Print version information\n");
  mxexit();
}

static void
show_version() {
  mxinfo(get_version_info("hevc_dump") + "\n");
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

    else if ((arg == "-f") || (arg == "--framed"))
      s_is_framed = true;

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
  mm_file_io_c in{file_name};
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

    if (marker == mtx::hevc::NALU_START_CODE)
      marker_size = 4;

    else if ((marker & 0x00ffffff) == mtx::hevc::NALU_START_CODE)
      marker_size = 3;

    else {
      marker = (marker << 8) | in.read_uint8();
      continue;
    }

    pos -= marker_size;

    if (-1 != previous_pos)
      mxinfo(fmt::format("NALU type 0x{0:02x} ({1}) size {2} marker size {3} at {4}\n",
                         previous_type, mtx::hevc::es_parser_c::get_nalu_type_name(previous_type), pos - previous_pos - previous_marker_size, previous_marker_size, previous_pos));

    previous_pos         = pos;
    previous_marker_size = marker_size;
    previous_type        = (in.read_uint8() >> 1) & 0x3f;
    marker               = (1ull << 24) | in.read_uint24_be();
  }
}

static void
parse_file_framed(std::string const &file_name) {
  mm_file_io_c in{file_name};
  auto file_size = static_cast<uint64_t>(in.get_size());

  uint64_t pos{};

  while ((pos + 5) < file_size) {
    auto nalu_size = in.read_uint32_be();
    auto nalu_type = (in.read_uint8() >> 1) & 0x3f;

    if (!nalu_size)
      return;

    mxinfo(fmt::format("NALU type 0x{0:02x} ({1}) size {2} at {3}\n", nalu_type, mtx::hevc::es_parser_c::get_nalu_type_name(nalu_type), nalu_size, pos));

    pos += nalu_size + 4;

    in.setFilePointer(pos);
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
    if (s_is_framed)
      parse_file_framed(file_name);
    else
      parse_file(file_name);

  } catch (mtx::mm_io::exception &) {
    mxerror(Y("File not found\n"));
  }

  mxexit();
}
