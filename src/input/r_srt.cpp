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

#include "common/codec.h"
#include "common/id_info.h"
#include "common/mm_proxy_io.h"
#include "common/mm_text_io.h"
#include "input/r_srt.h"
#include "input/subtitles.h"
#include "merge/input_x.h"

bool
srt_reader_c::probe_file() {
  return srt_parser_c::probe(static_cast<mm_text_io_c &>(*m_in));
}

void
srt_reader_c::read_headers() {
  auto text_in = std::static_pointer_cast<mm_text_io_c>(m_in);
  auto cc_utf8 = text_in->get_byte_order_mark() != byte_order_mark_e::none ? charset_converter_c::init("UTF-8")
               : mtx::includes(m_ti.m_sub_charsets,  0)                    ? charset_converter_c::init(m_ti.m_sub_charsets[ 0])
               : mtx::includes(m_ti.m_sub_charsets, -1)                    ? charset_converter_c::init(m_ti.m_sub_charsets[-1])
               :                                                             charset_converter_cptr{};
  m_encoding   = text_in->get_encoding();
  m_subs       = std::make_shared<srt_parser_c>(text_in, m_ti.m_fname, 0);

  show_demuxer_info();

  m_subs->set_charset_converter(cc_utf8);

  m_subs->parse();

  m_bytes_to_process = m_subs->get_total_byte_size();
}

void
srt_reader_c::create_packetizer(int64_t) {
  if (!demuxing_requested('s', 0) || !m_reader_packetizers.empty())
    return;

  add_packetizer(new textsubs_packetizer_c(this, m_ti, MKV_S_TEXTUTF8));

  show_packetizer_info(0, ptzr(0));
}

file_status_e
srt_reader_c::read(generic_packetizer_c *,
                   bool) {
  if (!m_subs->empty()) {
    m_bytes_processed += m_subs->get_next_byte_size();
    m_subs->process(&ptzr(0));
  }

  return m_subs->empty() ? flush_packetizers() : FILE_STATUS_MOREDATA;
}

int64_t
srt_reader_c::get_progress() {
  return m_bytes_processed;
}

int64_t
srt_reader_c::get_maximum_progress() {
  return m_bytes_to_process;
}

void
srt_reader_c::identify() {
  auto info = mtx::id::info_c{};

  info.add(mtx::id::text_subtitles, true);
  if (m_encoding)
    info.add(mtx::id::encoding, *m_encoding);

  id_result_container();
  id_result_track(0, ID_RESULT_TRACK_SUBTITLES, codec_c::get_name(codec_c::type_e::S_SRT, "SRT"), info.get());
}
