/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   HEVC ES video output module

*/

#include "common/common_pch.h"

#include <cstdint>
#include <type_traits>

#include "common/codec.h"
#include "common/fourcc.h"
#include "common/hacks.h"
#include "common/hevc/es_parser.h"
#include "common/list_utils.h"
#include "merge/connection_checks.h"
#include "output/p_avc_hevc_es.h"
#include "output/p_hevc_es.h"

static debugging_option_c s_debug_dovi_configuration_record{"dovi_configuration_record"};

hevc_es_video_packetizer_c::hevc_es_video_packetizer_c(generic_reader_c *p_reader,
                                                       track_info_c &p_ti,
                                                       uint32_t width,
                                                       uint32_t height)
  : avc_hevc_es_video_packetizer_c{p_reader, p_ti, "hevc", std::unique_ptr<mtx::avc_hevc::es_parser_c>(new mtx::hevc::es_parser_c), width, height}
  , m_parser{static_cast<mtx::hevc::es_parser_c &>(*m_parser_base)}
{
  set_codec_id(MKV_V_MPEGH_HEVC);

  m_parser.set_normalize_parameter_sets(!mtx::hacks::is_engaged(mtx::hacks::DONT_NORMALIZE_PARAMETER_SETS));
}

void
hevc_es_video_packetizer_c::handle_dovi_block_addition_mappings() {
  mxdebug_if(s_debug_dovi_configuration_record, fmt::format("hevc_es_video_packetizer_c::handle_dovi_block_addition_mappings: parser has DOVI RPU? {0} headers parsed? {1}\n", m_parser.has_dovi_rpu_header(), m_parser.headers_parsed()));

  if (!m_parser.has_dovi_rpu_header())
    return;

  for (auto const &addition : m_block_addition_mappings) {
    if (!addition.id_type)
      continue;

    auto type = fourcc_c{static_cast<uint32_t>(*addition.id_type)};

    mxdebug_if(s_debug_dovi_configuration_record, fmt::format("hevc_es_video_packetizer_c::handle_dovi_block_addition_mappings: have addition mapping with type {0}\n", type));

    if (mtx::included_in(type, fourcc_c{"dvcC"}, fourcc_c{"dvvC"}))
      return;
  }

  auto hdr                = m_parser.get_dovi_rpu_header();
  auto vui                = m_parser.get_vui_info();
  auto duration           = m_parser.has_stream_default_duration() ? m_parser.get_stream_default_duration() : m_parser.get_most_often_used_duration();

  auto dovi_config_record = create_dovi_configuration_record(hdr, m_width, m_height, vui, duration);

  auto mapping            = mtx::dovi::create_dovi_block_addition_mapping(dovi_config_record);

  if (!mapping.is_valid()) {
    mxdebug_if(s_debug_dovi_configuration_record, fmt::format("hevc_es_video_packetizer_c::handle_dovi_block_addition_mappings: no existing mapping found but could not create a new one!?\n"));
    return;
  }

  if (s_debug_dovi_configuration_record) {
    mxdebug(fmt::format("hevc_es_video_packetizer_c::handle_dovi_block_addition_mappings: no existing mapping found; creating new one; dumping DOVI RPU header & configuration record\n"));
    hdr.dump();
    dovi_config_record.dump();
  }

  m_block_addition_mappings.push_back(mapping);
  set_block_addition_mappings(m_block_addition_mappings);
}

void
hevc_es_video_packetizer_c::handle_delayed_headers() {
  handle_dovi_block_addition_mappings();

  avc_hevc_es_video_packetizer_c::handle_delayed_headers();
}

connection_result_e
hevc_es_video_packetizer_c::can_connect_to(generic_packetizer_c *src,
                                           std::string &error_message) {
  auto *vsrc = dynamic_cast<hevc_es_video_packetizer_c *>(src);
  if (!vsrc)
    return CAN_CONNECT_NO_FORMAT;

  connect_check_v_width( m_width,  vsrc->m_width);
  connect_check_v_height(m_height, vsrc->m_height);

  return CAN_CONNECT_YES;
}
