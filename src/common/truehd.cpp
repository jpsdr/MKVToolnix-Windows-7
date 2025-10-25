/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   helper function for TrueHD data

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/bit_reader.h"
#include "common/bit_writer.h"
#include "common/bswap.h"
#include "common/checksums/base_fwd.h"
#include "common/debugging.h"
#include "common/endian.h"
#include "common/list_utils.h"
#include "common/memory.h"
#include "common/truehd.h"

// TrueHD header fields
// ====================

// pos | value      | meaning
// ----+------------+------------------------------------------------------------------------------
//   0 | …          | size & other stuff
//  32 | f8726fba   | TrueHD sync word
//  64 | 0          | sample rate idx
//  68 | 0          | ?
//  72 | 5          | 2:ch_modifier_thd0 2:ch_modifier_thd1
//  76 | 7a00f      | 5:channel_arrangement 2:ch_modifier_thd2 13:channel_arrangement
//  96 | b752       | major_sync_info_signature
// 112 | 0000       | flags
// 128 | 0000       | ?
// 144 | 83b6       | 1:is_vbr 15:coded_peak_bitrate
// 160 | 2          | num_substreams
// 164 | 0          | ?
// 168 | 3c         | substream_info
// 176 | 0000       | 5:fs 5:wordlength 6:channel_occupancy
// 192 | 3d302306e3 | 2:? 5:dialnorm_2.0 6:? 5:dialnorm_5.1 11:? 5:dialnorm_7.1 6:?
// 232 | 00         | 3:? 5:summary_info
// 240 | f6b3       | header_checksum

