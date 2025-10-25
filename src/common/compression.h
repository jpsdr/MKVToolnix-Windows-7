/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Helper routines for various compression libs

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include <matroska/KaxSemantic.h>

/* compression types */
enum compression_method_e {
  COMPRESSION_UNSPECIFIED = 0,
  COMPRESSION_ZLIB,
  COMPRESSION_HEADER_REMOVAL,
  COMPRESSION_MPEG4_P2,
  COMPRESSION_MPEG4_P10,
  COMPRESSION_DIRAC,
  COMPRESSION_DTS,
  COMPRESSION_AC3,
  COMPRESSION_MP3,
  COMPRESSION_ANALYZE_HEADER_REMOVAL,
  COMPRESSION_NONE,
  COMPRESSION_NUM = COMPRESSION_NONE
};

namespace mtx {
  class compression_x: public exception {
  protected:
    std::string m_message;
  public:
    compression_x(const std::string &message) : m_message{message} { }
    virtual ~compression_x() throw() { }

    virtual const char *what() const throw() {
      return m_message.c_str();
    }
  };
}

class compressor_c;
using compressor_ptr = std::shared_ptr<compressor_c>;

class compressor_c {
protected:
  compression_method_e method{COMPRESSION_UNSPECIFIED};
  int64_t raw_size{}, compressed_size{}, items{};
  debugging_option_c m_debug{"compressor|compression"};

public:
  compressor_c(compression_method_e method_)
    : method{method_}
  {
  }

  virtual ~compressor_c();

  compression_method_e get_method() {
    return method;
  }


  virtual memory_cptr compress(memory_cptr const &buffer) {
    return do_compress(buffer->get_buffer(), buffer->get_size());
  }

  virtual memory_cptr compress(uint8_t const *buffer,
                               std::size_t size) {
    return do_compress(buffer, size);
  }

  virtual memory_cptr decompress(memory_cptr const &buffer) {
    return do_decompress(buffer->get_buffer(), buffer->get_size());
  }

  virtual memory_cptr decompress(uint8_t const *buffer,
                                 std::size_t size) {
    return do_decompress(buffer, size);
  }

  virtual void set_track_headers(libmatroska::KaxContentEncoding &c_encoding);

  static compressor_ptr create(compression_method_e method);
  static compressor_ptr create(const char *method);
  static compressor_ptr create_from_file_name(std::string const &file_name);

protected:
  virtual memory_cptr do_compress(uint8_t const *buffer,
                                  std::size_t size) {
    return memory_c::clone(buffer, size);
  }
  virtual memory_cptr do_decompress(uint8_t const *buffer,
                                    std::size_t size) {
    return memory_c::clone(buffer, size);
  }
};

#include "common/compression/header_removal.h"
#include "common/compression/zlib.h"
