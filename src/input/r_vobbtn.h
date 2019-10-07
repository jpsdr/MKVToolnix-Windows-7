/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   class definitions for the Vob Buttons stream reader

   Written by Steve Lhomme <steve.lhomme@free.fr>.
   Modified by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#include "merge/generic_reader.h"
#include "output/p_vobbtn.h"

class vobbtn_reader_c: public generic_reader_c {
private:
  uint16_t width, height;
  unsigned char chunk[0x400];

public:
  virtual mtx::file_type_e get_format_type() const {
    return mtx::file_type_e::vobbtn;
  }

  virtual void read_headers();
  virtual void identify();
  virtual void create_packetizer(int64_t tid);

  virtual bool probe_file() override;

protected:
  virtual file_status_e read(generic_packetizer_c *ptzr, bool force = false) override;
};
