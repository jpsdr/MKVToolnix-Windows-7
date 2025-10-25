/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definition for the AC-3 output module

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/ac3.h"
#include "merge/generic_packetizer.h"
#include "merge/stream_property_preserver.h"
#include "merge/timestamp_calculator.h"

class ac3_packetizer_c: public generic_packetizer_c {
protected:
  mtx::ac3::frame_c m_first_ac3_header;
  mtx::ac3::parser_c m_parser;
  timestamp_calculator_c m_timestamp_calculator;
  stream_property_preserver_c<timestamp_c> m_discard_padding;
  int64_t m_samples_per_packet, m_packet_duration;
  uint64_t m_stream_position;
  bool m_first_packet, m_remove_dialog_normalization_gain;
  std::vector<packet_extension_cptr> m_packet_extensions;
  debugging_option_c m_verify_checksums;

public:
  ac3_packetizer_c(generic_reader_c *p_reader, track_info_c &p_ti, int samples_per_sec, int channels, int bsid);
  virtual ~ac3_packetizer_c();

  virtual void flush_packets();
  virtual void set_headers();

  virtual translatable_string_c get_format_name() const {
    return YT("AC-3");
  }

  virtual connection_result_e can_connect_to(generic_packetizer_c *src, std::string &error_message);

protected:
  virtual void add_to_buffer(uint8_t *const buf, int size);
  virtual void adjust_header_values(mtx::ac3::frame_c const &ac3_header);
  virtual mtx::ac3::frame_c get_frame();
  virtual void flush_impl();
  virtual void process_impl(packet_cptr const &packet) override;
  virtual void set_timestamp_and_add_packet(packet_cptr const &packet, uint64_t packet_stream_position);
};

class ac3_bs_packetizer_c: public ac3_packetizer_c {
protected:
  uint8_t m_bsb;
  bool m_bsb_present;

public:
  ac3_bs_packetizer_c(generic_reader_c *p_reader, track_info_c &p_ti, unsigned long samples_per_sec, int channels, int bsid);

protected:
  virtual void add_to_buffer(uint8_t *const buf, int size);
};
