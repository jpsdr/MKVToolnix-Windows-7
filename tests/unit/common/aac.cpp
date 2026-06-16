#include "common/common_pch.h"

#include "common/aac.h"
#include "common/bit_writer.h"

#include "tests/unit/init.h"

namespace {

// Build a single, valid 7-byte-header ADTS frame (AAC-LC, 44.1 kHz, stereo,
// no CRC) followed by `payload_size` zero bytes. The payload starts with
// 0x00 so that its first three bits are not mistaken for a program config
// element (ID_PCE == 0x05).
void
append_adts_frame(std::vector<uint8_t> &out,
                  unsigned int payload_size) {
  auto const frame_length = 7u + payload_size;

  mtx::bits::writer_c w;
  w.put_bits(12, 0xfff);          // syncword
  w.put_bits( 1, 0);              // ID: MPEG-4
  w.put_bits( 2, 0);              // layer == 0
  w.put_bits( 1, 1);              // protection_absent (no CRC -> 7 byte header)
  w.put_bits( 2, 1);              // profile (AAC-LC)
  w.put_bits( 4, 4);              // sampling_frequency_index (44100)
  w.put_bits( 1, 0);              // private_bit
  w.put_bits( 3, 2);              // channel_configuration (stereo)
  w.put_bits( 1, 0);              // original/copy
  w.put_bits( 1, 0);              // home
  w.put_bits( 1, 0);              // copyright_id_bit
  w.put_bits( 1, 0);              // copyright_id_start
  w.put_bits(13, frame_length);   // aac_frame_length (incl. header)
  w.put_bits(11, 0x7ff);          // adts_buffer_fullness (VBR)
  w.put_bits( 2, 0);              // number_of_raw_data_blocks_in_frame - 1

  for (auto idx = 0u; idx < payload_size; ++idx)
    w.put_bits(8, 0);

  auto mem = w.get_buffer();
  out.insert(out.end(), mem->get_buffer(), mem->get_buffer() + mem->get_size());
}

std::vector<uint8_t>
build_adts_stream(unsigned int num_frames,
                  unsigned int num_leading_garbage_bytes) {
  std::vector<uint8_t> out;

  // Leading bytes that are neither an ADTS sync (0xfff) nor a LOAS sync
  // (0x2b7) at offset 0, emulating an mkvmerge buffer that begins partway
  // through the stream (e.g. r_aac feeding from `tag_size_start`).
  for (auto idx = 0u; idx < num_leading_garbage_bytes; ++idx)
    out.push_back(0x00);

  for (auto idx = 0u; idx < num_frames; ++idx)
    append_adts_frame(out, 16);

  return out;
}

void
append_garbage(std::vector<uint8_t> &out,
               unsigned int num_bytes) {
  for (auto idx = 0u; idx < num_bytes; ++idx)
    out.push_back(0x00);
}

// Two real, consecutive LOAS/LATM frames (AAC-LC, generated with ffmpeg's
// `latm` muxer). The first frame carries the StreamMuxConfig; the second
// reuses it.
uint8_t const s_two_loas_latm_frames[] = {
  0x56, 0xe0, 0xa0, 0x20, 0x00, 0x12, 0x08, 0x1f, 0xe4, 0xce, 0xf0, 0x10,
  0x02, 0x63, 0x0b, 0xb3, 0x19, 0xb1, 0x91, 0x71, 0x91, 0xc1, 0x71, 0x89,
  0x81, 0x88, 0x00, 0x13, 0x05, 0x2a, 0xa4, 0x86, 0xd3, 0x91, 0x97, 0x9e,
  0x3d, 0x3c, 0xf1, 0xaa, 0xf3, 0x97, 0x2d, 0x57, 0x59, 0x22, 0x3c, 0xa4,
  0x91, 0x7e, 0x61, 0x20, 0xdb, 0xb8, 0x1f, 0x82, 0xf5, 0x5e, 0xc5, 0xdf,
  0x70, 0xd9, 0x6d, 0x4b, 0x48, 0xff, 0x1e, 0x97, 0x9b, 0xde, 0xf5, 0x7e,
  0x4f, 0x03, 0x6a, 0x62, 0xab, 0x1b, 0x66, 0xb4, 0xd7, 0x9f, 0x59, 0x29,
  0x34, 0xd5, 0x53, 0x15, 0xcc, 0x55, 0x19, 0x34, 0xa5, 0x92, 0x96, 0x4b,
  0x4d, 0x29, 0x34, 0x64, 0xa1, 0x4a, 0x32, 0x51, 0x92, 0x89, 0x35, 0xa1,
  0xb4, 0x36, 0x86, 0x06, 0x36, 0x6c, 0x18, 0x19, 0x12, 0x20, 0x63, 0x66,
  0xcd, 0x9b, 0x44, 0x89, 0x14, 0xa6, 0xcd, 0x9b, 0x8a, 0x28, 0xa2, 0x8a,
  0x28, 0xa2, 0x8a, 0x28, 0xa2, 0x8a, 0x28, 0xa2, 0x44, 0x51, 0x45, 0x14,
  0x51, 0x44, 0x88, 0x88, 0x88, 0x90, 0x88, 0xa2, 0x88, 0x8a, 0x28, 0x91,
  0x14, 0x51, 0x14, 0x51, 0x45, 0x17, 0x00, 0x56, 0xe0, 0xa4, 0xd1, 0x00,
  0x9c, 0x4a, 0x6d, 0x64, 0xae, 0xca, 0x29, 0xd5, 0x94, 0xea, 0xc9, 0x74,
  0xe7, 0xf9, 0xf6, 0xe3, 0x4d, 0x9e, 0x35, 0x7f, 0xfd, 0x6b, 0xf7, 0xeb,
  0x8d, 0x71, 0xab, 0xd7, 0xff, 0xda, 0xf1, 0xfc, 0xf9, 0xe3, 0x5c, 0x6a,
  0xf5, 0xff, 0xf1, 0x7b, 0xff, 0x3e, 0x78, 0xd7, 0x5a, 0xd5, 0x86, 0xff,
  0x5b, 0x1a, 0x3e, 0x8a, 0x0c, 0x05, 0xd0, 0x26, 0x58, 0x42, 0xcf, 0xd4,
  0xeb, 0x37, 0x94, 0xc6, 0xdc, 0xe0, 0x33, 0x9c, 0x06, 0x06, 0x77, 0x09,
  0x98, 0x18, 0x19, 0xdc, 0x24, 0x24, 0x18, 0x19, 0xdc, 0x24, 0x24, 0x18,
  0x18, 0x19, 0xdd, 0xc2, 0x42, 0x56, 0x0c, 0x0c, 0x0c, 0xee, 0xe1, 0x21,
  0x27, 0x21, 0x36, 0x12, 0xef, 0xf0, 0x85, 0x58, 0x6e, 0xa7, 0xce, 0x5c,
  0xaf, 0x4a, 0x56, 0xf4, 0xa1, 0x37, 0xb3, 0x37, 0x28, 0x49, 0x52, 0x59,
  0x83, 0x4a, 0x12, 0x12, 0x4a, 0xf0, 0x34, 0xa1, 0x21, 0x24, 0xaf, 0x38,
  0x31, 0xb3, 0x61, 0x21, 0x25, 0x41, 0xb3, 0x70, 0x60, 0x63, 0x61, 0x21,
  0x25, 0x58, 0xe5, 0xe2, 0x1a, 0x8a, 0x28, 0xa0, 0xd4, 0x51, 0x45, 0x14,
  0x6e, 0x36, 0xba, 0x28, 0xe0, 0x00
};

} // anonymous namespace

