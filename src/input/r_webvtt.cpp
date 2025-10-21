/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   WebVTT subtitle reader

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/codec.h"
#include "common/id_info.h"
#include "common/mm_proxy_io.h"
#include "common/mm_text_io.h"
#include "input/r_webvtt.h"
#include "output/p_webvtt.h"
#include "merge/input_x.h"

bool
webvtt_reader_c::probe_file() {
  return m_in->getline(100).find("WEBVTT") == 0;
}

void
webvtt_reader_c::read_headers() {
  show_demuxer_info();
}

void
webvtt_reader_c::parse_file() {
  auto size    = m_in->get_size() - m_in->getFilePointer();
  auto content = m_in->read(size);

  m_parser     = std::make_shared<mtx::webvtt::parser_c>();

  m_parser->add_joined_lines(*content);
  m_parser->flush();

  m_bytes_to_process = m_parser->get_total_number_of_bytes();
}

void
webvtt_reader_c::create_packetizer(int64_t) {
  if (!demuxing_requested('s', 0) || !m_reader_packetizers.empty())
    return;

  parse_file();

  m_ti.m_private_data = m_parser->get_codec_private();

  add_packetizer(new webvtt_packetizer_c(this, m_ti));

  show_packetizer_info(0, ptzr(0));
}

file_status_e
webvtt_reader_c::read(generic_packetizer_c *,
                      bool) {
  if (!m_parser->cue_available())
    return FILE_STATUS_DONE;

  auto cue    = m_parser->get_cue();
  auto packet = std::make_shared<packet_t>(cue->m_content, cue->m_start.to_ns(), cue->m_duration.to_ns());

  if (cue->m_addition) {
    m_bytes_processed += cue->m_addition->get_size();
    packet->data_adds.emplace_back(cue->m_addition);
  }

  m_bytes_processed += cue->m_content->get_size();

  ptzr(0).process(packet);

  return FILE_STATUS_MOREDATA;
}

int64_t
webvtt_reader_c::get_progress() {
  return m_bytes_processed;
}

int64_t
webvtt_reader_c::get_maximum_progress() {
  return m_bytes_to_process;
}

void
webvtt_reader_c::identify() {
  auto info = mtx::id::info_c{};
  info.add(mtx::id::text_subtitles, true);
  info.add(mtx::id::encoding,       "UTF-8");

  id_result_container();
  id_result_track(0, ID_RESULT_TRACK_SUBTITLES, codec_c::get_name(codec_c::type_e::S_WEBVTT, "WebVTT"), info.get());
}
