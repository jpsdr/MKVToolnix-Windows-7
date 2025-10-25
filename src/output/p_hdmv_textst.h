/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definition for the HDMV TextST output module

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/compression.h"
#include "merge/generic_packetizer.h"

class hdmv_textst_packetizer_c: public generic_packetizer_c {
public:
  hdmv_textst_packetizer_c(generic_reader_c *p_reader, track_info_c &p_ti, memory_cptr const &dialog_style_segment);
  virtual ~hdmv_textst_packetizer_c();

  virtual void set_headers();

  virtual translatable_string_c get_format_name() const {
    return YT("HDMV TextST");
  }
  virtual connection_result_e can_connect_to(generic_packetizer_c *src, std::string &error_message);

protected:
  virtual void process_impl(packet_cptr const &packet) override;
};
