/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   class definition for the AAC output module

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/aac.h"
#include "merge/generic_packetizer.h"
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
  int64_t m_packet_duration;
  bool m_first_packet;

public:
  aac_packetizer_c(generic_reader_c *p_reader, track_info_c &p_ti, mtx::aac::audio_config_t const &config, mode_e mode);
  virtual ~aac_packetizer_c();

  virtual int process(packet_cptr packet);
  virtual void set_headers();

  virtual translatable_string_c get_format_name() const {
    return YT("AAC");
  }

  virtual connection_result_e can_connect_to(generic_packetizer_c *src, std::string &error_message);

private:
  virtual int process_headerless(packet_cptr packet);
  virtual void handle_parsed_audio_config();
};
