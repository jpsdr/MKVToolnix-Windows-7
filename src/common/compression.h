/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   Helper routines for various compression libs

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#include <matroska/KaxContentEncoding.h>

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
    compression_x(const std::string &message)  : m_message(message)       { }
    compression_x(const boost::format &message): m_message(message.str()) { }
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
  compression_method_e method;
  int64_t raw_size, compressed_size, items;

public:
  compressor_c(compression_method_e n_method):
    method(n_method), raw_size(0), compressed_size(0), items(0) {
  };

  virtual ~compressor_c();

  compression_method_e get_method() {
    return method;
  }


  virtual memory_cptr compress(memory_cptr const &buffer) {
    return do_compress(buffer->get_buffer(), buffer->get_size());
  }

  virtual memory_cptr compress(unsigned char const *buffer,
                               std::size_t size) {
    return do_compress(buffer, size);
  }

  virtual memory_cptr decompress(memory_cptr const &buffer) {
    return do_decompress(buffer->get_buffer(), buffer->get_size());
  }

  virtual memory_cptr decompress(unsigned char const *buffer,
                                 std::size_t size) {
    return do_decompress(buffer, size);
  }

  virtual void set_track_headers(KaxContentEncoding &c_encoding);

  static compressor_ptr create(compression_method_e method);
  static compressor_ptr create(const char *method);
  static compressor_ptr create_from_file_name(std::string const &file_name);

protected:
  virtual memory_cptr do_compress(unsigned char const *buffer,
                                  std::size_t size) {
    return memory_c::clone(buffer, size);
  }
  virtual memory_cptr do_decompress(unsigned char const *buffer,
                                    std::size_t size) {
    return memory_c::clone(buffer, size);
  }
};

#include "common/compression/header_removal.h"
#include "common/compression/zlib.h"
