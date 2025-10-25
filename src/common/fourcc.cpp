/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   FourCC handling

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/bswap.h"
#include "common/codec.h"
#include "common/endian.h"
#include "common/fourcc.h"

namespace {

uint32_t
val(uint32_t value,
    fourcc_c::byte_order_e byte_order) {
  return fourcc_c::byte_order_e::big_endian == byte_order ? value : mtx::bytes::swap_32(value);
}

char
printable_char(uint8_t c) {
  return (32 <= c) && (127 > c) ? static_cast<char>(c) : '?';
}

} // anonymous namespace

fourcc_c::fourcc_c()
  : m_value{}
{
}

fourcc_c::fourcc_c(uint32_t value,
                   fourcc_c::byte_order_e byte_order)
  : m_value{val(value, byte_order)}
{
}

fourcc_c::fourcc_c(std::string const &value)
  : m_value{read(value.c_str(), byte_order_e::big_endian)}
{
}

fourcc_c::fourcc_c(char const *value)
  : m_value{read(value, byte_order_e::big_endian)}
{
}

fourcc_c::fourcc_c(memory_cptr const &mem,
                   fourcc_c::byte_order_e byte_order)
  : m_value{read(mem->get_buffer(), byte_order)}
{
}

fourcc_c::fourcc_c(uint8_t const *mem,
                   fourcc_c::byte_order_e byte_order)
  : m_value{read(mem, byte_order)}
{
}

fourcc_c::fourcc_c(uint32_t const *mem,
                   fourcc_c::byte_order_e byte_order)
  : m_value{read(mem, byte_order)}
{
}

fourcc_c::fourcc_c(mm_io_cptr const &io,
                   fourcc_c::byte_order_e byte_order)
  : m_value{read(*io, byte_order)}
{
}

fourcc_c::fourcc_c(mm_io_c &io,
                   fourcc_c::byte_order_e byte_order)
  : m_value{read(io, byte_order)}
{
}

fourcc_c::fourcc_c(mm_io_c *io,
                   fourcc_c::byte_order_e byte_order)
  : m_value{read(*io, byte_order)}
{
}

size_t
fourcc_c::write(memory_cptr const &mem,
                fourcc_c::byte_order_e byte_order) {
  return write(mem->get_buffer(), byte_order);
}

size_t
fourcc_c::write(uint8_t *mem,
                fourcc_c::byte_order_e byte_order) {
  put_uint32_be(mem, val(m_value, byte_order));
  return 4;
}

size_t
fourcc_c::write(mm_io_cptr const &io,
                fourcc_c::byte_order_e byte_order) {
  return write(io.get(), byte_order);
}

size_t
fourcc_c::write(mm_io_c &io,
                fourcc_c::byte_order_e byte_order) {
  return write(&io, byte_order);
}

size_t
fourcc_c::write(mm_io_c *io,
                fourcc_c::byte_order_e byte_order) {
  return io->write_uint32_be(val(m_value, byte_order));
}

uint32_t
fourcc_c::value(fourcc_c::byte_order_e byte_order)
  const {
  return val(m_value, byte_order);
}

std::string
fourcc_c::str()
  const {
  char buffer[4];
  put_uint32_be(buffer, m_value);
  for (auto &c : buffer)
    c = 32 <= c ? c : '?';

  return std::string{buffer, 4};
}

std::string
fourcc_c::description()
  const {
  uint8_t buffer[4];
  put_uint32_be(buffer, m_value);

  auto result = fmt::format("0x{0:08x} \"{1}{2}{3}{4}\"", m_value, printable_char(buffer[0]), printable_char(buffer[1]), printable_char(buffer[2]), printable_char(buffer[3]));
  auto codec  = codec_c::look_up(*this);
  if (codec.valid())
    result += fmt::format(": {0}", codec.get_name());

  return result;
}

fourcc_c::operator bool()
  const {
  return !!m_value;
}

fourcc_c &
fourcc_c::reset() {
  m_value = 0;
  return *this;
}

bool
fourcc_c::operator ==(fourcc_c const &cmp)
  const {
  return m_value == cmp.m_value;
}

bool
fourcc_c::operator !=(fourcc_c const &cmp)
  const {
  return m_value != cmp.m_value;
}

bool
fourcc_c::equiv(char const *cmp)
  const {
  return balg::to_lower_copy(str()) == balg::to_lower_copy(std::string{cmp});
}

bool
fourcc_c::equiv(std::vector<std::string> const &cmp)
  const {
  auto lower = balg::to_lower_copy(str());
  for (auto &s : cmp)
    if (lower == balg::to_lower_copy(s))
      return true;
  return false;
}

bool
fourcc_c::human_readable(size_t min_num)
  const {
  static auto byte_ok = [](int c) { return (('a' <= c) && ('z' >= c)) || (('A' <= c) && ('Z' >= c)) || (('0' <= c) && ('9' >= c)) || (0xa9 == c); };
  auto num            = 0u;
  for (auto shift = 0; 3 >= shift; ++shift)
    num += byte_ok((m_value >> (shift * 8)) & 0xff) ? 1 : 0;
  return min_num <= num;
}

uint32_t
fourcc_c::read(void const *mem,
               fourcc_c::byte_order_e byte_order) {
  return val(get_uint32_be(mem), byte_order);
}

uint32_t
fourcc_c::read(mm_io_c &io,
               fourcc_c::byte_order_e byte_order) {
  return val(io.read_uint32_be(), byte_order);
}

fourcc_c &
fourcc_c::shift_read(mm_io_cptr const &io,
                     byte_order_e byte_order) {
  return shift_read(*io, byte_order);
}

fourcc_c &
fourcc_c::shift_read(mm_io_c &io,
                     byte_order_e byte_order) {
  m_value = byte_order_e::big_endian == byte_order ? (m_value << 8) | io.read_uint8() : (m_value >> 8) | (io.read_uint8() << 24);
  return *this;
}

fourcc_c &
fourcc_c::shift_read(mm_io_c *io,
                     byte_order_e byte_order) {
  return shift_read(*io, byte_order);
}
