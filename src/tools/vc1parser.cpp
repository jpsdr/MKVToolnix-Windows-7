/*
   vc1parser - A tool for testing the VC1 bitstream parser

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/byte_buffer.h"
#include "common/checksums/base.h"
#include "common/command_line.h"
#include "common/mm_file_io.h"
#include "common/translation.h"
#include "common/vc1.h"
#include "common/version.h"

static bool g_opt_checksum         = false;
static bool g_opt_entrypoints      = false;
static bool g_opt_frames           = false;
static bool g_opt_sequence_headers = false;

class vc1_info_c: public mtx::vc1::es_parser_c {
public:
  vc1_info_c()
    : mtx::vc1::es_parser_c() {
  }

  virtual ~vc1_info_c() {
  }

protected:
  virtual void handle_end_of_sequence_packet(memory_cptr const &packet) override;
  virtual void handle_entrypoint_packet(memory_cptr const &packet) override;
  virtual void handle_field_packet(memory_cptr const &packet) override;
  virtual void handle_frame_packet(memory_cptr const &packet) override;
  virtual void handle_sequence_header_packet(memory_cptr const &packet) override;
  virtual void handle_slice_packet(memory_cptr const &packet) override;
  virtual void handle_unknown_packet(uint32_t marker, memory_cptr const &packet) override;

  virtual void dump_sequence_header(mtx::vc1::sequence_header_t &seqhdr);
  virtual void dump_entrypoint(mtx::vc1::entrypoint_t &entrypoint);
  virtual void dump_frame_header(mtx::vc1::frame_header_t &frame_header);

  virtual std::string create_checksum_info(memory_cptr const &packet);
};

void
vc1_info_c::handle_end_of_sequence_packet(memory_cptr const &packet) {
  std::string checksum = create_checksum_info(packet);
  mxinfo(fmt::format(FY("End of sequence at {0} size {1}{2}\n"), m_stream_pos, packet->get_size(), checksum));
}

void
vc1_info_c::handle_entrypoint_packet(memory_cptr const &packet) {
  std::string checksum = create_checksum_info(packet);
  mxinfo(fmt::format(FY("Entrypoint at {0} size {1}{2}\n"), m_stream_pos, packet->get_size(), checksum));

  if (!g_opt_entrypoints)
    return;

  if (!m_seqhdr_found) {
    mxinfo(Y("  No sequence header found yet; parsing not possible\n"));
    return;
  }

  mtx::vc1::entrypoint_t entrypoint;
  if (mtx::vc1::parse_entrypoint(packet->get_buffer(), packet->get_size(), entrypoint, m_seqhdr))
    dump_entrypoint(entrypoint);
}

void
vc1_info_c::handle_field_packet(memory_cptr const &packet) {
  std::string checksum = create_checksum_info(packet);
  mxinfo(fmt::format(FY("Field at {0} size {1}{2}\n"), m_stream_pos, packet->get_size(), checksum));
}

void
vc1_info_c::handle_frame_packet(memory_cptr const &packet) {
  std::string checksum = create_checksum_info(packet);
  mxinfo(fmt::format(FY("Frame at {0} size {1}{2}\n"), m_stream_pos, packet->get_size(), checksum));

  if (!g_opt_frames)
    return;

  if (!m_seqhdr_found) {
    mxinfo(Y("  No sequence header found yet; parsing not possible\n"));
    return;
  }

  mtx::vc1::frame_header_t frame_header;
  if (mtx::vc1::parse_frame_header(packet->get_buffer(), packet->get_size(), frame_header, m_seqhdr))
    dump_frame_header(frame_header);
}

void
vc1_info_c::handle_sequence_header_packet(memory_cptr const &packet) {
  std::string checksum = create_checksum_info(packet);
  mxinfo(fmt::format(FY("Sequence header at {0} size {1}{2}\n"), m_stream_pos, packet->get_size(), checksum));

  m_seqhdr_found = mtx::vc1::parse_sequence_header(packet->get_buffer(), packet->get_size(), m_seqhdr);

  if (g_opt_sequence_headers) {
    if (m_seqhdr_found)
      dump_sequence_header(m_seqhdr);
    else
      mxinfo(Y("  parsing failed\n"));
  }
}

void
vc1_info_c::handle_slice_packet(memory_cptr const &packet) {
  std::string checksum = create_checksum_info(packet);
  mxinfo(fmt::format(FY("Slice at {0} size {1}{2}\n"), m_stream_pos, packet->get_size(), checksum));
}

void
vc1_info_c::handle_unknown_packet(uint32_t marker,
                                  memory_cptr const &packet) {
  std::string checksum = create_checksum_info(packet);
  mxinfo(fmt::format(FY("Unknown (0x{0:08x}) at {1} size {2}{3}\n"), marker, m_stream_pos, packet->get_size(), checksum));
}

std::string
vc1_info_c::create_checksum_info(memory_cptr const &packet) {
  if (!g_opt_checksum)
    return "";

  return fmt::format(FY(" checksum 0x{0:08x}"), mtx::checksum::calculate_as_uint(mtx::checksum::algorithm_e::adler32, *packet));
}

void
vc1_info_c::dump_sequence_header(mtx::vc1::sequence_header_t &seqhdr) {
  static const char *profile_names[4] = { "Simple", "Main", "Complex", "Advanced" };

  mxinfo(fmt::format(FY("  Sequence header dump:\n"
                        "    profile:               {0} ({1})\n"
                        "    level:                 {2}\n"
                        "    chroma_format:         {3}\n"
                        "    frame_rtq_postproc:    {4}\n"
                        "    bit_rtq_postproc:      {5}\n"
                        "    postproc_flag:         {6}\n"
                        "    pixel_width:           {7}\n"
                        "    pixel_height:          {8}\n"
                        "    pulldown_flag:         {9}\n"
                        "    interlace_flag:        {10}\n"
                        "    tf_counter_flag:       {11}\n"
                        "    f_inter_p_flag:        {12}\n"
                        "    psf_mode_flag:         {13}\n"
                        "    display_info_flag:     {14}\n"
                        "    display_width:         {15}\n"
                        "    display_height:        {16}\n"
                        "    aspect_ratio_flag:     {17}\n"
                        "    aspect_ratio_width:    {18}\n"
                        "    aspect_ratio_height:   {19}\n"
                        "    framerate_flag:        {20}\n"
                        "    framerate_num:         {21}\n"
                        "    framerate_den:         {22}\n"
                        "    hrd_param_flag:        {23}\n"
                        "    hrd_num_leaky_buckets: {24}\n"),
                     seqhdr.profile, profile_names[seqhdr.profile],
                     seqhdr.level,
                     seqhdr.chroma_format,
                     seqhdr.frame_rtq_postproc,
                     seqhdr.bit_rtq_postproc,
                     seqhdr.postproc_flag,
                     seqhdr.pixel_width,
                     seqhdr.pixel_height,
                     seqhdr.pulldown_flag,
                     seqhdr.interlace_flag,
                     seqhdr.tf_counter_flag,
                     seqhdr.f_inter_p_flag,
                     seqhdr.psf_mode_flag,
                     seqhdr.display_info_flag,
                     seqhdr.display_width,
                     seqhdr.display_height,
                     seqhdr.aspect_ratio_flag,
                     seqhdr.aspect_ratio_width,
                     seqhdr.aspect_ratio_height,
                     seqhdr.framerate_flag,
                     seqhdr.framerate_num,
                     seqhdr.framerate_den,
                     seqhdr.hrd_param_flag,
                     seqhdr.hrd_num_leaky_buckets));
}

void
vc1_info_c::dump_entrypoint(mtx::vc1::entrypoint_t &entrypoint) {
  mxinfo(fmt::format(FY("  Entrypoint dump:\n"
                        "    broken_link_flag:      {0}\n"
                        "    closed_entry_flag:     {1}\n"
                        "    pan_scan_flag:         {2}\n"
                        "    refdist_flag:          {3}\n"
                        "    loop_filter_flag:      {4}\n"
                        "    fast_uvmc_flag:        {5}\n"
                        "    extended_mv_flag:      {6}\n"
                        "    dquant:                {7}\n"
                        "    vs_transform_flag:     {8}\n"
                        "    overlap_flag:          {9}\n"
                        "    quantizer_mode:        {10}\n"
                        "    coded_dimensions_flag: {11}\n"
                        "    coded_width:           {12}\n"
                        "    coded_height:          {13}\n"
                        "    extended_dmv_flag:     {14}\n"
                        "    luma_scaling_flag:     {15}\n"
                        "    luma_scaling:          {16}\n"
                        "    chroma_scaling_flag:   {17}\n"
                        "    chroma_scaling:        {18}\n"),
                     entrypoint.broken_link_flag,
                     entrypoint.closed_entry_flag,
                     entrypoint.pan_scan_flag,
                     entrypoint.refdist_flag,
                     entrypoint.loop_filter_flag,
                     entrypoint.fast_uvmc_flag,
                     entrypoint.extended_mv_flag,
                     entrypoint.dquant,
                     entrypoint.vs_transform_flag,
                     entrypoint.overlap_flag,
                     entrypoint.quantizer_mode,
                     entrypoint.coded_dimensions_flag,
                     entrypoint.coded_width,
                     entrypoint.coded_height,
                     entrypoint.extended_dmv_flag,
                     entrypoint.luma_scaling_flag,
                     entrypoint.luma_scaling,
                     entrypoint.chroma_scaling_flag,
                     entrypoint.chroma_scaling));
}

void
vc1_info_c::dump_frame_header(mtx::vc1::frame_header_t &frame_header) {
  mxinfo(fmt::format(FY("  Frame header dump:\n"
                        "    fcm:                     {0} ({1})\n"
                        "    frame_type:              {2}\n"
                        "    tf_counter:              {3}\n"
                        "    repeat_frame:            {4}\n"
                        "    top_field_first_flag:    {5}\n"
                        "    repeat_first_field_flag: {6}\n"),
                     frame_header.fcm,
                       frame_header.fcm        == 0x00                           ? Y("progressive")
                     : frame_header.fcm        == 0x10                           ? Y("frame-interlace")
                     : frame_header.fcm        == 0x11                           ? Y("field-interlace")
                     :                                                             Y("unknown"),
                       frame_header.frame_type == mtx::vc1::FRAME_TYPE_I         ? Y("I")
                     : frame_header.frame_type == mtx::vc1::FRAME_TYPE_P         ? Y("P")
                     : frame_header.frame_type == mtx::vc1::FRAME_TYPE_B         ? Y("B")
                     : frame_header.frame_type == mtx::vc1::FRAME_TYPE_BI        ? Y("BI")
                     : frame_header.frame_type == mtx::vc1::FRAME_TYPE_P_SKIPPED ? Y("P (skipped)")
                     :                                                             Y("unknown"),
                     frame_header.tf_counter,
                     frame_header.repeat_frame,
                     frame_header.top_field_first_flag,
                     frame_header.repeat_first_field_flag));
}

static void
setup_help() {
  mtx::cli::g_usage_text = Y("vc1parser [options] input_file_name\n"
                             "\n"
                             "Options for output and information control:\n"
                             "\n"
                             "  -c, --checksum         Calculate and output checksums of each unit\n"
                             "  -e, --entrypoints      Show the content of entry point headers\n"
                             "  -f, --frames           Show basic frame header content\n"
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

    else if ((*arg == "-e") || (*arg == "--entrypoints"))
      g_opt_entrypoints = true;

    else if ((*arg == "-f") || (*arg == "--frames"))
      g_opt_frames = true;

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

  vc1_info_c parser;

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
  mtx_common_init("vc1parser", argv[0]);
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
