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

class mm_text_io_private_c;
class mm_text_io_c: public mm_proxy_io_c {
protected:
  MTX_DECLARE_PRIVATE(mm_text_io_private_c)

  explicit mm_text_io_c(mm_text_io_private_c &p);

public:
  mm_text_io_c(mm_io_cptr const &in);

  virtual void setFilePointer(int64_t offset, libebml::seek_mode mode=libebml::seek_beginning) override;
  virtual std::string getline(std::optional<std::size_t> max_chars = std::nullopt) override;
  virtual std::string read_next_codepoint();
  virtual byte_order_mark_e get_byte_order_mark() const;
  virtual unsigned int get_byte_order_length() const;
  virtual void set_byte_order_mark(byte_order_mark_e byte_order_mark);
  virtual std::optional<std::string> get_encoding() const;

protected:
  virtual void detect_eol_style();

public:
  static bool has_byte_order_marker(const std::string &string);
  static bool detect_byte_order_marker(const uint8_t *buffer, unsigned int size, byte_order_mark_e &byte_order_mark, unsigned int &bom_length);
  static std::optional<std::string> get_encoding(byte_order_mark_e byte_order_mark);
};
