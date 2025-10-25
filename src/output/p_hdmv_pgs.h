/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definition for the PGS output module

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/compression.h"
#include "merge/generic_packetizer.h"

class hdmv_pgs_packetizer_c: public generic_packetizer_c {
protected:
  bool m_aggregate_packets;
  packet_cptr m_aggregated;
  debugging_option_c m_debug{"hdmv_pgs|hdmv_pgs_packetizer"};

public:
  hdmv_pgs_packetizer_c(generic_reader_c *p_reader, track_info_c &p_ti);
  virtual ~hdmv_pgs_packetizer_c();

  virtual void set_headers();
  virtual void set_aggregate_packets(bool aggregate_packets) {
    m_aggregate_packets = aggregate_packets;
  }

  virtual translatable_string_c get_format_name() const {
    return YT("HDMV PGS");
  }
  virtual connection_result_e can_connect_to(generic_packetizer_c *src, std::string &error_message);

protected:
  virtual void process_impl(packet_cptr const &packet) override;
  void dump_and_add_packet(packet_cptr const &packet);
  void dump_packet(packet_t const &packet);
};
