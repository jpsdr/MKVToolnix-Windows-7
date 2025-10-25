/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   IO callback class definitions

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include <ebml/IOCallback.h>

#include "common/mm_io_fwd.h"

class charset_converter_c;
class mm_io_private_c;

class mm_io_c: public libebml::IOCallback {
protected:
  MTX_DECLARE_PRIVATE(mm_io_private_c)

  std::unique_ptr<mm_io_private_c> const p_ptr;

  explicit mm_io_c(mm_io_private_c &p);

public:
  mm_io_c();
  virtual ~mm_io_c();

  virtual uint64_t getFilePointer() override = 0;
  virtual void setFilePointer(int64_t offset, libebml::seek_mode mode = libebml::seek_beginning) override = 0;
  virtual bool setFilePointer2(int64_t offset, libebml::seek_mode mode = libebml::seek_beginning);
  virtual memory_cptr read(size_t size);
  virtual MTX_EBML_IOCALLBACK_READ_RETURN_TYPE read(void *buffer, size_t size) override;
  virtual uint32_t read(std::string &buffer, size_t size, size_t offset = 0);
  virtual uint32_t read(memory_cptr const &buffer, size_t size, int offset = 0);
  virtual uint8_t read_uint8();
  virtual uint16_t read_uint16_le();
  virtual uint32_t read_uint24_le();
  virtual uint32_t read_uint32_le();
  virtual uint64_t read_uint64_le();
  virtual uint16_t read_uint16_be();
  virtual int32_t read_int24_be();
  virtual uint32_t read_uint24_be();
  virtual uint32_t read_uint32_be();
  virtual uint64_t read_uint64_be();
  virtual double read_double();
  virtual unsigned int read_mp4_descriptor_len();
  virtual int write_uint8(uint8_t value);
  virtual int write_uint16_le(uint16_t value);
  virtual int write_uint32_le(uint32_t value);
  virtual int write_uint64_le(uint64_t value);
  virtual int write_uint16_be(uint16_t value);
  virtual int write_uint32_be(uint32_t value);
  virtual int write_uint64_be(uint64_t value);
  virtual int write_double(double value);
  virtual void skip(int64_t numbytes);
  virtual size_t write(const void *buffer, size_t size) override;
  virtual size_t write(std::string const &buffer);
  virtual size_t write(const memory_cptr &buffer, size_t size = UINT_MAX, size_t offset = 0);
  virtual bool eof() = 0;
  virtual void clear_eof() { }
  virtual void flush() {
  }
  virtual int truncate(int64_t) {
    return 0;
  }

  virtual std::string get_file_name() const = 0;

  virtual std::string getline(std::optional<std::size_t> max_chars = std::nullopt);
  virtual bool getline2(std::string &s, std::optional<std::size_t> max_chars = std::nullopt);
  virtual size_t puts(const std::string &s);
  virtual bool write_bom(const std::string &charset);
  virtual bool bom_written() const;
  virtual int getch();

  virtual void save_pos(int64_t new_pos = -1);
  virtual bool restore_pos();

  virtual int64_t get_size();

  virtual void close() override = 0;

  virtual void set_string_output_converter(std::shared_ptr<charset_converter_c> const &converter);
  virtual void use_dos_style_newlines(bool yes);

  virtual void enable_buffering(bool /* enable */) {
  }

protected:
  virtual uint32_t _read(void *buffer, size_t size) = 0;
  virtual size_t _write(const void *buffer, size_t size) = 0;

public:
  static void disable_writing_byte_order_markers();
};
