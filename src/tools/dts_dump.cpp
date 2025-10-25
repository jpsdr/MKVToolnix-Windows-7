/*
   dts_dump - A tool for dumping DTS structures

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/dts.h"
#include "common/dts_parser.h"
#include "common/checksums/base_fwd.h"
#include "common/command_line.h"
#include "common/mm_io_x.h"
#include "common/mm_file_io.h"
#include "common/qt.h"

static bool s_portable_format{};

static void
setup_help() {
  mtx::cli::g_usage_text = "dts_dump [options] input_file_name\n"
    "\n"
    "General options:\n"
    "\n"
    "  -p, --portable-format  Output a format that's comparable with e.g. 'diff'\n"
    "                         by not outputting the frames' positions.\n"
    "  -h, --help             This help text\n"
    "  -V, --version          Print version information\n";
}

static std::string
parse_args(std::vector<std::string> &args) {
  std::string file_name;

  for (auto & arg: args) {
    if ((arg == "-p") || (arg == "--portable-format"))
      s_portable_format = true;

    else if (!file_name.empty())
      mxerror(Y("More than one source file was given.\n"));

    else
      file_name = arg;
  }

  if (file_name.empty()) {
    mxinfo(fmt::format("{0}\n", mtx::cli::g_usage_text));
    mxerror(Y("No file name given\n"));
  }

  return file_name;
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

static std::string
dts_type_to_string(mtx::dts::dts_type_e type) {
  return type == mtx::dts::dts_type_e::normal          ? "normal"
       : type == mtx::dts::dts_type_e::high_resolution ? "high_resolution"
       : type == mtx::dts::dts_type_e::master_audio    ? "master_audio"
       : type == mtx::dts::dts_type_e::express         ? "express"
       : type == mtx::dts::dts_type_e::es              ? "es"
       : type == mtx::dts::dts_type_e::x96_24          ? "x96_24"
       :                                                 "unknown";
}

static std::string
lfe_type_to_string(mtx::dts::lfe_type_e type) {
  return type == mtx::dts::lfe_type_e::none    ? "none"
       : type == mtx::dts::lfe_type_e::lfe_128 ? "lfe_128"
       : type == mtx::dts::lfe_type_e::lfe_64  ? "lfe_64"
       : type == mtx::dts::lfe_type_e::invalid ? "invalid"
       :                                         "unknown";
}

static std::string
extension_audio_descriptor_to_string(mtx::dts::extension_audio_descriptor_e descriptor) {
  return descriptor == mtx::dts::extension_audio_descriptor_e::xch      ? "xch"
       : descriptor == mtx::dts::extension_audio_descriptor_e::unknown1 ? "unknown1"
       : descriptor == mtx::dts::extension_audio_descriptor_e::x96k     ? "x96k"
       : descriptor == mtx::dts::extension_audio_descriptor_e::xch_x96k ? "xch_x96k"
       : descriptor == mtx::dts::extension_audio_descriptor_e::unknown4 ? "unknown4"
       : descriptor == mtx::dts::extension_audio_descriptor_e::unknown5 ? "unknown5"
       : descriptor == mtx::dts::extension_audio_descriptor_e::unknown6 ? "unknown6"
       : descriptor == mtx::dts::extension_audio_descriptor_e::unknown7 ? "unknown7"
       :                                                                  "unknown";
}

static void
parse_file(std::string const &file_name) {
  auto in_ptr            = open_file(file_name);
  auto &in               = *in_ptr;
  auto file_size         = static_cast<uint64_t>(in.get_size());
  auto file_pos          = uint64_t{};
  auto const buffer_size = 100'000;
  auto buffer_ptr        = memory_c::alloc(buffer_size);
  auto buffer            = buffer_ptr->get_buffer();
  auto buffer_fill       = 0u;
  auto end_of_file       = false;

  mtx::dts::parser_c parser;

  do {
    auto bytes_to_read = std::min<uint64_t>(file_size - in.getFilePointer(), buffer_size - buffer_fill);
    if (!bytes_to_read)
      break;

    end_of_file      = (file_pos + bytes_to_read) >= file_size;
    auto num_read    = in.read(&buffer[buffer_fill], bytes_to_read);
    buffer_fill     += num_read;
    auto buffer_pos  = 0u;

    while (true) {
      mtx::dts::header_t dtsheader{};

      auto pos = mtx::dts::find_header(&buffer[buffer_pos], buffer_fill, dtsheader, end_of_file);

      if ((0 > pos) || ((buffer_pos + pos + dtsheader.frame_byte_size) > buffer_fill)) {
        buffer_fill -= buffer_pos;
        std::memmove(&buffer[0], &buffer[buffer_pos], buffer_fill);

        break;
      }

      auto checksum = mtx::checksum::calculate_as_hex_string(mtx::checksum::algorithm_e::md5, &buffer[buffer_pos + pos], dtsheader.frame_byte_size);

      mxinfo(fmt::format("Frame size {0}{1} dts_type {2} channels {3} ({4}+{5}) lfe {6} sampling_frequency {7} (ext {8}) extension {9} dialog_normalization_gain {10} (ext {11}) "
                         "has_core {12} has_exss {13}{14} has_xch {15} checksum {16}\n",
                         dtsheader.frame_byte_size,
                         s_portable_format ? ""s : fmt::format(" pos {0}", file_pos),
                         dts_type_to_string(dtsheader.dts_type),
                         dtsheader.get_total_num_audio_channels(),
                         dtsheader.get_core_num_audio_channels(),
                         dtsheader.get_total_num_audio_channels() - dtsheader.get_core_num_audio_channels(),
                         lfe_type_to_string(dtsheader.lfe_type),
                         dtsheader.core_sampling_frequency,
                         dtsheader.extension_sampling_frequency.value_or(0),
                         dtsheader.extended_coding ? extension_audio_descriptor_to_string(dtsheader.extension_audio_descriptor) : "—"s,
                         dtsheader.dialog_normalization_gain,
                         dtsheader.extension_dialog_normalization_gain,
                         dtsheader.has_core,
                         dtsheader.has_exss,
                         dtsheader.has_exss ? fmt::format(" (header_size {0} part_size {1} pos {2})", dtsheader.exss_header_size, dtsheader.exss_part_size, dtsheader.exss_offset) : ""s,
                         dtsheader.has_xch,
                         checksum));

      file_pos   += pos + dtsheader.frame_byte_size;
      buffer_pos += pos + dtsheader.frame_byte_size;
    }
  } while (!end_of_file);
}

int
main(int argc,
     char **argv) {
  mtx_common_init("dts_dump", argv[0]);
  setup_help();

  auto args = mtx::cli::args_in_utf8(argc, argv);
  while (mtx::cli::handle_common_args(args, "-r"))
    ;

  auto file_name = parse_args(args);

  parse_file(file_name);

  mxexit();
}
