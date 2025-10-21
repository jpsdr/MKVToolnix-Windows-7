/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definitions for the Timed Text (tx3g) to text/UTF-8 converter

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "input/packet_converter.h"

class timed_text_to_text_utf8_converter_c: public packet_converter_c {
public:
  timed_text_to_text_utf8_converter_c(generic_packetizer_c &packetizer);
  virtual ~timed_text_to_text_utf8_converter_c() {};

  virtual bool convert(packet_cptr const &packet);
};