namespace mtx::truehd {

int const frame_t::ms_sampling_rates[16]   = { 48000, 96000, 192000, 0, 0, 0, 0, 0, 44100, 88200, 176400, 0, 0, 0, 0, 0 };
uint8_t const frame_t::ms_mlp_channels[32] = {     1,     2,      3, 4, 3, 4, 5, 3,     4,     5,      4, 5, 6, 4, 5, 4,
                                                   5,     6,      5, 5, 6, 0, 0, 0,     0,     0,      0, 0, 0, 0, 0, 0 };

constexpr std::size_t PARSER_MIN_HEADER_SIZE = 12;

bool
frame_t::parse_header(uint8_t const *data,
                      std::size_t size) {
  if (size < PARSER_MIN_HEADER_SIZE)
    return false;

  auto const first_word = get_uint16_be(&data[0]);
  auto const sync_word  = get_uint32_be(&data[4]);

  if (!mtx::included_in(sync_word, TRUEHD_SYNC_WORD, MLP_SYNC_WORD) && (mtx::ac3::SYNC_WORD == first_word))
    return parse_ac3_header(data, size);

  m_codec = !mtx::included_in(sync_word, TRUEHD_SYNC_WORD, MLP_SYNC_WORD) ? truehd : TRUEHD_SYNC_WORD == sync_word ? truehd : mlp;
  m_type  =  mtx::included_in(sync_word, TRUEHD_SYNC_WORD, MLP_SYNC_WORD) ? sync   : normal;
  m_size  = (first_word & 0xfff) * 2;

  if (normal == m_type)
    return true;

  return m_codec == truehd ? parse_truehd_header(data, size) : parse_mlp_header(data, size);
}

bool
frame_t::parse_ac3_header(uint8_t const *data,
                          std::size_t size) {
  if (!m_ac3_header.decode_header(data, size))
    return false;

  m_codec = frame_t::ac3;
  m_type  = frame_t::sync;
  m_size  = m_ac3_header.m_bytes;

  return size >= m_ac3_header.m_bytes ? mtx::ac3::verify_checksums(data, size) : false;
}

unsigned int
frame_t::decode_rate_bits(unsigned int rate_bits) {
  if (0xf == rate_bits)
    return 0;

  return (rate_bits & 8 ? 44100 : 48000) << (rate_bits & 7);
}

bool
frame_t::parse_mlp_header(uint8_t const *data,
                          std::size_t) {
  m_sampling_rate     = decode_rate_bits(data[9] >> 4);
  m_samples_per_frame = 40 << ((data[9] >> 4) & 0x07);
  m_channels          = ms_mlp_channels[data[11] & 0x1f];

  return true;
}

bool
frame_t::parse_truehd_header(uint8_t const *data,
                             std::size_t size) {
  static debugging_option_c s_debug{"truehd_atmos"};

  try {
    mtx::bits::reader_c r{data, size};

    r.skip_bits(4 + 12);        // check_nibble, access_unit_length

    m_input_timing = r.get_bits(16);

    r.skip_bits(32);            // format_sync

    auto rate_bits      = r.get_bits(4);
    m_samples_per_frame = 40 << (rate_bits & 0x07);
    m_sampling_rate     = decode_rate_bits(rate_bits);
    r.skip_bits(4);

    auto chanmap_substream_1 = r.skip_get_bits(4, 5);
    auto chanmap_substream_2 = r.skip_get_bits(2, 13);
    m_channels               = decode_channel_map(chanmap_substream_2 ? chanmap_substream_2 : chanmap_substream_1);
    m_header_size            = 32;

    // The rest is only for Atmos detection.
    auto is_vbr          = r.skip_get_bits(6 * 8, 1);
    auto maximum_bitrate = r.get_bits(15);
    auto num_substreams  = r.get_bits(4);
    auto has_extensions  = r.skip_get_bits(4 + 8 * 8 + 7, 1);
    auto num_extensions  = r.get_bits(4);
    auto has_content     = !!r.get_bits(4);

    mxdebug_if(s_debug,
               fmt::format("is_vbr {0} maximum_bitrate {1} num_substreams {2} has_extensions {3} num_extensions {4} has_content {5}\n",
                           is_vbr, maximum_bitrate, num_substreams, has_extensions, num_extensions, has_content));

    if (!has_extensions)
      return true;

    m_header_size += 2 + num_extensions * 2;

    for (auto idx = 0u; idx < num_extensions; ++idx)
      if (r.get_bits(8))
        has_content = true;

    if (has_content)
      m_contains_atmos = true;

    return true;

  } catch (...) {
  }

  return false;
}

// Code for parser_c::decode_channel_map was taken from
// the ffmpeg project, source file "libavcodec/mlp_parser.c".
int
frame_t::decode_channel_map(int channel_map) {
  static const int s_channel_count[13] = {
    //  LR    C   LFE  LRs LRvh  LRc LRrs  Cs   Ts  LRsd  LRw  Cvh  LFE2
         2,   1,   1,   2,   2,   2,   2,   1,   1,   2,   2,   1,   1
  };

  int channels = 0, i;

  for (i = 0; 13 > i; ++i)
    channels += s_channel_count[i] * ((channel_map >> i) & 1);

  return channels;
}

// ----------------------------------------------------------------------

void
parser_c::add_data(uint8_t const *new_data,
                   unsigned int new_size) {
  if (!new_data || (0 == new_size))
    return;

  m_buffer.add(new_data, new_size);

  parse(false);
}

void
parser_c::parse(bool end_of_stream) {
  static debugging_option_c s_debug{"truehd_parser"};

  auto data           = m_buffer.get_buffer();
  auto size           = m_buffer.get_size();
  unsigned int offset = 0;

  if (PARSER_MIN_HEADER_SIZE > size)
    return;

  if (state_unsynced == m_sync_state) {
    offset = resync(offset);
    if (state_unsynced == m_sync_state)
      return;
  }

  while ((size - offset) >= PARSER_MIN_HEADER_SIZE) {
    auto frame  = std::make_shared<frame_t>();
    auto result = frame->parse_header(&data[offset], size - offset);

    if (   !result
        && (   !frame->is_ac3()
            || (frame->is_ac3() && !end_of_stream)))
      break;

    if (8 > frame->m_size) {
      unsigned int synced_at = resync(offset + 1);
      if (state_unsynced == m_sync_state)
        break;

      offset = synced_at;
    }

    if ((frame->m_size + offset) > size)
      break;

    frame->m_data = memory_c::clone(&data[offset], frame->m_size);

    if (frame->is_truehd() || frame->is_mlp()) {
      if (frame->is_sync())
        m_sync_codec = frame->m_codec;

      else
        frame->m_codec = m_sync_codec;
    }

    mxdebug_if(s_debug,
               fmt::format("codec {6} type {0} offset {1} size {2} channels {3} sampling_rate {4} samples_per_frame {5}\n",
                             frame->is_sync()   ? "S"
                           : frame->is_normal() ? "n"
                           : frame->is_ac3()    ? "A"
                           :                      "x",
                           offset, frame->m_size, frame->m_channels, frame->m_sampling_rate, frame->m_samples_per_frame,
                             frame->is_truehd() ? "TrueHD"
                           : frame->is_mlp()    ? "MLP"
                           :                      "AC-3"));

    m_frames.push_back(frame);

    offset += frame->m_size;
  }

  m_buffer.remove(offset);
}

bool
parser_c::frame_available() {
  return !m_frames.empty();
}

frame_cptr
parser_c::get_next_frame() {
  if (m_frames.empty())
    return frame_cptr{};

  auto frame = *m_frames.begin();
  m_frames.pop_front();

  return frame;
}

unsigned int
parser_c::resync(unsigned int offset) {
  auto data    = m_buffer.get_buffer();
  auto size    = m_buffer.get_size();

  m_sync_state = state_unsynced;
  auto frame   = frame_t{};

  for (offset = offset + 4; (offset + 4) < size; ++offset) {
    uint32_t sync_word = get_uint32_be(&data[offset]);
    if (   (   mtx::included_in(sync_word, TRUEHD_SYNC_WORD, MLP_SYNC_WORD)
               || (mtx::ac3::SYNC_WORD == get_uint16_be(&data[offset - 4])))
        && frame.parse_header(&data[offset - 4], size - 4)) {
      m_sync_state  = state_synced;
      return offset - 4;
    }
  }

  return 0;
}

// ----------------------------------------------------------------------

void
remove_dialog_normalization_gain(uint8_t *buf,
                                 std::size_t size) {
  if (size < 32)
    return;

  static debugging_option_c s_debug{"truehd_remove_dialog_normalization_gain|remove_dialog_normalization_gain"};

  frame_t frame;
  if (!frame.parse_header(buf, size) || !frame.is_truehd() || !frame.is_sync())
    return;

  unsigned int const removed_level = 31;

  mtx::bits::reader_c r{buf, size};
  r.set_bit_position(194);
  auto current_level_2_0 = r.get_bits(5);
  auto current_level_5_1 = r.skip_get_bits(6, 5);
  auto current_level_7_1 = r.skip_get_bits(11, 5);

  if ((current_level_2_0 == removed_level) && (current_level_5_1 == removed_level) && (current_level_7_1 == removed_level)) {
    mxdebug_if(s_debug, fmt::format("no need to remove the dialog normalization, it's already set to {0} for 2.0, 5.1 & 7.1\n", removed_level));
    return;
  }

  mtx::bits::writer_c w{buf, size};
  w.set_bit_position(194);
  w.put_bits(5, removed_level); // 2.0
  w.skip_bits(6);
  w.put_bits(5, removed_level); // 5.1
  w.skip_bits(11);
  w.put_bits(5, removed_level); // 7.1

  mxdebug_if(s_debug, fmt::format("changing dialog normalization from {0} (2.0), {1} (5.1) & {2} (7.1) to {3}\n", current_level_2_0, current_level_5_1, current_level_7_1, removed_level));

  auto new_checksum = mtx::bytes::swap_16(mtx::checksum::calculate_as_uint(mtx::checksum::algorithm_e::crc16_002d, buf + 4, frame.m_header_size - 4 - 2 - 2));
  new_checksum     ^= get_uint16_be(&buf[frame.m_header_size - 2 - 2]);

  // auto current = get_uint16_be(&buf[frame.m_header_size - 2]);
  // mxinfo(fmt::format("header size {3} new checksum 0x{0:04x} current checksum 0x{1:04x} {2}\n", new_checksum, current, new_checksum == current ? "✓" : "✗", frame.m_header_size));

  put_uint16_be(&buf[frame.m_header_size - 2], new_checksum);
}


}
