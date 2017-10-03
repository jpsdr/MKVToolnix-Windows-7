/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   helper function for TrueHD data

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include "common/bit_reader.h"
#include "common/debugging.h"
#include "common/endian.h"
#include "common/list_utils.h"
#include "common/memory.h"
#include "common/truehd.h"

int const truehd_frame_t::ms_sampling_rates[16]   = { 48000, 96000, 192000, 0, 0, 0, 0, 0, 44100, 88200, 176400, 0, 0, 0, 0, 0 };
uint8_t const truehd_frame_t::ms_mlp_channels[32] = {     1,     2,      3, 4, 3, 4, 5, 3,     4,     5,      4, 5, 6, 4, 5, 4,
                                                          5,     6,      5, 5, 6, 0, 0, 0,     0,     0,      0, 0, 0, 0, 0, 0 };

constexpr std::size_t PARSER_MIN_HEADER_SIZE = 12;

bool
truehd_frame_t::parse_header(unsigned char const *data,
                             std::size_t size) {
  if (size < PARSER_MIN_HEADER_SIZE)
    return false;

  auto const first_word = get_uint16_be(&data[0]);
  auto const sync_word  = get_uint32_be(&data[4]);

  if (!mtx::included_in(sync_word, TRUEHD_SYNC_WORD, MLP_SYNC_WORD) && (AC3_SYNC_WORD == first_word))
    return parse_ac3_header(data, size);

  m_codec = !mtx::included_in(sync_word, TRUEHD_SYNC_WORD, MLP_SYNC_WORD) ? truehd : TRUEHD_SYNC_WORD == sync_word ? truehd : mlp;
  m_type  =  mtx::included_in(sync_word, TRUEHD_SYNC_WORD, MLP_SYNC_WORD) ? sync   : normal;
  m_size  = (first_word & 0xfff) * 2;

  if (normal == m_type)
    return true;

  return m_codec == truehd ? parse_truehd_header(data, size) : parse_mlp_header(data, size);
}

bool
truehd_frame_t::parse_ac3_header(unsigned char const *data,
                                 std::size_t size) {
  if (!m_ac3_header.decode_header(data, size))
    return false;

  m_codec = truehd_frame_t::ac3;
  m_type  = truehd_frame_t::sync;
  m_size  = m_ac3_header.m_bytes;

  return size >= m_ac3_header.m_bytes ? mtx::ac3::verify_checksum(data, size) : false;
}

unsigned int
truehd_frame_t::decode_rate_bits(unsigned int rate_bits) {
  if (0xf == rate_bits)
    return 0;

  return (rate_bits & 8 ? 44100 : 48000) << (rate_bits & 7);
}

bool
truehd_frame_t::parse_mlp_header(unsigned char const *data,
                                 std::size_t) {
  m_sampling_rate     = decode_rate_bits(data[9] >> 4);
  m_samples_per_frame = 40 << ((data[9] >> 4) & 0x07);
  m_channels          = ms_mlp_channels[data[11] & 0x1f];

  return true;
}

bool
truehd_frame_t::parse_truehd_header(unsigned char const *data,
                                    std::size_t size) {
  static debugging_option_c s_debug{"truehd_atmos"};

  try {
    mtx::bits::reader_c r{&data[8], size - 8};

    auto rate_bits      = r.get_bits(4);
    m_samples_per_frame = 40 << (rate_bits & 0x07);
    m_sampling_rate     = decode_rate_bits(rate_bits);
    r.skip_bits(4);

    auto chanmap_substream_1 = r.skip_get_bits(4, 5);
    auto chanmap_substream_2 = r.skip_get_bits(2, 13);
    m_channels               = decode_channel_map(chanmap_substream_2 ? chanmap_substream_2 : chanmap_substream_1);

    // The rest is only for Atmos detection.
    auto is_vbr          = r.skip_get_bits(6 * 8, 1);
    auto maximum_bitrate = r.get_bits(15);
    auto num_substreams  = r.get_bits(4);
    auto has_extensions  = r.skip_get_bits(4 + 8 * 8 + 7, 1);
    auto num_extensions  = (r.get_bits(4) * 2) + 1;
    auto has_content     = !!r.get_bits(4);

    mxdebug_if(s_debug,
               boost::format("is_vbr %1% maximum_bitrate %2% num_substreams %3% has_extensions %4% num_extensions %5% has_content %6%\n")
               % is_vbr % maximum_bitrate % num_substreams % has_extensions % num_substreams % has_content);

    if (!has_extensions)
      return true;

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

// Code for truehd_parser_c::decode_channel_map was taken from
// the ffmpeg project, source file "libavcodec/mlp_parser.c".
int
truehd_frame_t::decode_channel_map(int channel_map) {
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

truehd_parser_c::truehd_parser_c()
  : m_sync_state(state_unsynced)
{
}

truehd_parser_c::~truehd_parser_c()
{
}

void
truehd_parser_c::add_data(const unsigned char *new_data,
                          unsigned int new_size) {
  if (!new_data || (0 == new_size))
    return;

  m_buffer.add(new_data, new_size);

  parse(false);
}

void
truehd_parser_c::parse(bool end_of_stream) {
  unsigned char *data = m_buffer.get_buffer();
  unsigned int size   = m_buffer.get_size();
  unsigned int offset = 0;

  if (PARSER_MIN_HEADER_SIZE > size)
    return;

  if (state_unsynced == m_sync_state) {
    offset = resync(offset);
    if (state_unsynced == m_sync_state)
      return;
  }

  while ((size - offset) >= PARSER_MIN_HEADER_SIZE) {
    auto frame  = std::make_shared<truehd_frame_t>();
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

    mxverb(3,
           boost::format("codec %7% type %1% offset %2% size %3% channels %4% sampling_rate %5% samples_per_frame %6%\n")
           % (  frame->is_sync()   ? "S"
              : frame->is_normal() ? "n"
              : frame->is_ac3()    ? "A"
              :                      "x")
           % offset % frame->m_size % frame->m_channels % frame->m_sampling_rate % frame->m_samples_per_frame
           % (  frame->is_truehd() ? "TrueHD"
              :                      "MLP"));

    m_frames.push_back(frame);

    offset += frame->m_size;
  }

  m_buffer.remove(offset);
}

bool
truehd_parser_c::frame_available() {
  return !m_frames.empty();
}

truehd_frame_cptr
truehd_parser_c::get_next_frame() {
  if (m_frames.empty())
    return truehd_frame_cptr{};

  truehd_frame_cptr frame = *m_frames.begin();
  m_frames.pop_front();

  return frame;
}

unsigned int
truehd_parser_c::resync(unsigned int offset) {
  const unsigned char *data = m_buffer.get_buffer();
  unsigned int size         = m_buffer.get_size();

  m_sync_state              = state_unsynced;
  auto frame                = truehd_frame_t{};

  for (offset = offset + 4; (offset + 4) < size; ++offset) {
    uint32_t sync_word = get_uint32_be(&data[offset]);
    if (   (   mtx::included_in(sync_word, TRUEHD_SYNC_WORD, MLP_SYNC_WORD)
            || (AC3_SYNC_WORD == get_uint16_be(&data[offset - 4])))
        && frame.parse_header(&data[offset - 4], size - 4)) {
      m_sync_state  = state_synced;
      return offset - 4;
    }
  }

  return 0;
}
