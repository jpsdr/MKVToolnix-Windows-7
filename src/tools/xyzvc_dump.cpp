/*
   xyzvc_dump - A tool for dumping HEVC structures

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include <memory>
#include <QRegularExpression>

#include "common/avc/es_parser.h"
#include "common/checksums/base_fwd.h"
#include "common/command_line.h"
#include "common/endian.h"
#include "common/hevc/es_parser.h"
#include "common/hevc/types.h"
#include "common/list_utils.h"
#include "common/mm_io_x.h"
#include "common/mm_file_io.h"
#include "common/qt.h"
#include "common/vvc/es_parser.h"
#include "common/xyzvc/es_parser.h"
#include "qregularexpression.h"

enum class framing_type_e {
  unknown,
  iso_14496_15,
  annex_b,
};

class parser_c {
protected:
  std::unique_ptr<mtx::xyzvc::es_parser_c> m_es_parser;

public:
  virtual ~parser_c() = default;

  virtual std::string get_full_name() = 0;
  virtual std::string get_itu_t_name() = 0;
  virtual QRegularExpression get_file_name_regex() = 0;
  virtual unsigned int get_nalu_type(uint8_t *bytes) = 0;
  virtual std::string get_nalu_type_name(unsigned int type) {
    return m_es_parser->get_nalu_type_name(type);
  }
  virtual std::optional<uint32_t> determine_inner_nalu_type([[maybe_unused]] uint8_t const *buffer, [[maybe_unused]] uint64_t size, [[maybe_unused]] uint32_t type) {
    return {};
  }
};

class avc_parser_c: public parser_c {
public:
  avc_parser_c() {
    m_es_parser.reset(new mtx::avc::es_parser_c);
  }

  virtual std::string get_full_name() override {
    return "AVC/H.264";
  }

  virtual std::string get_itu_t_name() override {
    return "H.264";
  }

  virtual QRegularExpression get_file_name_regex() override {
    return QRegularExpression{ Q("\\.(avc|[hx]?264)$"), QRegularExpression::CaseInsensitiveOption };
  }

  virtual unsigned int get_nalu_type(uint8_t *bytes) override {
    return bytes[0] & 0x1f;
  }
};

class hevc_parser_c: public parser_c {
public:
  hevc_parser_c() {
    m_es_parser.reset(new mtx::hevc::es_parser_c);
  }

  virtual std::string get_full_name() override {
    return "HEVC/H.265";
  }

  virtual std::string get_itu_t_name() override {
    return "H.265";
  }

  virtual QRegularExpression get_file_name_regex() override {
    return QRegularExpression{ Q("\\.(hevc|[hx]?265)$"), QRegularExpression::CaseInsensitiveOption };
  }

  virtual unsigned int get_nalu_type(uint8_t *bytes) override {
    return (bytes[0] >> 1) & 0x3f;
  }

  virtual std::optional<uint32_t> determine_inner_nalu_type(uint8_t const *buffer, uint64_t size, uint32_t type) override {
    if ((size >= 3) && mtx::included_in(static_cast<int>(type), mtx::hevc::NALU_TYPE_UNSPEC62, mtx::hevc::NALU_TYPE_UNSPEC63))
      return (buffer[2] >> 1) & 0x3f;

    return {};
  }
};

class vvc_parser_c: public parser_c {
public:
  vvc_parser_c() {
    m_es_parser.reset(new mtx::vvc::es_parser_c);
  }

  virtual std::string get_full_name() override {
    return "VVC/H.266";
  }

  virtual std::string get_itu_t_name() override {
    return "H.266";
  }

  virtual QRegularExpression get_file_name_regex() override {
    return QRegularExpression{ Q("\\.(vvc|[hx]?266)$"), QRegularExpression::CaseInsensitiveOption };
  }

  virtual unsigned int get_nalu_type(uint8_t *bytes) override {
    return (bytes[1] >> 3) & 0x1f;
  }
};

static framing_type_e s_framing_type{framing_type_e::unknown};
static bool s_portable_format{};
static memory_cptr s_frame;
static uint64_t s_frame_fill{};
static std::shared_ptr<parser_c> s_parser;

static void
setup_help() {
  mtx::cli::g_usage_text = "xyzvc_dump [options] input_file_name\n"
    "\n"
    "General options:\n"
    "\n"
    "  -4, --h264, --avc      Treat input file as an AVC/H.264 file instead of\n"
    "                         deriving its type from its file name extension\n"
    "  -5, --h265, --hevc     Treat input file as an HEVC/H.265 file instead of\n"
    "                         deriving its type from its file name extension\n"
    "  -6, --h266, --vvc      Treat input file as an VVC/H.266 file instead of\n"
    "                         deriving its type from its file name extension\n"
    "  -a, --annex-b          Treat input file as an ITU-T H.264/H.265 Annex B\n"
    "                         bitstream instead of trying to derive the type\n"
    "                         from the content\n"
    "  -i, --iso-14496-15     Treat input file as an ISO/IEC 14496-15 bitstream\n"
    "                         (NALUs prefixed with a four-byte size field) instead\n"
    "                         of trying to derive the type from the content\n"
    "  -p, --portable-format  Output a format that's comparable with e.g. 'diff'\n"
    "                         between the ISO/IEC 14496-15 bitstream and ITU-T\n"
    "                         H.264/H.265 Annex B bitsream variants by not\n"
    "                         outputting the NALU's position (both modes) nor the\n"
    "                         marker size (Annex B mode).\n"
    "  -h, --help             This help text\n"
    "  -V, --version          Print version information\n";
}

static framing_type_e
detect_framing_type(std::string const &file_name) {
  mm_file_io_c in{file_name, libebml::MODE_READ};

  auto marker_or_size = in.read_uint32_be();

  if (marker_or_size == mtx::xyzvc::NALU_START_CODE)
    return framing_type_e::annex_b;

  if ((marker_or_size >> 8) != mtx::xyzvc::NALU_START_CODE)
    return framing_type_e::iso_14496_15;

  try {
    int tries = 0;
    while (tries < 5) {
      in.setFilePointer(marker_or_size, libebml::seek_current);
      marker_or_size = in.read_uint32_be();
    }

    return framing_type_e::iso_14496_15;

  } catch (mtx::mm_io::exception &) {
  }

  return framing_type_e::annex_b;
}

static std::string
parse_args(std::vector<std::string> &args) {
  std::string file_name;
  std::vector<std::shared_ptr<parser_c>> parsers = {
    std::make_shared<avc_parser_c>(),
    std::make_shared<hevc_parser_c>(),
    std::make_shared<vvc_parser_c>(),
  };

  for (auto & arg: args) {
    if ((arg == "-4") || (arg == "--h264") || (arg == "--avc"))
      s_parser = parsers[0];

    else if ((arg == "-5") || (arg == "--h265") || (arg == "--hevc"))
      s_parser = parsers[1];

    else if ((arg == "-6") || (arg == "--h266") || (arg == "--vvc"))
      s_parser = parsers[2];

    else if ((arg == "-a") || (arg == "--annex-b"))
      s_framing_type = framing_type_e::annex_b;

    else if ((arg == "-i") || (arg == "--iso-14496-15"))
      s_framing_type = framing_type_e::iso_14496_15;

    else if ((arg == "-p") || (arg == "--portable-format"))
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

  if (!s_parser) {
    auto file_name_q = Q(file_name);

    for (auto const &parser : parsers)
      if (file_name_q.contains(parser->get_file_name_regex())) {
        s_parser = parser;
        break;
      }

    if (!s_parser)
      mxerror("The file type could not be derived from the file name's extension. Please specify the corresponding command line option (see 'xyzvc_dump --help').\n");
  }

  if (s_framing_type == framing_type_e::unknown) {
    s_framing_type = detect_framing_type(file_name);

    if (s_framing_type == framing_type_e::unknown)
      mxerror("The framing type could not be derived from the file's content. Please specify the corresponding command line option (see 'xyzvc_dump --help').\n");
  }

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
    return {};

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
          std::optional<uint64_t> marker_size = std::nullopt,
          std::optional<uint32_t> inner_type = std::nullopt) {
  std::string inner_type_str;

  if (inner_type)
    inner_type_str = fmt::format(" inner NALU type 0x{0:02x} ({1})", *inner_type, s_parser->get_nalu_type_name(*inner_type));

  mxinfo(fmt::format("NALU type 0x{0:02x} ({1}) size {2}{3}{4}{5} checksum 0x{6}{7}\n",
                     type,
                     s_parser->get_nalu_type_name(type),
                     size,
                     s_portable_format ? ""s : marker_size ? fmt::format(" marker size {0}", *marker_size) : ""s,
                     s_portable_format ? ""s : fmt::format(" at {0}", position),
                     s_portable_format ? ""s : fmt::format(" ends at {0}", position + size + marker_size.value_or(4)),
                     checksum,
                     inner_type_str));
}

static void
parse_file_annex_b(std::string const &file_name) {
  auto in_ptr    = open_file(file_name);
  auto &in       = *in_ptr;
  auto file_size = static_cast<uint64_t>(in.get_size());

  mxinfo(fmt::format("{0}: {1}, ITU-T {2} Annex B, {3} bytes\n", file_name, s_parser->get_full_name(), s_parser->get_itu_t_name(), file_size));

  if (4 > file_size)
    return;

  auto marker               = static_cast<uint32_t>((1ull << 24) | in.read_uint24_be());
  auto marker_size          = 0;
  auto previous_marker_size = 0;
  auto previous_pos         = static_cast<int64_t>(-1);
  auto previous_type        = 0;
  uint8_t next_bytes[4];

  std::memset(next_bytes, 0, 4);

  while (true) {
    auto pos = in.getFilePointer();
    if (pos >= file_size)
      break;

    if (marker == mtx::xyzvc::NALU_START_CODE)
      marker_size = 4;

    else if ((marker & 0x00ffffff) == mtx::xyzvc::NALU_START_CODE)
      marker_size = 3;

    else {
      auto byte = in.read_uint8();
      marker    = (marker << 8) | byte;

      add_frame_byte(byte);

      continue;
    }

    pos -= marker_size;

    if (-1 != previous_pos) {
      auto size       = pos - previous_pos - previous_marker_size;
      auto inner_type = s_parser->determine_inner_nalu_type(s_frame->get_buffer(), size, previous_type);
      show_nalu(previous_type, size, previous_pos, calc_frame_checksum(marker_size), previous_marker_size, inner_type);

    } else
      s_frame_fill = 0;

    if (in.getFilePointer() >= file_size)
      return;

    auto num_to_read     = std::min<uint64_t>(4, file_size - in.getFilePointer());
    auto num_read        = in.read(next_bytes, num_to_read);
    previous_pos         = pos;
    previous_marker_size = marker_size;
    previous_type        = s_parser->get_nalu_type(next_bytes);
    marker               = (1ull << 24) | get_uint24_be(&next_bytes[1]);

    for (auto idx = 0u; idx < num_read; ++idx)
      add_frame_byte(next_bytes[idx]);
  }

  if (-1 == previous_pos)
    return;

  auto size       = in.getFilePointer() - previous_pos - previous_marker_size;
  auto inner_type = s_parser->determine_inner_nalu_type(s_frame->get_buffer(), size, previous_type);

  show_nalu(previous_type, size, previous_pos, calc_frame_checksum(0), previous_marker_size, inner_type);
}

static void
parse_file_iso_14496_15(std::string const &file_name) {
  auto in_ptr    = open_file(file_name);
  auto &in       = *in_ptr;
  auto file_size = static_cast<uint64_t>(in.get_size());

  mxinfo(fmt::format("{0}: {1}, ISO/IEC 14496-15, {2} bytes\n", file_name, s_parser->get_full_name(), file_size));

  uint64_t pos{};

  while ((pos + 5) < file_size) {
    uint8_t nalu_header[2];

    auto nalu_size = in.read_uint32_be();
    in.read(nalu_header, 2);
    auto nalu_type = s_parser->get_nalu_type(nalu_header);

    if (!nalu_size)
      return;

    in.setFilePointer(pos + 4);
    auto frame      = in.read(nalu_size);
    auto inner_type = s_parser->determine_inner_nalu_type(frame->get_buffer(), nalu_size, nalu_type);

    show_nalu(nalu_type, nalu_size, pos, mtx::checksum::calculate_as_hex_string(mtx::checksum::algorithm_e::md5, *frame), std::nullopt, inner_type);

    pos += nalu_size + 4;
  }
}

int
main(int argc,
     char **argv) {
  mtx_common_init("xyzvc_dump", argv[0]);
  setup_help();

  auto args = mtx::cli::args_in_utf8(argc, argv);
  while (mtx::cli::handle_common_args(args, "-r"))
    ;

  auto file_name = parse_args(args);

  if (s_framing_type == framing_type_e::iso_14496_15)
    parse_file_iso_14496_15(file_name);
  else
    parse_file_annex_b(file_name);

  s_parser.reset();

  mxexit();
}
