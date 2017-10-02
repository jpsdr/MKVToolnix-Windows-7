/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   class definitions for the DVB subtitling PES framing removal converter

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#include "input/packet_converter.h"

class dvbsub_pes_framing_removal_packet_converter_c: public packet_converter_c {
protected:
  packet_cptr m_packet;

public:
  dvbsub_pes_framing_removal_packet_converter_c(generic_packetizer_c *ptzr);
  virtual ~dvbsub_pes_framing_removal_packet_converter_c();

  virtual void flush() override;
  virtual bool convert(packet_cptr const &packet) override;
};
