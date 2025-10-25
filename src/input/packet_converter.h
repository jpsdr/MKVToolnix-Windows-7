/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definitions for a generic packet converter class

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "merge/packet.h"

class packet_t;

class packet_converter_c {
protected:
  generic_packetizer_c *m_ptzr;
  uint16_t m_current_pid{};

public:
  packet_converter_c(generic_packetizer_c *packetizer)
    : m_ptzr{packetizer}
  {}

  virtual ~packet_converter_c() {}

  virtual bool convert(packet_cptr const &packet) = 0;
  virtual bool convert_for_pid(packet_cptr const &packet, uint16_t pid);
  virtual void flush() {}

  virtual void set_packetizer(generic_packetizer_c *packetizer) {
    m_ptzr = packetizer;
  }
};
using packet_converter_cptr = std::shared_ptr<packet_converter_c>;
