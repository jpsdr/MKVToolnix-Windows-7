/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   zlib compressor

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include <zlib.h>

#include "common/compression.h"

class zlib_compressor_c: public compressor_c {
public:
  zlib_compressor_c();
  virtual ~zlib_compressor_c();

protected:
  virtual memory_cptr do_compress(uint8_t const *buffer, std::size_t size) override;
  virtual memory_cptr do_decompress(uint8_t const *buffer, std::size_t size) override;
};
