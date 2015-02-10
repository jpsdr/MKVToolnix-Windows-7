/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   class definitions for the TrueHD/AC3 splitting converter

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#ifndef MTX_INPUT_TRUEHD_AC3_SPLITTING_PACKET_CONVERTER_H
#define MTX_INPUT_TRUEHD_AC3_SPLITTING_PACKET_CONVERTER_H

#include "common/common_pch.h"

#include "common/truehd.h"
#include "input/packet_converter.h"

class truehd_ac3_splitting_packet_converter_c: public packet_converter_c {
protected:
  truehd_parser_c m_parser;
  generic_packetizer_c *m_ac3_ptzr;

public:
  truehd_ac3_splitting_packet_converter_c(generic_packetizer_c *truehd_ptzr = nullptr, generic_packetizer_c *ac3_ptzr = nullptr);
  virtual ~truehd_ac3_splitting_packet_converter_c() {};

  virtual bool convert(packet_cptr const &packet);
  virtual void set_ac3_packetizer(generic_packetizer_c *ac3_ptzr);
  virtual void flush();

protected:
  virtual void process_frames(int64_t timecode = -1);
};

typedef std::shared_ptr<truehd_ac3_splitting_packet_converter_c> truehd_ac3_splitting_packet_converter_cptr;

#endif  // MTX_INPUT_TRUEHD_AC3_SPLITTING_PACKET_CONVERTER_H
