/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definitions for the TrueHD/AC-3 splitting converter

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/truehd.h"
#include "input/packet_converter.h"

class truehd_ac3_splitting_packet_converter_c: public packet_converter_c {
protected:
  mtx::truehd::parser_c m_parser;
  generic_packetizer_c *m_ac3_ptzr;
  int64_t m_truehd_timestamp, m_ac3_timestamp;

public:
  truehd_ac3_splitting_packet_converter_c(generic_packetizer_c *truehd_ptzr = nullptr, generic_packetizer_c *ac3_ptzr = nullptr);
  virtual ~truehd_ac3_splitting_packet_converter_c() {};

  virtual bool convert(packet_cptr const &packet);
  virtual void set_ac3_packetizer(generic_packetizer_c *ac3_ptzr);
  virtual void flush();

protected:
  virtual void process_frames();
};

using truehd_ac3_splitting_packet_converter_cptr = std::shared_ptr<truehd_ac3_splitting_packet_converter_c>;
