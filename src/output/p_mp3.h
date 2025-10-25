/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definition for the MP3 output module

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/byte_buffer.h"
#include "common/mp3.h"
#include "merge/generic_packetizer.h"
#include "merge/stream_property_preserver.h"
#include "merge/timestamp_calculator.h"

class mp3_packetizer_c: public generic_packetizer_c {
private:
  bool m_first_packet;
  int64_t m_bytes_skipped;
  int m_samples_per_sec, m_channels, m_samples_per_frame;
  mtx::bytes::buffer_c m_byte_buffer;
  bool m_codec_id_set, m_valid_headers_found;
  timestamp_calculator_c m_timestamp_calculator;
  stream_property_preserver_c<timestamp_c> m_discard_padding;
  int64_t m_packet_duration;
  std::vector<packet_extension_cptr> m_packet_extensions;

public:
  mp3_packetizer_c(generic_reader_c *p_reader, track_info_c &p_ti, int samples_per_sec, int channels, bool source_is_good);
  virtual ~mp3_packetizer_c();

  virtual void set_headers();

  virtual translatable_string_c get_format_name() const {
    return YT("MPEG-1/2 Audio Layer II/III");
  }
  virtual connection_result_e can_connect_to(generic_packetizer_c *src, std::string &error_message);

protected:
  virtual void process_impl(packet_cptr const &packet) override;
  virtual memory_cptr get_mp3_packet(mp3_header_t *mp3header);

  virtual void handle_garbage(int64_t bytes);
  virtual void flush_packets();
};
