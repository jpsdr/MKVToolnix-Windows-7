/*
   base64.cpp
   Base64 encoding and decoding.

   Part of this file (the two encoding functions) were adopted from
   http://base64.sourceforge.net/ and are licensed under the MIT license
   (following):

   LICENCE:     Copyright (c) 2001 Bob Trower, Trantor Standard Systems Inc.

                Permission is hereby granted, free of charge, to any person
                obtaining a copy of this software and associated
                documentation files (the "Software"), to deal in the
                Software without restriction, including without limitation
                the rights to use, copy, modify, merge, publish, distribute,
                sublicense, and/or sell copies of the Software, and to
                permit persons to whom the Software is furnished to do so,
                subject to the following conditions:

                The above copyright notice and this permission notice shall
                be included in all copies or substantial portions of the
                Software.

                THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
                KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
                WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
                PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
                OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
                OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
                OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
                SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

   The decoding function was adapted from
   http://sourceforge.net/projects/base64decoder/ and is licensed under
   the GPL v2 or later. See the file COPYING for details.

   The rest is licensed the same way the rest of MKVToolNix is
   licensed: under the GPL v2.

   base64 encoding and decoding functions

   See the above URLs for the original authors.
   Modified by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include "common/base64.h"
#include "common/error.h"
#include "common/strings/editing.h"

namespace mtx::base64 {

static const char base64_encoding[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static void
encode_block(const uint8_t in[3],
             int len,
             std::string &out) {
  out += base64_encoding[in[0] >> 2];
  out += base64_encoding[((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4)];
  out += (uint8_t)(len > 1 ? base64_encoding[((in[1] & 0x0f) << 2) | ((in[2] & 0xc0) >> 6)] : '=');
  out += (uint8_t)(len > 2 ? base64_encoding[in[2] & 0x3f]                                  : '=');
}

std::string
encode(const uint8_t *src,
       int src_len,
       bool line_breaks,
       int max_line_len) {
  int pos        = 0;
  int blocks_out = 0;

  if (4 > max_line_len)
    max_line_len = 4;

  int i, len_mod = max_line_len / 4;
  std::string out;

  while (pos < src_len) {
    uint8_t in[3];
    int len = 0;
    for (i = 0; 3 > i; ++i) {
      if (pos < src_len) {
        ++len;
        in[i] = (uint8_t)src[pos];
      } else
        in[i] = 0;
      ++pos;
    }

    encode_block(in, len, out);

    ++blocks_out;
    if (line_breaks && ((blocks_out % len_mod) == 0))
      out += "\n";
  }

  return out;
}

memory_cptr
decode(std::string const &src) {
  auto pos      = 0u;
  auto pad      = 0u;
  auto src_size = src.size();
  auto dst      = memory_c::alloc(src_size);
  auto dst_ptr  = dst->get_buffer();
  auto dst_idx  = 0;

  while (pos < src_size) {
    uint8_t in[4];
    unsigned int in_pos = 0;

    while ((src_size > pos) && (4 > in_pos)) {
      auto c = static_cast<uint8_t>(src[pos]);
      ++pos;

      if ((('A' <= c) && ('Z' >= c)) || (('a' <= c) && ('z' >= 'c')) || (('0' <= c) && ('9' >= c)) || ('+' == c) || ('/' == c)) {
        in[in_pos] = c;
        ++in_pos;

      } else if (c == '=')
        pad++;

      else if (!mtx::string::is_blank_or_tab(c) && !mtx::string::is_newline(c))
        throw mtx::base64::invalid_data_x{};
    }

    unsigned int values_idx;
    uint8_t values[4];
    memset(values, 0, 4);
    for (values_idx = 0; values_idx < in_pos; values_idx++) {
      values[values_idx] =
          (('A' <= in[values_idx]) && ('Z' >= in[values_idx])) ? in[values_idx] - 'A'
        : (('a' <= in[values_idx]) && ('z' >= in[values_idx])) ? in[values_idx] - 'a' + 26
        : (('0' <= in[values_idx]) && ('9' >= in[values_idx])) ? in[values_idx] - '0' + 52
        :  ('+' == in[values_idx])                             ?  62
        :  ('/' == in[values_idx])                             ?  63
        :                                                        255;

      if (255 == values[values_idx])
        throw mtx::base64::invalid_data_x{};
    }

    uint8_t mid[6];
    mid[0] = values[0] << 2;
    mid[1] = values[1] >> 4;
    mid[2] = values[1] << 4;
    mid[3] = values[2] >> 2;
    mid[4] = values[2] << 6;
    mid[5] = values[3];

    dst_ptr[dst_idx] = mid[0] | mid[1];
    if (1 >= pad) {
      dst_ptr[dst_idx + 1] = mid[2] | mid[3];
      if (0 == pad)
        dst_ptr[dst_idx + 2] = mid[4] | mid[5];
    }

    dst_idx += 0 == pad ? 3 : 1 == pad ? 2 : 1;

    if (0 != pad)
      break;
  }

  dst->resize(dst_idx);

  return dst;
}

}
