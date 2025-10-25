/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definition for the PCM output module

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/byte_buffer.h"
#include "common/debugging.h"
#include "common/samples_to_timestamp_converter.h"
#include "merge/generic_packetizer.h"

class pcm_packetizer_c: public generic_packetizer_c {
public:
  enum pcm_format_e {
    little_endian_integer = 0,
    big_endian_integer    = 1,
    ieee_float            = 2
  };

private:
  int m_samples_per_sec, m_channels, m_bits_per_sample, m_samples_per_packet;
  std::size_t m_packet_size, m_samples_output;
  debugging_option_c m_debug{"pcm_packetizer"};
  pcm_format_e m_format;
  mtx::bytes::buffer_c m_buffer;
  samples_to_timestamp_converter_c m_s2ts;
  std::function<void(uint8_t const *, uint8_t *, std::size_t)> m_byte_swapper;

public:
  pcm_packetizer_c(generic_reader_c *p_reader, track_info_c &p_ti, int p_samples_per_sec, int channels, int bits_per_sample, pcm_format_e format = little_endian_integer);
  virtual ~pcm_packetizer_c();

  virtual void set_headers();

  virtual translatable_string_c get_format_name() const {
    return YT("PCM");
  }
  virtual connection_result_e can_connect_to(generic_packetizer_c *src, std::string &error_message);

protected:
  virtual void process_impl(packet_cptr const &packet) override;
  virtual void flush_impl();
  virtual void flush_packets();
  virtual int64_t size_to_samples(int64_t size) const;
  virtual int64_t samples_to_size(int64_t size) const;
  virtual void byte_swap_data(memory_c &data) const;
};
