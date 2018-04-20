 /*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   IO callback class implementation

   Written by Moritz Bunkus <moritz@bunkus.org>.
   Modifications by Peter Niemayer <niemayer@isg.de>.
*/

#include "common/common_pch.h"

#include "common/mm_io_x.h"
#include "common/mm_proxy_io.h"
#include "common/mm_text_io.h"

/*
   Class for handling UTF-8/UTF-16/UTF-32 text files.
*/

mm_text_io_c::mm_text_io_c(mm_io_cptr const &in)
  : mm_proxy_io_c{in}
  , m_byte_order(BO_NONE)
  , m_bom_len(0)
  , m_uses_carriage_returns(false)
  , m_uses_newlines(false)
  , m_eol_style_detected(false)
{
  in->setFilePointer(0, seek_beginning);

  unsigned char buffer[4];
  int num_read = in->read(buffer, 4);
  if (2 > num_read) {
    in->setFilePointer(0, seek_beginning);
    return;
  }

  detect_byte_order_marker(buffer, num_read, m_byte_order, m_bom_len);

  in->setFilePointer(m_bom_len, seek_beginning);
}

void
mm_text_io_c::detect_eol_style() {
  if (m_eol_style_detected)
    return;

  m_eol_style_detected = true;
  bool found_cr_or_nl  = false;

  save_pos();

  auto num_chars_read = 0u;

  while (1) {
    auto utf8char = read_next_codepoint();
    auto len      = utf8char.length();

    if (!len)
      break;

    ++num_chars_read;

    if ((1 == len) && ('\r' == utf8char[0])) {
      found_cr_or_nl          = true;
      m_uses_carriage_returns = true;

    } else if ((1 == len) && ('\n' == utf8char[0])) {
      found_cr_or_nl  = true;
      m_uses_newlines = true;

    } else if (found_cr_or_nl)
      break;

    else if (num_chars_read > 1000) {
      m_uses_carriage_returns = false;
      m_uses_newlines         = true;
      break;
    }
  }

  restore_pos();
}

bool
mm_text_io_c::detect_byte_order_marker(const unsigned char *buffer,
                                       unsigned int size,
                                       byte_order_e &byte_order,
                                       unsigned int &bom_length) {
  if ((3 <= size) && (buffer[0] == 0xef) && (buffer[1] == 0xbb) && (buffer[2] == 0xbf)) {
    byte_order = BO_UTF8;
    bom_length = 3;
  } else if ((4 <= size) && (buffer[0] == 0xff) && (buffer[1] == 0xfe) && (buffer[2] == 0x00) && (buffer[3] == 0x00)) {
    byte_order = BO_UTF32_LE;
    bom_length = 4;
  } else if ((4 <= size) && (buffer[0] == 0x00) && (buffer[1] == 0x00) && (buffer[2] == 0xfe) && (buffer[3] == 0xff)) {
    byte_order = BO_UTF32_BE;
    bom_length = 4;
  } else if ((2 <= size) && (buffer[0] == 0xff) && (buffer[1] == 0xfe)) {
    byte_order = BO_UTF16_LE;
    bom_length = 2;
  } else if ((2 <= size) && (buffer[0] == 0xfe) && (buffer[1] == 0xff)) {
    byte_order = BO_UTF16_BE;
    bom_length = 2;
  } else {
    byte_order = BO_NONE;
    bom_length = 0;
  }

  return BO_NONE != byte_order;
}

bool
mm_text_io_c::has_byte_order_marker(const std::string &string) {
  byte_order_e byte_order;
  unsigned int bom_length;
  return detect_byte_order_marker(reinterpret_cast<const unsigned char *>(string.c_str()), string.length(), byte_order, bom_length);
}

boost::optional<std::string>
mm_text_io_c::get_encoding(byte_order_e byte_order) {
  if (BO_NONE == byte_order)
    return {};

  return BO_UTF8     == byte_order ? "UTF-8"s
       : BO_UTF16_LE == byte_order ? "UTF-16LE"s
       : BO_UTF16_BE == byte_order ? "UTF-16BE"s
       : BO_UTF32_LE == byte_order ? "UTF-32LE"s
       :                             "UTF-32BE"s;
}

