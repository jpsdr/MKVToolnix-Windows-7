#include "common/common_pch.h"

#include "common/bit_reader.h"
#include "common/bit_writer.h"
#include "common/xyzvc/util.h"

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

std::optional<stereo_mode_c::mode>
parse(bool cancel_flag,
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

  w.put_bits(32, 0);                                 // remainder of the payload

  auto buffer = w.get_buffer();
  auto r      = mtx::bits::reader_c{buffer->get_buffer(), buffer->get_size()};

  return mtx::xyzvc::parse_frame_packing_arrangement(r);
}

TEST(XyzvcFramePackingArrangement, LeftViewFirst) {
  EXPECT_EQ(stereo_mode_c::checkerboard_left_first,       parse(false, 0, 1));
  EXPECT_EQ(stereo_mode_c::column_interleaved_left_first, parse(false, 1, 1));
  EXPECT_EQ(stereo_mode_c::row_interleaved_left_first,    parse(false, 2, 1));
  EXPECT_EQ(stereo_mode_c::side_by_side_left_first,       parse(false, 3, 1));
  EXPECT_EQ(stereo_mode_c::top_bottom_left_first,         parse(false, 4, 1));
}

TEST(XyzvcFramePackingArrangement, RightViewFirst) {
  EXPECT_EQ(stereo_mode_c::checkerboard_right_first,       parse(false, 0, 2));
  EXPECT_EQ(stereo_mode_c::column_interleaved_right_first, parse(false, 1, 2));
  EXPECT_EQ(stereo_mode_c::row_interleaved_right_first,    parse(false, 2, 2));
  EXPECT_EQ(stereo_mode_c::side_by_side_right_first,       parse(false, 3, 2));
  EXPECT_EQ(stereo_mode_c::top_bottom_right_first,         parse(false, 4, 2));
}

TEST(XyzvcFramePackingArrangement, UnspecifiedInterpretationIsLeftViewFirst) {
  EXPECT_EQ(stereo_mode_c::side_by_side_left_first, parse(false, 3, 0));
}

TEST(XyzvcFramePackingArrangement, NonZeroArrangementId) {
  EXPECT_EQ(stereo_mode_c::top_bottom_left_first, parse(false, 4, 1, 3));
  EXPECT_EQ(stereo_mode_c::side_by_side_right_first, parse(false, 3, 2, 254));
}

TEST(XyzvcFramePackingArrangement, NoStereoModeEquivalent) {
  EXPECT_FALSE(parse(false, 5, 1).has_value());      // temporal interleaving
  EXPECT_FALSE(parse(false, 6, 1).has_value());
  EXPECT_FALSE(parse(false, 7, 1).has_value());
}

TEST(XyzvcFramePackingArrangement, Cancel) {
  EXPECT_FALSE(parse(true, 3, 1).has_value());
}

}
