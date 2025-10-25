/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definition for the DTS output module

   Written by Peter Niemayer <niemayer@isg.de>.
   Modified by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/byte_buffer.h"
#include "common/dts.h"
#include "merge/generic_packetizer.h"
#include "merge/stream_property_preserver.h"
#include "merge/timestamp_calculator.h"

class dts_packetizer_c: public generic_packetizer_c {
private:
  using header_and_packet_t = std::tuple<mtx::dts::header_t, memory_cptr, uint64_t>;

  mtx::bytes::buffer_c m_packet_buffer;

  mtx::dts::header_t m_first_header, m_previous_header;
  bool m_skipping_is_normal, m_reduce_to_core, m_remove_dialog_normalization_gain;
  timestamp_calculator_c m_timestamp_calculator;
  stream_property_preserver_c<timestamp_c> m_discard_padding;
  std::deque<header_and_packet_t> m_queued_packets;
  uint64_t m_stream_position, m_packet_position;

public:
  dts_packetizer_c(generic_reader_c *p_reader, track_info_c &p_ti, mtx::dts::header_t const &dts_header);
  virtual ~dts_packetizer_c();

  virtual void set_headers();
  virtual void set_skipping_is_normal(bool skipping_is_normal) {
    m_skipping_is_normal = skipping_is_normal;
  }
  virtual translatable_string_c get_format_name() const {
    return YT("DTS");
  }

  virtual connection_result_e can_connect_to(generic_packetizer_c *src, std::string &error_message);

protected:
  virtual void process_impl(packet_cptr const &packet) override;
  virtual void flush_impl();

private:
  virtual header_and_packet_t get_dts_packet(bool flushing);
  virtual void queue_available_packets(bool flushing);
  virtual void process_available_packets();
};
