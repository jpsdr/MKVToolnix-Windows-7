 /*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   IO callback class implementation

   Written by Moritz Bunkus <moritz@bunkus.org>.
   Modifications by Peter Niemayer <niemayer@isg.de>.
*/

#include "common/common_pch.h"

#include <QRegularExpression>

#include "common/endian.h"
#include "common/error.h"
#include "common/fs_sys_helpers.h"
#include "common/mm_io.h"
#include "common/mm_io_p.h"
#include "common/mm_io_x.h"
#include "common/qt.h"
#include "common/strings/editing.h"
#include "common/strings/parsing.h"


namespace {
bool s_disable_writing_byte_order_markers = false;
};

union double_to_uint64_t {
  uint64_t i;
  double d;
};

/*
   Abstract base class.
*/

mm_io_c::mm_io_c()
  : p_ptr{new mm_io_private_c}
{
}

mm_io_c::mm_io_c(mm_io_private_c &p)
  : p_ptr{&p}
{
}

mm_io_c::~mm_io_c() {           // NOLINT(modernize-use-equals-default) due to pimpl idiom requiring explicit dtor declaration somewhere
}

std::string
mm_io_c::getline(std::optional<std::size_t> max_chars) {
  char c;
  std::string s;

  if (eof())
    throw mtx::mm_io::end_of_file_x{mtx::mm_io::make_error_code()};

  while (read(&c, 1) == 1) {
    if (c == '\r')
      continue;
    if (c == '\n')
      return s;
    s += c;

    if (max_chars && (s.length() >= *max_chars))
      break;
  }

  return s;
}

bool
mm_io_c::getline2(std::string &s,
                  std::optional<std::size_t> max_chars) {
  try {
    s = getline(max_chars);
  } catch(...) {
    return false;
  }

  return true;
}

bool
mm_io_c::setFilePointer2(int64_t offset,
                         libebml::seek_mode mode) {
  try {
    setFilePointer(offset, mode);
    return true;
  } catch(...) {
    return false;
  }
}

size_t
mm_io_c::puts(const std::string &s) {
#if defined(SYS_WINDOWS)
  static std::optional<bool> s_use_unix_newlines;
  if (!s_use_unix_newlines.has_value())
    s_use_unix_newlines = mtx::sys::get_environment_variable("MTX_ALWAYS_USE_UNIX_NEWLINES") == "1";

  bool use_unix_newlines = *s_use_unix_newlines;
#else
  bool use_unix_newlines = true;
#endif
  size_t num_written = 0;
  const char *cs     = s.c_str();
  int prev_pos       = 0;
  int cur_pos;

  for (cur_pos = 0; cs[cur_pos] != 0; ++cur_pos) {
    bool keep_char = !use_unix_newlines || ('\r' != cs[cur_pos]) || ('\n' != cs[cur_pos + 1]);
    bool insert_cr = !use_unix_newlines && ('\n' == cs[cur_pos]) && ((0 == cur_pos) || ('\r' != cs[cur_pos - 1]));

    if (keep_char && !insert_cr)
      continue;

    if (prev_pos < cur_pos)
      num_written += write(std::string{&cs[prev_pos], static_cast<std::string::size_type>(cur_pos - prev_pos)});

    if (insert_cr)
      num_written += write("\r"s);

    prev_pos = cur_pos + (keep_char ? 0 : 1);
  }

  if (prev_pos < cur_pos)
    num_written += write(std::string{&cs[prev_pos], static_cast<std::string::size_type>(cur_pos - prev_pos)});

  return num_written;
}

memory_cptr
mm_io_c::read(size_t size) {
  auto buffer = memory_c::alloc(size);
  if (read(buffer, size) != size)
    throw mtx::mm_io::end_of_file_x{};

  return buffer;
}

MTX_EBML_IOCALLBACK_READ_RETURN_TYPE
mm_io_c::read(void *buffer,
              size_t size) {
  return _read(buffer, size);
}

uint32_t
mm_io_c::read(std::string &buffer,
              size_t size,
              size_t offset) {
  buffer.resize(offset + size);

  int num_read = read(&buffer[offset], size);
  if (0 > num_read)
    num_read = 0;

  buffer.resize(offset + num_read);

  return num_read;
}

uint8_t
mm_io_c::read_uint8() {
  uint8_t value;

  if (read(&value, 1) != 1)
    throw mtx::mm_io::end_of_file_x{mtx::mm_io::make_error_code()};

  return value;
}

