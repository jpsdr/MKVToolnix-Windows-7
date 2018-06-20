/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   Subripper subtitle reader

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include <matroska/KaxTracks.h>

#include "common/codec.h"
#include "common/strings/editing.h"
#include "common/strings/parsing.h"
#include "merge/connection_checks.h"
#include "merge/output_control.h"
#include "merge/packet_extensions.h"
#include "output/p_textsubs.h"

using namespace libmatroska;

textsubs_packetizer_c::textsubs_packetizer_c(generic_reader_c *p_reader,
                                             track_info_c &p_ti,
                                             const char *codec_id,
                                             bool recode)
  : generic_packetizer_c(p_reader, p_ti)
  , m_codec_id{codec_id}
{
  if (recode)
    m_cc_utf8 = charset_converter_c::init(m_ti.m_sub_charset);

  set_track_type(track_subtitle);
  if (m_codec_id == MKV_S_TEXTUSF)
    set_default_compression_method(COMPRESSION_ZLIB);

  auto arg = std::string{};
  if (!debugging_c::requested("textsubs_force_rerender", &arg))
    return;

  auto tid_and_packetno = split(arg, ":");
  auto tid              = int64_t{};
  if (!parse_number(tid_and_packetno[0], tid) || (tid != m_ti.m_id))
    return;

  unsigned int packetno{};
  parse_number(tid_and_packetno[1], packetno);
  m_force_rerender_track_headers_on_packetno.reset(packetno);

  mxdebug(boost::format("textsubs_packetizer_c: track %1%: forcing rerendering of track headers after packet %2%\n") % tid % packetno);
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
textsubs_packetizer_c::set_line_ending_style(line_ending_style_e line_ending_style) {
  m_line_ending_style = line_ending_style;
}

int
textsubs_packetizer_c::process(packet_cptr packet) {
  ++m_packetno;

  if (0 > packet->duration) {
    subtitle_number_packet_extension_c *extension = dynamic_cast<subtitle_number_packet_extension_c *>(packet->find_extension(packet_extension_c::SUBTITLE_NUMBER));
    mxwarn_tid(m_ti.m_fname, m_ti.m_id, boost::format(Y("Ignoring an entry which starts after it ends (%1%).\n")) % (extension ? extension->get_number() : static_cast<unsigned int>(m_packetno)));
    return FILE_STATUS_MOREDATA;
  }

  packet->duration_mandatory = true;

  auto subs = std::string{reinterpret_cast<char *>(packet->data->get_buffer()), packet->data->get_size()};
  subs      = chomp(normalize_line_endings(subs, m_line_ending_style));

  if (m_cc_utf8)
    subs = m_cc_utf8->utf8(subs);

  packet->data = memory_c::borrow(subs);

  add_packet(packet);

  if (m_force_rerender_track_headers_on_packetno && (*m_force_rerender_track_headers_on_packetno == m_packetno)) {
    auto codec_private = memory_c::alloc(20000);
    std::memset(codec_private->get_buffer(), 0, codec_private->get_size());
    set_codec_private(codec_private);
    rerender_track_headers();
  }

  return FILE_STATUS_MOREDATA;
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
