/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definitions for the AAC framing type converter

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/aac.h"
#include "input/packet_converter.h"

class aac_framing_packet_converter_c: public packet_converter_c {
protected:
  mtx::aac::parser_c m_parser;

public:
  aac_framing_packet_converter_c(generic_packetizer_c *packetizer, mtx::aac::parser_c::multiplex_type_e multiplex_type);
  virtual ~aac_framing_packet_converter_c() {};

  virtual bool convert(packet_cptr const &packet);
};
