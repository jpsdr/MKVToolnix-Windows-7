/*
   hevc_dump - A tool for dumping HEVC structures

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include "common/checksums/base_fwd.h"
#include "common/command_line.h"
#include "common/hevc/es_parser.h"
#include "common/mm_io_x.h"
#include "common/mm_file_io.h"

static bool s_is_framed{}, s_portable_format{};
static memory_cptr s_frame;
static uint64_t s_frame_fill{};
static std::unique_ptr<mtx::hevc::es_parser_c> s_parser;

static void
setup_help() {
  mtx::cli::g_usage_text = "hevc_dump [options] input_file_name\n"
    "\n"
    "General options:\n"
    "\n"
    "  -f, --framed           Source is ISO/IEC 14496-15 bitstream (NALUs prefixed\n"
    "                         with size field) instead of an ITU-T H.265 Annex B\n"
    "                         bitstream.\n"
    "  -p, --portable-format  Output a format that's comparable with e.g. 'diff'\n"
    "                         between the ISO/IEC 14496-15 bitstream and ITU-T H.265\n"
    "                         Annex B bitsream variants by not outputting the NALU's\n"
    "                         position (both modes) nor the marker size (Annex B\n"
    "                         mode).\n"
    "  -h, --help             This help text\n"
    "  -V, --version          Print version information\n";
}

static std::string
parse_args(std::vector<std::string> &args) {
  std::string file_name;

  for (auto & arg: args) {
    if ((arg == "-f") || (arg == "--framed"))
      s_is_framed = true;

    else if ((arg == "-p") || (arg == "--portable-format"))
      s_portable_format = true;

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
add_frame_byte(uint8_t byte) {
  if (!s_frame)
    s_frame = memory_c::alloc(100'000);

  else if (s_frame->get_size() == s_frame_fill)
    s_frame->resize(s_frame->get_size() + 100'000);

  s_frame->get_buffer()[s_frame_fill] = byte;
  ++s_frame_fill;
}

static std::string
calc_frame_checksum(uint64_t skip_at_end) {
  if (skip_at_end >= s_frame_fill)
    return 0;

  auto checksum = mtx::checksum::calculate_as_hex_string(mtx::checksum::algorithm_e::md5, s_frame->get_buffer(), s_frame_fill - skip_at_end);
  s_frame_fill  = 0;

  return checksum;
}

static mm_file_io_cptr
open_file(std::string const &file_name) {
  try {
    return std::make_shared<mm_file_io_c>(file_name);

  } catch (mtx::mm_io::exception &) {
    mxerror(Y("File not found\n"));
  }

  return {};
}

static void
show_nalu(uint32_t type,
          uint64_t size,
          uint64_t position,
          std::string const &checksum,
          std::optional<uint64_t> marker_size = std::nullopt) {
  mxinfo(fmt::format("NALU type 0x{0:02x} ({1}) size {2}{3}{4} checksum 0x{5}\n",
                     type,
                     s_parser->get_nalu_type_name(type),
                     size,
                     s_portable_format ? ""s : marker_size ? fmt::format(" marker size {0}", *marker_size) : ""s,
                     s_portable_format ? ""s : fmt::format(" at {0}", position),
                     checksum));
}

static void
parse_file(std::string const &file_name) {
  auto in_ptr    = open_file(file_name);
  auto &in       = *in_ptr;
  auto file_size = static_cast<uint64_t>(in.get_size());

  if (4 > file_size)
    return;

  auto marker               = static_cast<uint32_t>((1ull << 24) | in.read_uint24_be());
  auto marker_size          = 0;
  auto previous_marker_size = 0;
  auto previous_pos         = static_cast<int64_t>(-1);
  auto previous_type        = 0;

  while (true) {
    auto pos = in.getFilePointer();
    if (pos >= file_size)
      break;

    if (marker == mtx::avc_hevc::NALU_START_CODE)
      marker_size = 4;

    else if ((marker & 0x00ffffff) == mtx::avc_hevc::NALU_START_CODE)
      marker_size = 3;

    else {
      auto byte = in.read_uint8();
      marker    = (marker << 8) | byte;

      add_frame_byte(byte);

      continue;
    }

    pos -= marker_size;

    if (-1 != previous_pos)
      show_nalu(previous_type, pos - previous_pos - previous_marker_size, previous_pos, calc_frame_checksum(marker_size), previous_marker_size);
    else
      s_frame_fill = 0;

    auto next_bytes      = in.read_uint32_be();
    previous_pos         = pos;
    previous_marker_size = marker_size;
    previous_type        = (next_bytes >> (24 + 1)) & 0x3f;
    marker               = (1ull << 24) | (next_bytes & 0x00ff'ffff);

    for (auto idx = 0; idx < 4; ++idx)
      add_frame_byte(next_bytes >> ((3 - idx) * 8));
  }

  if (-1 != previous_pos)
    show_nalu(previous_type, in.getFilePointer() - previous_pos - previous_marker_size, previous_pos, calc_frame_checksum(0), previous_marker_size);
}

static void
parse_file_framed(std::string const &file_name) {
  auto in_ptr    = open_file(file_name);
  auto &in       = *in_ptr;
  auto file_size = static_cast<uint64_t>(in.get_size());

  uint64_t pos{};

  while ((pos + 5) < file_size) {
    auto nalu_size = in.read_uint32_be();
    auto nalu_type = (in.read_uint8() >> 1) & 0x3f;

    if (!nalu_size)
      return;

    in.setFilePointer(pos + 4);
    auto frame = in.read(nalu_size);

    show_nalu(nalu_type, nalu_size, pos, mtx::checksum::calculate_as_hex_string(mtx::checksum::algorithm_e::md5, *frame));

    pos += nalu_size + 4;
  }
}

int
main(int argc,
     char **argv) {
  mtx_common_init("hevc_dump", argv[0]);
  setup_help();

  s_parser = std::make_unique<mtx::hevc::es_parser_c>();

  auto args = mtx::cli::args_in_utf8(argc, argv);
  while (mtx::cli::handle_common_args(args, "-r"))
    ;

  auto file_name = parse_args(args);

  if (s_is_framed)
    parse_file_framed(file_name);
  else
    parse_file(file_name);

  mxexit();
}
