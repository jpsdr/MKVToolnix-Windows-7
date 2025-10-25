/*
   diracparser - A tool for testing the Dirac bitstream parser

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/byte_buffer.h"
#include "common/checksums/base.h"
#include "common/command_line.h"
#include "common/dirac.h"
#include "common/mm_file_io.h"
#include "common/translation.h"
#include "common/version.h"

static bool g_opt_checksum         = false;
static bool g_opt_sequence_headers = false;

class dirac_info_c: public mtx::dirac::es_parser_c {
public:
  dirac_info_c()
    : mtx::dirac::es_parser_c() {
  }

  virtual ~dirac_info_c() {
  }

protected:
  virtual void handle_auxiliary_data_unit(memory_c const &packet) override;
  virtual void handle_end_of_sequence_unit(memory_c const &packet) override;
  virtual void handle_padding_unit(memory_c const &packet) override;
  virtual void handle_picture_unit(memory_cptr const &packet) override;
  virtual void handle_sequence_header_unit(memory_c const &packet) override;
  virtual void handle_unknown_unit(memory_c const &packet) override;

  virtual void dump_sequence_header(mtx::dirac::sequence_header_t &seqhdr);

  virtual std::string create_checksum_info(memory_c const &packet);
};

void
dirac_info_c::handle_auxiliary_data_unit(memory_c const &packet) {
  std::string checksum = create_checksum_info(packet);
  mxinfo(fmt::format(FY("Auxiliary data at {0} size {1}{2}\n"), m_stream_pos, packet.get_size(), checksum));
}

void
dirac_info_c::handle_end_of_sequence_unit(memory_c const &packet) {
  std::string checksum = create_checksum_info(packet);
  mxinfo(fmt::format(FY("End of sequence at {0} size {1}{2}\n"), m_stream_pos, packet.get_size(), checksum));
}

void
dirac_info_c::handle_padding_unit(memory_c const &packet) {
  std::string checksum = create_checksum_info(packet);
  mxinfo(fmt::format(FY("Padding at {0} size {1}{2}\n"), m_stream_pos, packet.get_size(), checksum));
}

void
dirac_info_c::handle_picture_unit(memory_cptr const &packet) {
  std::string checksum = create_checksum_info(*packet);
  mxinfo(fmt::format(FY("Picture at {0} size {1}{2}\n"), m_stream_pos, packet->get_size(), checksum));
}

void
dirac_info_c::handle_sequence_header_unit(memory_c const &packet) {
  std::string checksum = create_checksum_info(packet);
  mxinfo(fmt::format(FY("Sequence header at {0} size {1}{2}\n" ),m_stream_pos, packet.get_size(), checksum));

  m_seqhdr_found = mtx::dirac::parse_sequence_header(packet.get_buffer(), packet.get_size(), m_seqhdr);

  if (g_opt_sequence_headers) {
    if (m_seqhdr_found)
      dump_sequence_header(m_seqhdr);
    else
      mxinfo(Y("  parsing failed\n"));
  }
}

void
dirac_info_c::handle_unknown_unit(memory_c const &packet) {
  std::string checksum = create_checksum_info(packet);
  mxinfo(fmt::format(FY("Unknown (0x{0:02x}) at {1} size {2}{3}\n"), static_cast<int>(packet.get_buffer()[4]), m_stream_pos, packet.get_size(), checksum));
}

std::string
dirac_info_c::create_checksum_info(memory_c const &packet) {
  if (!g_opt_checksum)
    return "";

  return fmt::format(FY(" checksum 0x{0:08x}"), mtx::checksum::calculate_as_uint(mtx::checksum::algorithm_e::adler32, packet));
}

void
dirac_info_c::dump_sequence_header(mtx::dirac::sequence_header_t &seqhdr) {
  mxinfo(fmt::format(FY("  Sequence header dump:\n"
                        "    major_version:            {0}\n"
                        "    minor_version:            {1}\n"
                        "    profile:                  {2}\n"
                        "    level:                    {3}\n"
                        "    base_video_format:        {4}\n"
                        "    pixel_width:              {5}\n"
                        "    pixel_height:             {6}\n"
                        "    chroma_format:            {7}\n"
                        "    interlaced:               {8}\n"
                        "    top_field_first:          {9}\n"
                        "    frame_rate_numerator:     {10}\n"
                        "    frame_rate_denominator:   {11}\n"
                        "    aspect_ratio_numerator:   {12}\n"
                        "    aspect_ratio_denominator: {13}\n"
                        "    clean_width:              {14}\n"
                        "    clean_height:             {15}\n"
                        "    left_offset:              {16}\n"
                        "    top_offset:               {17}\n"),
                     seqhdr.major_version,
                     seqhdr.minor_version,
                     seqhdr.profile,
                     seqhdr.level,
                     seqhdr.base_video_format,
                     seqhdr.pixel_width,
                     seqhdr.pixel_height,
                     seqhdr.chroma_format,
                     seqhdr.interlaced,
                     seqhdr.top_field_first,
                     seqhdr.frame_rate_numerator,
                     seqhdr.frame_rate_denominator,
                     seqhdr.aspect_ratio_numerator,
                     seqhdr.aspect_ratio_denominator,
                     seqhdr.clean_width,
                     seqhdr.clean_height,
                     seqhdr.left_offset,
                     seqhdr.top_offset));
}

static void
setup_help() {
  mtx::cli::g_usage_text = Y("diracparser [options] input_file_name\n"
                             "\n"
                             "Options for output and information control:\n"
                             "\n"
                             "  -c, --checksum         Calculate and output checksums of each unit\n"
                             "  -s, --sequence-headers Show the content of sequence headers\n"
                             "\n"
                             "General options:\n"
                             "\n"
                             "  -h, --help             This help text\n"
                             "  -V, --version          Print version information\n");
}

static std::string
parse_args(std::vector<std::string> &args) {
  std::string file_name;

  std::vector<std::string>::iterator arg = args.begin();
  while (arg != args.end()) {
    if ((*arg == "-c") || (*arg == "--checksum"))
      g_opt_checksum = true;

    else if ((*arg == "-s") || (*arg == "--sequence-headers"))
      g_opt_sequence_headers = true;

    else if (!file_name.empty())
      mxerror(Y("More than one source file given.\n"));

    else
      file_name = *arg;

    ++arg;
  }

  if (file_name.empty())
    mxerror(Y("No file name given\n"));

  return file_name;
}

static void
parse_file(const std::string &file_name) {
  mm_file_io_c in(file_name);

  const int buf_size = 100000;
  int64_t size       = in.get_size();

  if (4 > size)
    mxerror(Y("File too small\n"));

  auto mem = memory_c::alloc(buf_size);
  auto ptr = mem->get_buffer();

  dirac_info_c parser;

  while (1) {
    int num_read = in.read(ptr, buf_size);

    parser.add_bytes(ptr, num_read);
    if (num_read < buf_size) {
      parser.flush();
      break;
    }
  }
}

int
main(int argc,
     char **argv) {
  mtx_common_init("diracparser", argv[0]);
  setup_help();

  auto args = mtx::cli::args_in_utf8(argc, argv);
  while (mtx::cli::handle_common_args(args, "-r"))
    ;

  auto file_name = parse_args(args);

  try {
    parse_file(file_name);
  } catch (...) {
    mxerror(Y("File not found\n"));
  }

  mxexit();
}
