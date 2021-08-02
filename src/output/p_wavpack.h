/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definition for the TTA output module

   Written by Steve Lhomme <steve.lhomme@free.fr>.
*/

#pragma once

#include "common/common_pch.h"

#include "merge/generic_packetizer.h"
#include "common/wavpack.h"

class wavpack_packetizer_c: public generic_packetizer_c {
private:
  int m_channels, m_sample_rate, m_bits_per_sample;
  int64_t m_samples_per_block;
  int64_t m_samples_output;
  bool m_has_correction;
  mtx_mp_rational_t m_sample_duration;

public:
  wavpack_packetizer_c(generic_reader_c *p_reader, track_info_c &p_ti, mtx::wavpack::meta_t &meta);

  virtual void set_headers();

  virtual translatable_string_c get_format_name() const {
    return YT("WAVPACK4");
  }
  virtual connection_result_e can_connect_to(generic_packetizer_c *src, std::string &error_message);

protected:
  virtual void process_impl(packet_cptr const &packet) override;
};
