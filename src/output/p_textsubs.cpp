/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Subripper subtitle reader

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include <matroska/KaxTracks.h>

#include "common/codec.h"
#include "common/debugging.h"
#include "common/hacks.h"
#include "common/qt.h"
#include "common/strings/editing.h"
#include "common/strings/parsing.h"
#include "common/strings/utf8.h"
#include "merge/connection_checks.h"
#include "merge/output_control.h"
#include "merge/packet_extensions.h"
#include "output/p_textsubs.h"

textsubs_packetizer_c::textsubs_packetizer_c(generic_reader_c *p_reader,
                                             track_info_c &p_ti,
                                             const char *codec_id,
                                             bool recode)
  : generic_packetizer_c{p_reader, p_ti, track_subtitle}
  , m_strip_whitespaces{!mtx::hacks::is_engaged(mtx::hacks::KEEP_WHITESPACES_IN_TEXT_SUBTITLES)}
  , m_codec_id{codec_id}
{
  if (recode) {
    m_cc_utf8           = charset_converter_c::init(m_ti.m_sub_charset);
    m_converter_is_utf8 = charset_converter_c::is_utf8_charset_name(m_cc_utf8->get_charset());
    m_try_utf8          = m_ti.m_sub_charset.empty();
  }

  if (m_codec_id == MKV_S_TEXTUSF)
    set_default_compression_method(COMPRESSION_ZLIB);

  auto arg = std::string{};
  if (!debugging_c::requested("textsubs_force_rerender", &arg))
    return;

  auto tid_and_packetno = mtx::string::split(arg, ":");
  auto tid              = int64_t{};
  if (!mtx::string::parse_number(tid_and_packetno[0], tid) || (tid != m_ti.m_id))
    return;

  unsigned int packetno{};
  mtx::string::parse_number(tid_and_packetno[1], packetno);
  m_force_rerender_track_headers_on_packetno = packetno;

  mxdebug(fmt::format("textsubs_packetizer_c: track {0}: forcing rerendering of track headers after packet {1}\n", tid, packetno));
}

textsubs_packetizer_c::~textsubs_packetizer_c() {
}

void
textsubs_packetizer_c::set_headers() {
  set_codec_id(m_codec_id);
  set_codec_private(m_ti.m_private_data);

  generic_packetizer_c::set_headers();

  m_track_entry->EnableLacing(false);
}

void
textsubs_packetizer_c::set_line_ending_style(mtx::string::line_ending_style_e line_ending_style) {
  m_line_ending_style = line_ending_style;
}

void
textsubs_packetizer_c::process_impl(packet_cptr const &packet) {
  if (m_buffered_packet) {
    m_buffered_packet->duration = packet->timestamp - m_buffered_packet->timestamp;
    process_one_packet(m_buffered_packet);
    m_buffered_packet.reset();
  }

  auto subs   = recode(packet->data->to_string());
  subs        = mtx::string::normalize_line_endings(subs, m_line_ending_style);

  if (m_strip_whitespaces) {
    auto q_subs = Q(subs);

    q_subs.replace(QRegularExpression{Q("^[ \t]+|[ \t]+$"), QRegularExpression::MultilineOption}, {});
    q_subs.replace(QRegularExpression{Q("[ \t]+\r")},                                             Q("\r"));
    q_subs.replace(QRegularExpression{Q("[\r\n]+\\z")},                                           {});

    subs = to_utf8(q_subs);

  } else
    subs = mtx::string::chomp(subs);

  if (subs.empty())
    return;

  packet->data = memory_c::clone(subs);

  packet->force_key_frame();

  if (0 <= packet->duration)
    process_one_packet(packet);

  else {
    m_buffered_packet = packet;
    m_buffered_packet->data->take_ownership();
  }
}

std::string
textsubs_packetizer_c::recode(std::string subs) {
  if (m_try_utf8 && !mtx::utf8::is_valid(subs))
    m_try_utf8 = false;

  auto emit_invalid_utf8_warning = false;

  if (!m_try_utf8 && m_cc_utf8) {
    if (!m_invalid_utf8_warned && m_converter_is_utf8 && !mtx::utf8::is_valid(subs))
      emit_invalid_utf8_warning = true;

    subs = m_cc_utf8->utf8(subs);

  } else if (!m_invalid_utf8_warned && !mtx::utf8::is_valid(subs))
    emit_invalid_utf8_warning = true;

  if (emit_invalid_utf8_warning) {
    m_invalid_utf8_warned = true;
    mxwarn_tid(m_ti.m_fname, m_ti.m_id, fmt::format(FY("This text subtitle track contains invalid 8-bit characters outside valid multi-byte UTF-8 sequences. Please specify the correct encoding for this track.\n")));
  }

  return subs;
}

void
textsubs_packetizer_c::process_one_packet(packet_cptr const &packet) {
  ++m_packetno;

  if (0 > packet->duration) {
    subtitle_number_packet_extension_c *extension = dynamic_cast<subtitle_number_packet_extension_c *>(packet->find_extension(packet_extension_c::SUBTITLE_NUMBER));
    mxwarn_tid(m_ti.m_fname, m_ti.m_id, fmt::format(FY("Ignoring an entry which starts after it ends ({0}).\n"), extension ? extension->get_number() : static_cast<unsigned int>(m_packetno)));
    return;
  }

  add_packet(packet);

  if (m_force_rerender_track_headers_on_packetno && (*m_force_rerender_track_headers_on_packetno == m_packetno)) {
    auto codec_private = memory_c::alloc(20000);
    std::memset(codec_private->get_buffer(), 0, codec_private->get_size());
    set_codec_private(codec_private);
    rerender_track_headers();
  }
}

connection_result_e
textsubs_packetizer_c::can_connect_to(generic_packetizer_c *src,
                                      std::string &error_message) {
  textsubs_packetizer_c *psrc = dynamic_cast<textsubs_packetizer_c *>(src);
  if (!psrc)
    return CAN_CONNECT_NO_FORMAT;

  connect_check_codec_private(src);

  return CAN_CONNECT_YES;
}
