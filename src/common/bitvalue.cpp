/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   The "value_c" class

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/bitvalue.h"
#include "common/error.h"
#include "common/memory.h"
#include "common/random.h"
#include "common/strings/editing.h"

namespace mtx::bits {

namespace {

uint8_t
hex_digit_to_decimal(char c) {
  return isdigit(c) ? (c - '0') : (c - 'a' + 10);
}

}

value_c::value_c(int bitsize) {
  assert((0 < bitsize) && (0 == (bitsize % 8)));

  m_value = memory_c::alloc(bitsize / 8);
  memset(m_value->get_buffer(), 0, m_value->get_size());
}

value_c::value_c(const value_c &src) {
  *this = src;
}

value_c::value_c(std::string s,
                 unsigned int allowed_bitlength) {
  if ((allowed_bitlength != 0) && ((allowed_bitlength % 8) != 0))
    throw mtx::invalid_parameter_x();

  unsigned int len = s.size();
  balg::to_lower(s);
  std::string s2;

  unsigned int i;
  for (i = 0; i < len; i++) {
    // Space or tab?
    if (mtx::string::is_blank_or_tab(s[i]))
      continue;

    // Skip hyphens and curly braces. Makes copy & paste a bit easier.
    if ((s[i] == '-') || (s[i] == '{') || (s[i] == '}'))
      continue;

    // Space or tab followed by "0x"? Then skip it.
    if (s.substr(i, 2) == "0x") {
      i++;
      continue;
    }

    // Invalid character?
    auto is_hex_digit = isdigit(s[i]) || ((s[i] >= 'a') && (s[i] <= 'f'));
    if (!is_hex_digit)
      throw mtx::bits::value_parser_x{fmt::format(FY("Not a hex digit at position {0}"), i)};

    // Input too long?
    if ((allowed_bitlength > 0) && ((s2.length() * 4) >= allowed_bitlength))
      throw mtx::bits::value_parser_x{fmt::format(FY("Input too long: {0} > {1}"), s2.length() * 4, allowed_bitlength)};

    // Store the value.
    s2 += s[i];
  }

  // Is half a byte or more missing?
  len = s2.length();
  if (((len % 2) != 0)
      ||
      ((allowed_bitlength != 0) && ((len * 4) < allowed_bitlength)))
    throw mtx::bits::value_parser_x{Y("Missing one hex digit")};

  m_value = memory_c::alloc(len / 2);

  auto buffer = m_value->get_buffer();
  for (i = 0; i < len; i += 2)
    buffer[i / 2] = (hex_digit_to_decimal(s2[i]) << 4) | hex_digit_to_decimal(s2[i + 1]);
}

value_c::value_c(libebml::EbmlBinary const &elt)
  : m_value(memory_c::clone(static_cast<const void *>(elt.GetBuffer()), elt.GetSize()))
{
}

value_c &
value_c::operator =(const value_c &src) {
  m_value = src.m_value->clone();

  return *this;
}

bool
value_c::operator ==(const value_c &cmp)
  const {
  return (cmp.m_value->get_size() == m_value->get_size()) && (0 == memcmp(m_value->get_buffer(), cmp.m_value->get_buffer(), m_value->get_size()));
}

uint8_t
value_c::operator [](size_t index)
  const {
  assert(m_value->get_size() > index);
  return m_value->get_buffer()[index];
}

void
value_c::generate_random() {
  random_c::generate_bytes(m_value->get_buffer(), m_value->get_size());
}

void
value_c::zero_content() {
  std::memset(m_value->get_buffer(), 0, m_value->get_size());
}

}
