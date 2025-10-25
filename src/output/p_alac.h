/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definition for the ALAC output module

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/alac.h"
#include "merge/generic_packetizer.h"

class alac_packetizer_c: public generic_packetizer_c {
private:
  memory_cptr m_magic_cookie;
  unsigned int m_sample_rate, m_channels;

public:
  alac_packetizer_c(generic_reader_c *p_reader, track_info_c &p_ti, memory_cptr const &magic_cookie, unsigned int sample_rate, unsigned int channels);
  virtual ~alac_packetizer_c();

  virtual translatable_string_c get_format_name() const {
    return YT("ALAC");
  }

  virtual connection_result_e can_connect_to(generic_packetizer_c *src, std::string &error_message);

protected:
  virtual void process_impl(packet_cptr const &packet) override;
};
