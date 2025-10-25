/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definitions for the DVB subtitling PES framing removal converter

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "input/packet_converter.h"

class dvbsub_pes_framing_removal_packet_converter_c: public packet_converter_c {
protected:
  packet_cptr m_packet;

public:
  dvbsub_pes_framing_removal_packet_converter_c(generic_packetizer_c *packetizer);
  virtual ~dvbsub_pes_framing_removal_packet_converter_c();

  virtual void flush() override;
  virtual bool convert(packet_cptr const &packet) override;
};