// 1 byte: 0xxxxxxx,
// 2 bytes: 110xxxxx 10xxxxxx,
// 3 bytes: 1110xxxx 10xxxxxx 10xxxxxx

std::string
mm_text_io_c::read_next_codepoint() {
  unsigned char buffer[9];

  if (BO_NONE == m_byte_order) {
    std::string::size_type length = read(buffer, 1);
    return std::string{reinterpret_cast<char *>(buffer), length};
  }

  std::string::size_type size;
  if (BO_UTF8 == m_byte_order) {
    if (read(buffer, 1) != 1)
      return {};

    size = ((buffer[0] & 0x80) == 0x00) ?  1
         : ((buffer[0] & 0xe0) == 0xc0) ?  2
         : ((buffer[0] & 0xf0) == 0xe0) ?  3
         : ((buffer[0] & 0xf8) == 0xf0) ?  4
         : ((buffer[0] & 0xfc) == 0xf8) ?  5
         : ((buffer[0] & 0xfe) == 0xfc) ?  6
         :                                99;

    if (99 == size)
      throw mtx::mm_io::text::invalid_utf8_char_x(buffer[0]);

    if ((1 < size) && (read(&buffer[1], size - 1) != (size - 1)))
      return {};

    return std::string{reinterpret_cast<char *>(buffer), size};
  }

  size = ((BO_UTF16_LE == m_byte_order) || (BO_UTF16_BE == m_byte_order)) ? 2 : 4;

  if (read(buffer, size) != size)
    return {};

  unsigned long data = 0;
  auto little_endian = ((BO_UTF16_LE == m_byte_order) || (BO_UTF32_LE == m_byte_order));
  auto shift         = little_endian ? 0 : 8 * (size - 1);
  for (auto i = 0u; i < size; i++) {
    data  |= static_cast<unsigned long>(buffer[i]) << shift;
    shift += little_endian ? 8 : -8;
  }

  if (data < 0x80) {
    buffer[0] = data;
    return std::string{reinterpret_cast<char *>(buffer), 1};
  }

  if (data < 0x800) {
    buffer[0] = 0xc0 | (data >> 6);
    buffer[1] = 0x80 | (data & 0x3f);
    return std::string{reinterpret_cast<char *>(buffer), 2};
  }

  if (data < 0x10000) {
    buffer[0] = 0xe0 |  (data >> 12);
    buffer[1] = 0x80 | ((data >> 6) & 0x3f);
    buffer[2] = 0x80 |  (data       & 0x3f);
    return std::string{reinterpret_cast<char *>(buffer), 3};
  }

  mxerror(Y("mm_text_io_c: UTF32_* is not supported at the moment.\n"));

  return {};
}

std::string
mm_text_io_c::getline(boost::optional<std::size_t> max_chars) {
  if (eof())
    throw mtx::mm_io::end_of_file_x{mtx::mm_io::make_error_code()};

  if (!m_eol_style_detected)
    detect_eol_style();

  std::string s;
  bool previous_was_carriage_return = false;
  std::size_t num_chars_read{};

  while (1) {
    auto previous_pos = getFilePointer();
    auto utf8char     = read_next_codepoint();
    auto len          = utf8char.length();

    if (0 == len)
      return s;

    if ((1 == len) && (utf8char[0] == '\r')) {
      if (previous_was_carriage_return && !m_uses_newlines) {
        setFilePointer(previous_pos);
        return s;
      }

      previous_was_carriage_return = true;
      continue;
    }

    if ((1 == len) && (utf8char[0] == '\n') && (!m_uses_carriage_returns || previous_was_carriage_return))
      return s;

    if (previous_was_carriage_return) {
      setFilePointer(previous_pos);
      return s;
    }

    previous_was_carriage_return  = false;
    s                            += utf8char;
    ++num_chars_read;

    if (max_chars && (num_chars_read >= *max_chars))
      return s;
  }
}

void
mm_text_io_c::setFilePointer(int64 offset,
                             seek_mode mode) {
  mm_proxy_io_c::setFilePointer(((0 == offset) && (seek_beginning == mode)) ? m_bom_len : offset, mode);
}
