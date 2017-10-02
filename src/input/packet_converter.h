/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   class definitions for a generic packet converter class

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#include "merge/packet.h"

class packet_t;

class packet_converter_c {
protected:
  generic_packetizer_c *m_ptzr;

public:
  packet_converter_c(generic_packetizer_c *ptzr)
    : m_ptzr{ptzr}
  {}

  virtual ~packet_converter_c() {}

  virtual bool convert(packet_cptr const &packet) = 0;
  virtual void flush() {}

  virtual void set_packetizer(generic_packetizer_c *ptzr) {
    m_ptzr = ptzr;
  }
};
using packet_converter_cptr = std::shared_ptr<packet_converter_c>;
