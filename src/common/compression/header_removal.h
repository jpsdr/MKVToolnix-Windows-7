/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   header removal compressor

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/compression.h"

class header_removal_compressor_c: public compressor_c {
protected:
  memory_cptr m_bytes;

public:
  header_removal_compressor_c();

  virtual void set_bytes(memory_cptr const &bytes) {
    m_bytes = bytes;
    m_bytes->take_ownership();
  }

  virtual memory_cptr do_compress(uint8_t const *buffer, std::size_t size) override;
  virtual memory_cptr do_decompress(uint8_t const *buffer, std::size_t size) override;

  virtual void set_track_headers(libmatroska::KaxContentEncoding &c_encoding);
};

class analyze_header_removal_compressor_c: public compressor_c {
protected:
  memory_cptr m_bytes;
  unsigned int m_packet_counter;

public:
  analyze_header_removal_compressor_c();
  virtual ~analyze_header_removal_compressor_c();

  virtual memory_cptr do_compress(uint8_t const *buffer, std::size_t size) override;
  virtual memory_cptr do_decompress(uint8_t const *buffer, std::size_t size) override;

  virtual void set_track_headers(libmatroska::KaxContentEncoding &c_encoding);
};

class mpeg4_p2_compressor_c: public header_removal_compressor_c {
public:
  mpeg4_p2_compressor_c();
};

class mpeg4_p10_compressor_c: public header_removal_compressor_c {
public:
  mpeg4_p10_compressor_c();
};

class dirac_compressor_c: public header_removal_compressor_c {
public:
  dirac_compressor_c();
};

class dts_compressor_c: public header_removal_compressor_c {
public:
  dts_compressor_c();
};

class ac3_compressor_c: public header_removal_compressor_c {
public:
  ac3_compressor_c();
};

class mp3_compressor_c: public header_removal_compressor_c {
public:
  mp3_compressor_c();
};
