/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definition for the AAC output module

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/aac.h"
#include "merge/generic_packetizer.h"
#include "merge/stream_property_preserver.h"
#include "merge/timestamp_calculator.h"

class aac_packetizer_c: public generic_packetizer_c {
public:
  enum mode_e {
    with_headers,
    headerless,
  };

private:
  mtx::aac::audio_config_t m_config;
  mode_e m_mode;
  mtx::aac::parser_c m_parser;
  timestamp_calculator_c m_timestamp_calculator;
  stream_property_preserver_c<timestamp_c> m_discard_padding;
  int64_t m_packet_duration;
  bool m_first_packet;

public:
  aac_packetizer_c(generic_reader_c *p_reader, track_info_c &p_ti, mtx::aac::audio_config_t const &config, mode_e mode);
  virtual ~aac_packetizer_c();

  virtual void set_headers();

  virtual translatable_string_c get_format_name() const {
    return YT("AAC");
  }

  virtual connection_result_e can_connect_to(generic_packetizer_c *src, std::string &error_message);

protected:
  virtual void process_impl(packet_cptr const &packet) override;
  virtual void process_headerless(packet_cptr const &packet);
  virtual void handle_parsed_audio_config();
};
