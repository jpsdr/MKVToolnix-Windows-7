/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definition for the RealAudio output module

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "merge/generic_packetizer.h"

class ra_packetizer_c: public generic_packetizer_c {
private:
  int m_samples_per_sec, m_channels, m_bits_per_sample;
  uint32_t m_fourcc;

public:
  ra_packetizer_c(generic_reader_c *p_reader, track_info_c &p_ti, int samples_per_sec, int channels, int bits_per_sample, uint32_t fourcc);
  virtual ~ra_packetizer_c();

  virtual void set_headers();

  virtual translatable_string_c get_format_name() const {
    return YT("RealAudio");
  }
  virtual connection_result_e can_connect_to(generic_packetizer_c *src, std::string &error_message);

protected:
  virtual void process_impl(packet_cptr const &packet) override;
};