// Positive control: a clean ADTS stream that begins exactly on a frame
// boundary must be detected as ADTS.
TEST(AacMultiplexDetection, CleanAdtsStreamIsDetectedAsAdts) {
  auto buffer = build_adts_stream(6, 0);

  mtx::aac::parser_c parser;
  parser.add_bytes(buffer.data(), buffer.size());

  EXPECT_EQ(parser.get_multiplex_type(), mtx::aac::parser_c::adts_multiplex);
  EXPECT_EQ(parser.frames_available(), 6u);
}

// Regression test for issue #6196: a genuine ADTS stream whose buffer does
// not start on a frame boundary must still be detected as ADTS (not flipped
// to LOAS/LATM or the dead `adif` candidate) and must decode its frames.
TEST(AacMultiplexDetection, AdtsStreamNotStartingOnFrameBoundaryIsDetectedAsAdts) {
  auto buffer = build_adts_stream(6, 5);

  mtx::aac::parser_c parser;
  parser.add_bytes(buffer.data(), buffer.size());

  EXPECT_EQ(parser.get_multiplex_type(), mtx::aac::parser_c::adts_multiplex);
  EXPECT_GT(parser.frames_available(), 0u);
}

// The original #6196 source file also contained garbage stretches between
// frames (mkvmerge 86.0 logged ~692 such skips while still decoding the audio).
// A stream with leading garbage and a garbage stretch in the middle must be
// detected as ADTS, skip the garbage, and decode every real frame.
TEST(AacMultiplexDetection, AdtsStreamWithMidStreamGarbageIsDetectedAndRecovers) {
  std::vector<uint8_t> buffer;
  append_garbage(buffer, 8);
  for (auto idx = 0u; idx < 5u; ++idx)
    append_adts_frame(buffer, 16);
  append_garbage(buffer, 50);
  for (auto idx = 0u; idx < 5u; ++idx)
    append_adts_frame(buffer, 16);

  mtx::aac::parser_c parser;
  parser.add_bytes(buffer.data(), buffer.size());

  EXPECT_EQ(parser.get_multiplex_type(), mtx::aac::parser_c::adts_multiplex);
  EXPECT_EQ(parser.frames_available(), 10u);
}

// The fallback must weigh the longest run of *consecutive* frames, not the
// total number of frames decodable anywhere in the buffer. Here three isolated
// ADTS frames (total count 3, longest run 1) are followed by two consecutive
// LOAS/LATM frames (total count 2, longest run 2). A total-count heuristic
// would pick ADTS; scoring by the longest consecutive run correctly picks
// LOAS/LATM, since only the LATM frames chain.
TEST(AacMultiplexDetection, LongestConsecutiveRunWinsOverTotalFrameCount) {
  std::vector<uint8_t> buffer;
  append_garbage(buffer, 8);
  for (auto idx = 0u; idx < 3u; ++idx) {
    append_adts_frame(buffer, 16);
    append_garbage(buffer, 8);
  }
  buffer.insert(buffer.end(), s_two_loas_latm_frames, s_two_loas_latm_frames + sizeof(s_two_loas_latm_frames));

  mtx::aac::parser_c parser;
  parser.add_bytes(buffer.data(), buffer.size());

  EXPECT_EQ(parser.get_multiplex_type(), mtx::aac::parser_c::loas_latm_multiplex);
}
