/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   class definitions for a generic packet converter class

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#ifndef MTX_INPUT_PACKET_CONVERTER_H
#define MTX_INPUT_PACKET_CONVERTER_H

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
};
typedef std::shared_ptr<packet_converter_c> packet_converter_cptr;

#endif  // MTX_INPUT_PACKET_CONVERTER_H