uint16_t
mm_io_c::read_uint16_le() {
  uint8_t buffer[2];

  if (read(buffer, 2) != 2)
    throw mtx::mm_io::end_of_file_x{mtx::mm_io::make_error_code()};

  return get_uint16_le(buffer);
}

uint32_t
mm_io_c::read_uint24_le() {
  uint8_t buffer[3];

  if (read(buffer, 3) != 3)
    throw mtx::mm_io::end_of_file_x{mtx::mm_io::make_error_code()};

  return get_uint24_le(buffer);
}

uint32_t
mm_io_c::read_uint32_le() {
  uint8_t buffer[4];

  if (read(buffer, 4) != 4)
    throw mtx::mm_io::end_of_file_x{mtx::mm_io::make_error_code()};

  return get_uint32_le(buffer);
}

uint64_t
mm_io_c::read_uint64_le() {
  uint8_t buffer[8];

  if (read(buffer, 8) != 8)
    throw mtx::mm_io::end_of_file_x{mtx::mm_io::make_error_code()};

  return get_uint64_le(buffer);
}

uint16_t
mm_io_c::read_uint16_be() {
  uint8_t buffer[2];

  if (read(buffer, 2) != 2)
    throw mtx::mm_io::end_of_file_x{mtx::mm_io::make_error_code()};

  return get_uint16_be(buffer);
}

int32_t
mm_io_c::read_int24_be() {
  return (static_cast<int32_t>(read_uint24_be()) + 0xff800000) ^ 0xff800000;
}

uint32_t
mm_io_c::read_uint24_be() {
  uint8_t buffer[3];

  if (read(buffer, 3) != 3)
    throw mtx::mm_io::end_of_file_x{mtx::mm_io::make_error_code()};

  return get_uint24_be(buffer);
}

uint32_t
mm_io_c::read_uint32_be() {
  uint8_t buffer[4];

  if (read(buffer, 4) != 4)
    throw mtx::mm_io::end_of_file_x{mtx::mm_io::make_error_code()};

  return get_uint32_be(buffer);
}

uint64_t
mm_io_c::read_uint64_be() {
  uint8_t buffer[8];

  if (read(buffer, 8) != 8)
    throw mtx::mm_io::end_of_file_x{mtx::mm_io::make_error_code()};

  return get_uint64_be(buffer);
}

double
mm_io_c::read_double() {
  double_to_uint64_t d2ui;

  d2ui.i = read_uint64_be();
  return d2ui.d;
}

unsigned int
mm_io_c::read_mp4_descriptor_len() {
  unsigned int len       = 0;
  unsigned int num_bytes = 0;
  uint8_t byte;

  do {
    byte = read_uint8();
    len  = (len << 7) | (byte & 0x7f);
    ++num_bytes;

  } while (((byte & 0x80) == 0x80) && (4 > num_bytes));

  return len;
}

uint32_t
mm_io_c::read(memory_cptr const &buffer,
              size_t size,
              int offset) {
  if (-1 == offset)
    offset = buffer->get_size();

  if (buffer->get_size() <= (size + static_cast<size_t>(offset)))
    buffer->resize(size + offset);

  if (read(buffer->get_buffer() + offset, size) != size)
    throw mtx::mm_io::end_of_file_x{mtx::mm_io::make_error_code()};

  buffer->set_size(size + offset);

  return size;
}

int
mm_io_c::write_uint8(uint8_t value) {
  return write(&value, 1);
}

int
mm_io_c::write_uint16_le(uint16_t value) {
  uint16_t buffer;

  put_uint16_le(&buffer, value);
  return write(&buffer, sizeof(uint16_t));
}

int
mm_io_c::write_uint32_le(uint32_t value) {
  uint32_t buffer;

  put_uint32_le(&buffer, value);
  return write(&buffer, sizeof(uint32_t));
}

int
mm_io_c::write_uint64_le(uint64_t value) {
  uint64_t buffer;

  put_uint64_le(&buffer, value);
  return write(&buffer, sizeof(uint64_t));
}

int
mm_io_c::write_uint16_be(uint16_t value) {
  uint16_t buffer;

  put_uint16_be(&buffer, value);
  return write(&buffer, sizeof(uint16_t));
}

int
mm_io_c::write_uint32_be(uint32_t value) {
  uint32_t buffer;

  put_uint32_be(&buffer, value);
  return write(&buffer, sizeof(uint32_t));
}

