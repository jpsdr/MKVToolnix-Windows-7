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
#include "merge/block_addition_mapping.h"
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
  auto el_configuration_record = m_parser.get_dovi_enhancement_layer_configuration_record();

  mxdebug_if(s_debug_dovi_configuration_record, fmt::format("hevc_es_video_packetizer_c::handle_dovi_block_addition_mappings: parser headers parsed: {0}; has DOVI RPU: {1}; has enhancement layer configuration record: {2}\n", m_parser.headers_parsed(), m_parser.has_dovi_rpu_header(), !!el_configuration_record));

  if (!m_parser.has_dovi_rpu_header())
    return;

  if (!handle_dovi_configuration_record())
    return;

  handle_dovi_enhancement_layer_configuration_record();

  if (!m_block_addition_mappings.empty())
    set_block_addition_mappings(m_block_addition_mappings);
}

bool
hevc_es_video_packetizer_c::handle_dovi_configuration_record() {
  if (have_block_addition_mapping_type(fourcc_c{"dvcC"}.value()) || have_block_addition_mapping_type(fourcc_c{"dvvC"}.value())) {
    mxdebug_if(s_debug_dovi_configuration_record, fmt::format("hevc_es_video_packetizer_c::handle_dovi_configuration_record: have addition mapping of type dvcC or dvvC already\n"));
    return true;
  }

  auto hdr                = m_parser.get_dovi_rpu_header();
  auto vui                = m_parser.get_vui_info();
  auto duration           = m_parser.has_stream_default_duration() ? m_parser.get_stream_default_duration() : m_parser.get_most_often_used_duration();

  auto dovi_config_record = create_dovi_configuration_record(hdr, m_width, m_height, vui, duration);

  auto mapping            = mtx::dovi::create_dovi_block_addition_mapping(dovi_config_record);

  if (!mapping.is_valid()) {
    mxdebug_if(s_debug_dovi_configuration_record, fmt::format("hevc_es_video_packetizer_c::handle_dovi_configuration_record: no existing mapping found but could not create a new one!?\n"));
    return false;
  }

  if (s_debug_dovi_configuration_record) {
    mxdebug(fmt::format("hevc_es_video_packetizer_c::handle_dovi_configuration_record: no existing mapping found; creating new one; dumping DOVI RPU header & configuration record\n"));
    hdr.dump();
    dovi_config_record.dump();
  }

  m_block_addition_mappings.push_back(mapping);

  return true;
}

void
hevc_es_video_packetizer_c::handle_dovi_enhancement_layer_configuration_record() {
  auto el_configuration_record = m_parser.get_dovi_enhancement_layer_configuration_record();

  if (!el_configuration_record)
    return;

  if (have_block_addition_mapping_type(fourcc_c{"hvcE"}.value())) {
    mxdebug_if(s_debug_dovi_configuration_record, fmt::format("hevc_es_video_packetizer_c::handle_dovi_enhancement_layer_configuration_record: have addition mapping of type hvcE already\n"));
    return;
  }

  mxdebug_if(s_debug_dovi_configuration_record, fmt::format("hevc_es_video_packetizer_c::handle_dovi_enhancement_layer_configuration_record: no existing mapping found; creating new one\n"));

  block_addition_mapping_t mapping{};

  mapping.id_name       = "Dolby Vision enhancement-layer HEVC configuration";
  mapping.id_type       = fourcc_c{"hvcE"}.value();
  mapping.id_extra_data = el_configuration_record->clone();

  m_block_addition_mappings.push_back(mapping);
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
