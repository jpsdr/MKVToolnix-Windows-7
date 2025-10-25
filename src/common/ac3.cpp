/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   helper function for AC-3 data

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include <cstring>

#include "common/ac3.h"
#include "common/bit_reader.h"
#include "common/bit_writer.h"
#include "common/bswap.h"
#include "common/byte_buffer.h"
#include "common/channels.h"
#include "common/checksums/base.h"
#include "common/codec.h"
#include "common/debugging.h"
#include "common/endian.h"
#include "common/math.h"
#include "common/strings/formatting.h"

namespace mtx::ac3 {

namespace {
uint64_t s_acmod_to_channel_layout[8] = {
  mtx::channels::front_left | mtx::channels::front_right,
                                                           mtx::channels::front_center,
  mtx::channels::front_left | mtx::channels::front_right,
  mtx::channels::front_left | mtx::channels::front_right | mtx::channels::front_center,
  mtx::channels::front_left | mtx::channels::front_right |                               mtx::channels::back_center,
  mtx::channels::front_left | mtx::channels::front_right | mtx::channels::front_center | mtx::channels::back_center,
  mtx::channels::front_left | mtx::channels::front_right |                               mtx::channels::side_left | mtx::channels::side_right,
  mtx::channels::front_left | mtx::channels::front_right | mtx::channels::front_center | mtx::channels::side_left | mtx::channels::side_right,
};

uint64_t s_custom_channel_map_to_layout[16] = {
  mtx::channels::front_left,
  mtx::channels::front_center,
  mtx::channels::front_right,
  mtx::channels::side_left,
  mtx::channels::side_right,
  mtx::channels::front_left_of_center | mtx::channels::front_right_of_center,
  mtx::channels::back_left | mtx::channels::back_right,
  mtx::channels::back_center,
  mtx::channels::top_center,
  mtx::channels::surround_direct_left | mtx::channels::surround_direct_right,
  mtx::channels::wide_left | mtx::channels::wide_right,
  mtx::channels::top_front_left | mtx::channels::top_front_right,
  mtx::channels::top_front_center,
  mtx::channels::top_back_left | mtx::channels::top_back_right,
  mtx::channels::low_frequency_2,
  mtx::channels::low_frequency,
};

}

void
frame_c::init() {
  *this = frame_c{};
}

bool
frame_c::is_eac3()
  const {
  return ((10 < m_bs_id) && (16 >= m_bs_id)) || !m_dependent_frames.empty();
}

codec_c
frame_c::get_codec()
  const {
  auto specialization = is_eac3()        ? codec_c::specialization_e::e_ac_3
                      : m_is_surround_ex ? codec_c::specialization_e::ac3_dolby_surround_ex
                      :                    codec_c::specialization_e::none;

  return codec_c::look_up(codec_c::type_e::A_AC3).specialize(specialization);
}

void
frame_c::add_dependent_frame(frame_c const &frame,
                             uint8_t const *buffer,
                             std::size_t buffer_size) {
  m_data->add(buffer, buffer_size);
  m_dependent_frames.push_back(frame);

  m_channels = get_effective_number_of_channels();
}

bool
frame_c::decode_header(uint8_t const *buffer,
                       std::size_t buffer_size) {
  if (buffer_size < 18)
    return false;

  uint8_t swapped_buffer[18];
  std::unique_ptr<mtx::bits::reader_c> r;

  if (get_uint16_le(buffer) == SYNC_WORD) {
    // byte-swapped
    mtx::bytes::swap_buffer(buffer, swapped_buffer, 18, 2);
    r.reset(new mtx::bits::reader_c(swapped_buffer, 18));

  } else if (get_uint16_be(buffer) == SYNC_WORD)
    r.reset(new mtx::bits::reader_c(buffer, 18));

  else
    return false;

  try {
    init();

    r->set_bit_position(16);
    m_bs_id = r->get_bits(29) & 0x1f;
    r->set_bit_position(16);

    m_valid = 16 == m_bs_id                    ? decode_header_type_eac3(*r) // original E-AC-3
            :  8 >= m_bs_id                    ? decode_header_type_ac3(*r)  // regular AC-3
            : (16 > m_bs_id) && (10 < m_bs_id) ? decode_header_type_eac3(*r) // versions of E-AC-3 backwards-compatible with 0x10
            :                                  false;

  } catch (mtx::mm_io::end_of_file_x &) {
  }

  return m_valid;
}

bool
frame_c::decode_header_type_eac3(mtx::bits::reader_c &bc) {
  static const int sample_rates[] = { 48000, 44100, 32000, 24000, 22050, 16000 };
  static const int samples[]      = {   256,   512,   768,  1536 };

  m_frame_type = bc.get_bits(2);

  if (FRAME_TYPE_RESERVED == m_frame_type)
    return false;

  m_sub_stream_id = bc.get_bits(3);
  m_bytes         = (bc.get_bits(11) + 1) << 1;

  if (!m_bytes)
    return false;

  uint8_t fscod   = bc.get_bits(2);
  uint8_t fscod2  = bc.get_bits(2);

  if ((0x03 == fscod) && (0x03 == fscod2))
    return false;

  uint8_t acmod = bc.get_bits(3);
  uint8_t lfeon = bc.get_bit();
  bc.skip_bits(5);              // bsid

  m_dialog_normalization_gain_bit_position = bc.get_bit_position();
  m_dialog_normalization_gain              = bc.get_bits(5);

  if (bc.get_bit())             // compre
    bc.skip_bits(8);            // compr

  if (acmod == 0x00) {          // dual mono mode
    m_dialog_normalization_gain2_bit_position = bc.get_bit_position();
    m_dialog_normalization_gain2              = bc.get_bits(5);

    if (bc.get_bit())           // compr2e
      bc.skip_bits(8);          // compr2
  }

  if ((m_frame_type == FRAME_TYPE_DEPENDENT) && bc.get_bit()) { // chanmape
    auto chanmap     = bc.get_bits(16);
    m_channel_layout = 0;

    for (auto idx = 0; idx < 16; ++idx) {
      auto mask = 1 << (15 - idx);
      if (chanmap & mask)
        m_channel_layout |= s_custom_channel_map_to_layout[idx];
    }

  } else
    m_channel_layout = s_acmod_to_channel_layout[acmod];

  m_sample_rate = sample_rates[0x03 == fscod ? 3 + fscod2 : fscod];
  m_lfeon       = lfeon;
  m_samples     = (0x03 == fscod) ? 1536 : samples[fscod2];
  m_channels    = get_effective_number_of_channels();

  return true;
}

bool
frame_c::decode_header_type_ac3(mtx::bits::reader_c &bc) {
  static const uint16_t sample_rates[]     = { 48000, 44100, 32000 };
  static const uint16_t bit_rates[]        = { 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, 448, 512, 576, 640 };
  static const uint16_t frame_sizes[38][3] = {
    { 64,   69,   96   },
    { 64,   70,   96   },
    { 80,   87,   120  },
    { 80,   88,   120  },
    { 96,   104,  144  },
    { 96,   105,  144  },
    { 112,  121,  168  },
    { 112,  122,  168  },
    { 128,  139,  192  },
    { 128,  140,  192  },
    { 160,  174,  240  },
    { 160,  175,  240  },
    { 192,  208,  288  },
    { 192,  209,  288  },
    { 224,  243,  336  },
    { 224,  244,  336  },
    { 256,  278,  384  },
    { 256,  279,  384  },
    { 320,  348,  480  },
    { 320,  349,  480  },
    { 384,  417,  576  },
    { 384,  418,  576  },
    { 448,  487,  672  },
    { 448,  488,  672  },
    { 512,  557,  768  },
    { 512,  558,  768  },
    { 640,  696,  960  },
    { 640,  697,  960  },
    { 768,  835,  1152 },
    { 768,  836,  1152 },
    { 896,  975,  1344 },
    { 896,  976,  1344 },
    { 1024, 1114, 1536 },
    { 1024, 1115, 1536 },
    { 1152, 1253, 1728 },
    { 1152, 1254, 1728 },
    { 1280, 1393, 1920 },
    { 1280, 1394, 1920 },
  };

  bc.skip_bits(16);             // crc1
  uint8_t fscod = bc.get_bits(2);
  if (0x03 == fscod)
    return false;

  uint8_t frmsizecod = bc.get_bits(6);
  if (38 <= frmsizecod)
    return false;

  auto bsid = bc.get_bits(5);   // bsid
  bc.skip_bits(3);              // bsmod

  uint8_t acmod = bc.get_bits(3);

  if ((acmod & 0x01) && (acmod != 0x01))
    bc.skip_bits(2);            // cmixlev
  if (acmod & 0x04)
    bc.skip_bits(2);            // surmixlev
  if (0x02 == acmod)
    bc.skip_bits(2);            // dsurmod
  uint8_t lfeon                            = bc.get_bit();

  uint8_t sr_shift                         = std::max(m_bs_id, 8u) - 8;

  m_dialog_normalization_gain_bit_position = bc.get_bit_position();
  m_dialog_normalization_gain              = bc.get_bits(5);
  m_sample_rate                            = sample_rates[fscod] >> sr_shift;
  m_bit_rate                               = (bit_rates[frmsizecod >> 1] * 1000) >> sr_shift;
  m_lfeon                                  = lfeon;
  m_channel_layout                         = s_acmod_to_channel_layout[acmod];
  m_channels                               = get_effective_number_of_channels();
  m_bytes                                  = frame_sizes[frmsizecod][fscod] << 1;

  m_samples                                = 1536;
  m_frame_type                             = FRAME_TYPE_INDEPENDENT;

  if (bc.get_bit())           // compre
    bc.skip_bits(8);          // compr
  if (bc.get_bit())           // langcode
    bc.skip_bits(8);          // langcod
  if (bc.get_bit())           // audprodie
    bc.skip_bits(5 + 2);      // mixlevel, roomtyp

  if (acmod == 0) {
    // Dual-mono mode
    m_dialog_normalization_gain2_bit_position = bc.get_bit_position();
    m_dialog_normalization_gain2              = bc.get_bits(5);

    if (bc.get_bit())           // compre2
      bc.skip_bits(8);          // compr2
    if (bc.get_bit())           // langcode2
      bc.skip_bits(8);          // langcod2
    if (bc.get_bit())           // audprodie2
      bc.skip_bits(5 + 2);      // mixlevel2, roomtyp2
  }

  bc.skip_bits(2);              // copyrightb, origbs

  if (bsid == 6) {
    // Alternate Bit Syntax Stream

    if (bc.get_bit())           // xbsi1e
      bc.skip_bits(  2 + 3 + 3  // dmixmod, ltrtcmixlev, ltrtsurmixlev
                   + 3 + 3);    // lorocmixlev, lorosurmixlev

    if (bc.get_bit()) {         // xbsi2e
      auto dsurexmod   = bc.get_bits(2);
      m_is_surround_ex = dsurexmod == 0b10;
    }
  }

  return m_bytes != 0;
}

int
frame_c::find_in(memory_cptr const &buffer) {
  return find_in(buffer->get_buffer(), buffer->get_size());
}

int
frame_c::find_in(uint8_t const *buffer,
                 std::size_t buffer_size) {
  for (int offset = 0; offset < static_cast<int>(buffer_size); ++offset)
    if (decode_header(&buffer[offset], buffer_size - offset))
      return offset;
  return -1;
}

uint64_t
frame_c::get_effective_channel_layout()
  const {
  auto channel_layout = m_channel_layout;

  for (auto const &dependent_frame : m_dependent_frames)
    channel_layout |= dependent_frame.m_channel_layout;

  return channel_layout;
}

int
frame_c::get_effective_number_of_channels()
  const {
  auto num_channels = mtx::math::count_1_bits(get_effective_channel_layout()) + (m_lfeon ? 1 : 0);
  for (auto const &dependent_frame : m_dependent_frames)
    if (dependent_frame.m_lfeon)
      ++num_channels;

  return num_channels;
}

std::string
frame_c::to_string(bool verbose)
  const {
  if (!verbose)
    return fmt::format("position {0} BS ID {1} size {2} E-AC-3 {3}", m_stream_position, m_bs_id, m_bytes, is_eac3());

  const std::string &frame_type = !is_eac3()                                  ? "---"
                                : m_frame_type == FRAME_TYPE_INDEPENDENT ? "independent"
                                : m_frame_type == FRAME_TYPE_DEPENDENT   ? "dependent"
                                : m_frame_type == FRAME_TYPE_AC3_CONVERT ? "AC-3 convert"
                                : m_frame_type == FRAME_TYPE_RESERVED    ? "reserved"
                                :                                               "unknown";

  auto output = fmt::format("position {0} size {2} garbage {1} BS ID {3} E-AC-3 {14} sample rate {4} bit rate {5} channels {6} (effective layout 0x{15:08x}) flags {7} samples {8} type {9} ({12}) "
                            "sub stream ID {10} has dependent frames {11} total size {13}",
                            m_stream_position,
                            m_garbage_size,
                            m_bytes,
                            m_bs_id,
                            m_sample_rate,
                            m_bit_rate,
                            m_channels,
                            m_flags,
                            m_samples,
                            m_frame_type,
                            m_sub_stream_id,
                            m_dependent_frames.size(),
                            frame_type,
                            m_data ? m_data->get_size() : 0,
                            is_eac3(),
                            get_effective_channel_layout());

  for (auto &frame : m_dependent_frames)
    output += fmt::format(" {{ {0} }}", frame.to_string(verbose));

  return output;
}

// ------------------------------------------------------------

parser_c::parser_c()
  : m_parsed_stream_position(0)
  , m_total_stream_position(0)
  , m_garbage_size(0)
{
}

void
parser_c::add_bytes(memory_cptr const &mem) {
  add_bytes(mem->get_buffer(), mem->get_size());
}

void
parser_c::add_bytes(uint8_t *const buffer,
                    std::size_t size) {
  m_buffer.add(buffer, size);
  m_total_stream_position += size;
  parse(false);
}

void
parser_c::flush() {
  parse(true);
}

std::size_t
parser_c::frame_available()
  const {
  return m_frames.size();
}

frame_c
parser_c::get_frame() {
  frame_c frame = m_frames.front();
  m_frames.pop_front();
  return frame;
}

uint64_t
parser_c::get_total_stream_position()
  const {
  return m_total_stream_position;
}

uint64_t
parser_c::get_parsed_stream_position()
  const {
  return m_parsed_stream_position;
}

void
parser_c::parse(bool end_of_stream) {
  uint8_t swapped_buffer[18];
  auto buffer          = m_buffer.get_buffer();
  auto buffer_size     = m_buffer.get_size();
  std::size_t position = 0;

  while ((position + 18) < buffer_size) {
    auto sync_word = get_uint16_le(&buffer[position]);

    if (sync_word == 0x1001) {
      position += 16;
      continue;
    }

    frame_c frame;
    uint8_t const *buffer_to_decode;

    if (sync_word == SYNC_WORD) {
      mtx::bytes::swap_buffer(&buffer[position], swapped_buffer, 18, 2);
      buffer_to_decode = swapped_buffer;

    } else
      buffer_to_decode = &buffer[position];

    if (!frame.decode_header(buffer_to_decode, 18)) {
      ++position;
      ++m_garbage_size;
      continue;
    }

    if ((position + frame.m_bytes) > buffer_size)
      break;

    if (buffer_to_decode == swapped_buffer)
      mtx::bytes::swap_buffer(&buffer[position], &buffer[position], frame.m_bytes, 2);

    frame.m_stream_position = m_parsed_stream_position + position;

    if (!m_current_frame.m_valid || (FRAME_TYPE_DEPENDENT != frame.m_frame_type)) {
      if (m_current_frame.m_valid)
        m_frames.push_back(m_current_frame);

      m_current_frame        = frame;
      m_current_frame.m_data = memory_c::clone(&buffer[position], frame.m_bytes);

    } else
      m_current_frame.add_dependent_frame(frame, &buffer[position], frame.m_bytes);

    m_current_frame.m_garbage_size += m_garbage_size;
    m_garbage_size                  = 0;
    position                       += frame.m_bytes;
  }

  m_buffer.remove(position);
  m_parsed_stream_position += position;

  if (m_current_frame.m_valid && end_of_stream) {
    m_frames.push_back(m_current_frame);
    m_current_frame.init();
  }
}

int
parser_c::find_consecutive_frames(uint8_t const *buffer,
                                  std::size_t buffer_size,
                                  std::size_t num_required_headers) {
  static auto s_debug = debugging_option_c{"ac3_consecutive_frames"};
  std::size_t base = 0;

  do {
    mxdebug_if(s_debug, fmt::format("Starting search for {1} headers with base {0}, buffer size {2}\n", base, num_required_headers, buffer_size));

    std::size_t position = base;

    if (((position + 16) < buffer_size) && get_uint16_be(&buffer[position]) == 0x0110)
      position += 16;

    frame_c first_frame;
    while (((position + 8) < buffer_size) && !first_frame.decode_header(&buffer[position], buffer_size - position))
      ++position;

    mxdebug_if(s_debug, fmt::format("First frame at {0} valid {1}\n", position, first_frame.m_valid));

    if (!first_frame.m_valid)
      return -1;

    std::size_t offset            = position + first_frame.m_bytes;
    std::size_t num_headers_found = 1;

    while (   (num_headers_found < num_required_headers)
           && (offset            < buffer_size)) {

      if (((offset + 16) < buffer_size) && get_uint16_be(&buffer[offset]) == 0x0110)
        offset += 16;

      frame_c current_frame;
      if (!current_frame.decode_header(&buffer[offset], buffer_size - offset))
        break;

      if (8 > current_frame.m_bytes) {
        mxdebug_if(s_debug, fmt::format("Current frame at {0} has invalid size {1}\n", offset, current_frame.m_bytes));
        break;
      }

      if (   (current_frame.m_bs_id       != first_frame.m_bs_id)
          && (current_frame.m_channels    != first_frame.m_channels)
          && (current_frame.m_sample_rate != first_frame.m_sample_rate)) {
        mxdebug_if(s_debug,
                   fmt::format("Current frame at {6} differs from first frame. (first/current) BS ID: {0}/{1} channels: {2}/{3} sample rate: {4}/{5}\n",
                               first_frame.m_bs_id, current_frame.m_bs_id, first_frame.m_channels, current_frame.m_channels, first_frame.m_sample_rate, current_frame.m_sample_rate, offset));
        break;
      }

      mxdebug_if(s_debug, fmt::format("Current frame at {0} equals first frame, found {1}\n", offset, num_headers_found + 1));

      ++num_headers_found;
      offset += current_frame.m_bytes;
    }

    if (num_headers_found == num_required_headers) {
      mxdebug_if(s_debug, fmt::format("Found required number of headers at {0}\n", position));
      return position;
    }

    base = position + 2;
  } while (base < buffer_size);

  return -1;
}

// ------------------------------------------------------------

/*
   The functions mul_poly, pow_poly and verify_ac3_crc were taken
   or adopted from the ffmpeg project, file "libavcodec/ac3enc.c".

   The license here is the GPL v2.
 */

constexpr auto CRC16_POLY = (1 << 0) | (1 << 2) | (1 << 15) | (1 << 16);

static unsigned int
mul_poly(unsigned int a,
         unsigned int b,
         unsigned int poly) {
  unsigned int c = 0;
  while (a) {
    if (a & 1)
      c ^= b;
    a = a >> 1;
    b = b << 1;
    if (b & (1 << 16))
      b ^= poly;
  }
  return c;
}

static unsigned int
pow_poly(unsigned int a,
         unsigned int n,
         unsigned int poly) {
  unsigned int r = 1;
  while (n) {
    if (n & 1)
      r = mul_poly(r, a, poly);
    a = mul_poly(a, a, poly);
    n >>= 1;
  }
  return r;
}

static uint16_t
calculate_crc1(uint8_t const *buf,
               std::size_t frame_size) {
  int frame_size_words = frame_size >> 1;
  int frame_size_58    = (frame_size_words >> 1) + (frame_size_words >> 3);

  uint16_t crc1        = mtx::bytes::swap_16(mtx::checksum::calculate_as_uint(mtx::checksum::algorithm_e::crc16_ansi, buf + 4, 2 * frame_size_58 - 4));
  unsigned int crc_inv = pow_poly((CRC16_POLY >> 1), (16 * frame_size_58) - 16, CRC16_POLY);

  return mul_poly(crc_inv, crc1, CRC16_POLY);
}

static uint16_t
calculate_crc2(uint8_t const *buf,
               std::size_t frame_size) {
  return mtx::bytes::swap_16(mtx::checksum::calculate_as_uint(mtx::checksum::algorithm_e::crc16_ansi, &buf[2], frame_size - 4));
}

bool
verify_checksums(uint8_t const *buf,
                 std::size_t size,
                 bool full_buffer) {
  while (size) {
    frame_c frame;

    auto res = frame.decode_header(buf, size);

    if (!res)
      return false;

    if (size < frame.m_bytes)
      return false;

    // crc1 is not present in E-AC-3.
    if (!frame.is_eac3()) {
      uint16_t expected_crc1 = get_uint16_be(&buf[2]);
      auto actual_crc1       = calculate_crc1(buf, frame.m_bytes);

      if (expected_crc1 != actual_crc1)
        return false;
    }

    auto expected_crc2 = get_uint16_be(&buf[frame.m_bytes - 2]);
    auto actual_crc2   = calculate_crc2(buf, frame.m_bytes);

    if (expected_crc2 != actual_crc2)
      return false;

    if (!full_buffer)
      break;

    buf  += frame.m_bytes;
    size -= frame.m_bytes;
  }

  return true;
}

static void
remove_dialog_normalization_gain_impl(uint8_t *buf,
                                      frame_c &frame) {
  static debugging_option_c s_debug{"ac3_remove_dialog_normalization_gain|remove_dialog_normalization_gain"};


  unsigned int const removed_level = 31;

  if (   (frame.m_dialog_normalization_gain == removed_level)
      && (   !frame.m_dialog_normalization_gain2
          || (*frame.m_dialog_normalization_gain2 == removed_level))) {
    mxdebug_if(s_debug, fmt::format("no need to remove the dialog normalization gain, it's already set to -{0} dB\n", removed_level));
    return;
  }

  mxdebug_if(s_debug,
             fmt::format("changing dialog normalization gain from -{0} dB ({1}) to -{2} dB\n",
                         frame.m_dialog_normalization_gain, frame.m_dialog_normalization_gain2 ? fmt::format("-{0} dB", *frame.m_dialog_normalization_gain2) : "—"s, removed_level));

  mtx::bits::writer_c w{buf, frame.m_bytes};

  w.set_bit_position(frame.m_dialog_normalization_gain_bit_position);
  w.put_bits(5, removed_level);

  if (frame.m_dialog_normalization_gain2_bit_position) {
    w.set_bit_position(*frame.m_dialog_normalization_gain2_bit_position);
    w.put_bits(5, removed_level);
  }

  // crc1 is not present in E-AC-3.
  if (!frame.is_eac3())
    put_uint16_be(&buf[2], calculate_crc1(buf, frame.m_bytes));

  auto &crcrsv_byte  = buf[frame.m_bytes - 3];
  crcrsv_byte       &= 0xfe;

  auto crc2 = calculate_crc2(buf, frame.m_bytes);
  if (crc2 == SYNC_WORD) {
    crcrsv_byte |= 0x01;
    crc2         = calculate_crc2(buf, frame.m_bytes);
  }

  put_uint16_be(&buf[frame.m_bytes - 2], crc2);
}

void
remove_dialog_normalization_gain(uint8_t *buf,
                                 std::size_t size) {
  while (true) {
    frame_c frame;

    if (!frame.decode_header(buf, size))
      return;

    if (size < frame.m_bytes)
      return;

    remove_dialog_normalization_gain_impl(buf, frame);

    buf  += frame.m_bytes;
    size -= frame.m_bytes;
  }
}

}                              // namespace mtx::ac3
