#include "common/common_pch.h"

#include "common/bit_writer.h"
#include "common/hevc/util.h"

#include "tests/unit/init.h"

namespace {

void
put_unsigned_golomb(mtx::bits::writer_c &w,
                    uint64_t value) {
  auto code_num      = value + 1;
  auto num_bits      = 0u;

  for (auto tmp = code_num; tmp > 1; tmp >>= 1)
    ++num_bits;

  w.put_bits(num_bits, 0);
  w.put_bits(num_bits + 1, code_num);
}

memory_cptr
make_frame_packing_payload(bool cancel_flag,
                           unsigned int arrangement_type,
                           unsigned int content_interpretation_type,
                           unsigned int arrangement_id = 0) {
  mtx::bits::writer_c w;

  put_unsigned_golomb(w, arrangement_id);            // frame_packing_arrangement_id
  w.put_bit(cancel_flag);                            // frame_packing_arrangement_cancel_flag

  if (!cancel_flag) {
    w.put_bits(7, arrangement_type);                 // frame_packing_arrangement_type
    w.put_bit(false);                                // quincunx_sampling_flag
    w.put_bits(6, content_interpretation_type);      // content_interpretation_type
  }

  w.put_bit(true);                                   // stop bit, just so the payload ends aligned
  w.byte_align();

  return w.get_buffer();
}

memory_cptr
make_dummy_payload() {
  mtx::bits::writer_c w;

  w.put_bits(16, 0);

  return w.get_buffer();
}

// Builds the RBSP of a prefix SEI NAL unit carrying the given payloads,
// terminated by rbsp_trailing_bits. parse_sei() expects its input to have
// the emulation prevention bytes removed already.
memory_cptr
make_sei_nalu(std::vector<std::pair<unsigned int, memory_cptr>> const &payloads) {
  std::vector<uint8_t> bytes{0x4e, 0x01};            // nal_unit_type 39 (prefix SEI), nuh_temporal_id_plus1 1

  for (auto const &[type, payload] : payloads) {
    auto t = type;
    while (t >= 255) {
      bytes.push_back(0xff);
      t -= 255;
    }
    bytes.push_back(t);

    auto s = payload->get_size();
    while (s >= 255) {
      bytes.push_back(0xff);
      s -= 255;
    }
    bytes.push_back(s);

    for (std::size_t i = 0; i < payload->get_size(); ++i)
      bytes.push_back(payload->get_buffer()[i]);
  }

  bytes.push_back(0x80);                             // rbsp_trailing_bits

  return memory_c::clone(bytes.data(), bytes.size());
}

std::optional<stereo_mode_c::mode>
parse(memory_cptr const &nalu) {
  mtx::hevc::user_data_t user_data;
  std::optional<stereo_mode_c::mode> stereo_mode;

  mtx::hevc::parse_sei(nalu, user_data, &stereo_mode);

  return stereo_mode;
}

TEST(HevcSeiFramePacking, DetectsStereoModeAfterOtherPayloads) {
  auto stereo_mode = parse(make_sei_nalu({{0, make_dummy_payload()}, {45, make_frame_packing_payload(false, 3, 1)}}));

  ASSERT_TRUE(stereo_mode.has_value());
  EXPECT_EQ(stereo_mode_c::side_by_side_left_first, *stereo_mode);
}

TEST(HevcSeiFramePacking, RightViewFirst) {
  auto stereo_mode = parse(make_sei_nalu({{45, make_frame_packing_payload(false, 4, 2)}}));

  ASSERT_TRUE(stereo_mode.has_value());
  EXPECT_EQ(stereo_mode_c::top_bottom_right_first, *stereo_mode);
}

TEST(HevcSeiFramePacking, CancelYieldsNoStereoMode) {
  auto nalu        = make_sei_nalu({{45, make_frame_packing_payload(true, 0, 0)}});
  mtx::hevc::user_data_t user_data;
  std::optional<stereo_mode_c::mode> stereo_mode;

  EXPECT_TRUE(mtx::hevc::parse_sei(nalu, user_data, &stereo_mode));
  EXPECT_FALSE(stereo_mode.has_value());
}

TEST(HevcSeiFramePacking, ArrangementTypeWithoutEquivalentYieldsNoStereoMode) {
  EXPECT_FALSE(parse(make_sei_nalu({{45, make_frame_packing_payload(false, 5, 1)}})).has_value());
}

TEST(HevcSeiFramePacking, NoFramePackingPayloadYieldsNoStereoMode) {
  EXPECT_FALSE(parse(make_sei_nalu({{0, make_dummy_payload()}})).has_value());
}

TEST(HevcSeiFramePacking, FirstMessageWins) {
  auto stereo_mode = parse(make_sei_nalu({{45, make_frame_packing_payload(false, 4, 1)}, {45, make_frame_packing_payload(false, 3, 1)}}));

  ASSERT_TRUE(stereo_mode.has_value());
  EXPECT_EQ(stereo_mode_c::top_bottom_left_first, *stereo_mode);
}

TEST(HevcSeiFramePacking, SuffixSeiNaluIsRejected) {
  auto nalu = make_sei_nalu({{45, make_frame_packing_payload(false, 3, 1)}});

  nalu->get_buffer()[0] = 40 << 1;                   // nal_unit_type 40 (suffix SEI)

  mtx::hevc::user_data_t user_data;
  std::optional<stereo_mode_c::mode> stereo_mode;

  EXPECT_FALSE(mtx::hevc::parse_sei(nalu, user_data, &stereo_mode));
  EXPECT_FALSE(stereo_mode.has_value());
}

}