int
mm_io_c::write_uint64_be(uint64_t value) {
  uint64_t buffer;

  put_uint64_be(&buffer, value);
  return write(&buffer, sizeof(uint64_t));
}

int
mm_io_c::write_double(double value) {
  double_to_uint64_t d2ui;
  d2ui.d = value;
  return write_uint64_be(d2ui.i);
}

size_t
mm_io_c::write(std::string const &buffer) {
  auto p = p_func();

  if (!p->string_output_converter)
    return write(buffer.c_str(), buffer.length());
  auto native = p->string_output_converter->native(buffer);
  return write(native.c_str(), native.length());
}

size_t
mm_io_c::write(const void *buffer,
               size_t size) {
  return _write(buffer, size);
}

size_t
mm_io_c::write(const memory_cptr &buffer,
               size_t size,
               size_t offset) {
  size = std::min(buffer->get_size() - offset, size);

  if (write(buffer->get_buffer() + offset, size) != size)
    throw mtx::mm_io::end_of_file_x{mtx::mm_io::make_error_code()};
  return size;
}

void
mm_io_c::skip(int64_t num_bytes) {
  uint64_t pos = getFilePointer();
  setFilePointer(pos + num_bytes);
  if ((pos + num_bytes) != getFilePointer())
    throw mtx::mm_io::end_of_file_x{mtx::mm_io::make_error_code()};
}

void
mm_io_c::save_pos(int64_t new_pos) {
  p_func()->positions.push(getFilePointer());

  if (-1 != new_pos)
    setFilePointer(new_pos);
}

bool
mm_io_c::restore_pos() {
  auto p = p_func();

  if (p->positions.empty())
    return false;

  setFilePointer(p->positions.top());
  p->positions.pop();

  return true;
}

void
mm_io_c::disable_writing_byte_order_markers() {
  s_disable_writing_byte_order_markers = true;
}

bool
mm_io_c::write_bom(const std::string &charset_) {
  if (s_disable_writing_byte_order_markers)
    return false;

  auto p = p_func();

  static const uint8_t utf8_bom[3]    = {0xef, 0xbb, 0xbf};
  static const uint8_t utf16le_bom[2] = {0xff, 0xfe};
  static const uint8_t utf16be_bom[2] = {0xfe, 0xff};
  static const uint8_t utf32le_bom[4] = {0xff, 0xfe, 0x00, 0x00};
  static const uint8_t utf32be_bom[4] = {0x00, 0x00, 0xff, 0xfe};
  const uint8_t *bom;
  unsigned int bom_len;

  if (p->bom_written || charset_.empty())
    return false;

  if (p->string_output_converter && !charset_converter_c::is_utf8_charset_name(p->string_output_converter->get_charset()))
    return false;

  auto charset = to_utf8(Q(charset_).toLower().replace(QRegularExpression{"[^a-z0-9]+"}, {}));
  if (charset == "utf8") {
    bom_len = 3;
    bom     = utf8_bom;
  } else if ((charset == "utf16") || (charset =="utf16LE")) {
    bom_len = 2;
    bom     = utf16le_bom;
  } else if (charset == "utF16be") {
    bom_len = 2;
    bom     = utf16be_bom;
  } else if ((charset == "utf32") || (charset == "utf32le")) {
    bom_len = 4;
    bom     = utf32le_bom;
  } else if (charset == "utf32be") {
    bom_len = 4;
    bom     = utf32be_bom;
  } else
    return false;

  setFilePointer(0);

  p->bom_written = write(bom, bom_len) == bom_len;

  return p->bom_written;
}

bool
mm_io_c::bom_written()
  const {
  return p_func()->bom_written;
}

int64_t
mm_io_c::get_size() {
  auto p = p_func();

  if (-1 == p->cached_size) {
    save_pos();
    setFilePointer(0, libebml::seek_end);
    p->cached_size = getFilePointer();
    restore_pos();
  }

  return p->cached_size;
}

int
mm_io_c::getch() {
  uint8_t c;

  if (read(&c, 1) != 1)
    return -1;

  return c;
}

void
mm_io_c::set_string_output_converter(charset_converter_cptr const &converter) {
  p_func()->string_output_converter = converter;
}

void
mm_io_c::use_dos_style_newlines(bool yes) {
  p_func()->dos_style_newlines = yes;
}
