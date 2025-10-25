/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definition for the Kate packetizer

   Written by ogg.k.ogg.k <ogg.k.ogg.k@googlemail.com>.
   Adapted from code by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "merge/generic_packetizer.h"
#include "common/kate.h"

class kate_packetizer_c: public generic_packetizer_c {
private:
  std::vector<memory_cptr> m_headers;

  mtx::kate::identification_header_t m_kate_id;

  int64_t m_previous_timestamp;
  mtx_mp_rational_t m_frame_duration;

public:
  kate_packetizer_c(generic_reader_c *reader, track_info_c &ti);
  virtual ~kate_packetizer_c();

  virtual void set_headers();

  virtual translatable_string_c get_format_name() const {
    return YT("Kate");
  }
  virtual connection_result_e can_connect_to(generic_packetizer_c *src, std::string &error_message);

protected:
  virtual void process_impl(packet_cptr const &packet) override;
};
