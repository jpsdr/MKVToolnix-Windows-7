#include "common/common_pch.h"

#include <limits>

#include "common/endian.h"
#include "common/math.h"

#include "gtest/gtest.h"

namespace {

TEST(Math, Popcount) {
  EXPECT_EQ(3, mtx::math::count_1_bits(131));
  EXPECT_EQ(3, mtx::math::count_1_bits(131u));
  EXPECT_EQ(3, mtx::math::count_1_bits(131ll));
  EXPECT_EQ(3, mtx::math::count_1_bits(131llu));

  EXPECT_EQ(16, mtx::math::count_1_bits(std::numeric_limits<uint16_t>::max()));
  EXPECT_EQ(32, mtx::math::count_1_bits(std::numeric_limits<uint32_t>::max()));
  EXPECT_EQ(64, mtx::math::count_1_bits(std::numeric_limits<uint64_t>::max()));

  EXPECT_EQ(63, mtx::math::count_1_bits(std::numeric_limits<uint64_t>::max() - 1));
}

TEST(Math, IntLog2) {
  EXPECT_EQ(-1, mtx::math::int_log2(0));
  EXPECT_EQ(0, mtx::math::int_log2(1));
  EXPECT_EQ(1, mtx::math::int_log2(2));
  EXPECT_EQ(2, mtx::math::int_log2(4));
  EXPECT_EQ(31, mtx::math::int_log2(0x80000000ul));
  EXPECT_EQ(32, mtx::math::int_log2(0x100000000ull));
  EXPECT_EQ(63, mtx::math::int_log2(0x8000000000000000ull));
  EXPECT_EQ(63, mtx::math::int_log2(0x8000001230000000ull));
}

TEST(Math, ToSigned) {
  unsigned char big_endian_signed_numbers[] = {
    0x83,                                           // 0
    0x80, 0x03,                                     // 1
    0x80, 0x00, 0x00, 0x03,                         // 3
    0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, // 7

    0xfd,                                           // 15
    0xff, 0xfd,                                     // 16
    0xff, 0xff, 0xff, 0xfd,                         // 18
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfd, // 22
  };

  EXPECT_EQ(std::numeric_limits<int8_t>::min()  + 3, mtx::math::to_signed(big_endian_signed_numbers[0]));
  EXPECT_EQ(std::numeric_limits<int16_t>::min() + 3, mtx::math::to_signed(get_uint16_be(&big_endian_signed_numbers[1])));
  EXPECT_EQ(std::numeric_limits<int32_t>::min() + 3, mtx::math::to_signed(get_uint32_be(&big_endian_signed_numbers[3])));
  EXPECT_EQ(std::numeric_limits<int64_t>::min() + 3, mtx::math::to_signed(get_uint64_be(&big_endian_signed_numbers[7])));

  EXPECT_EQ(-3, mtx::math::to_signed(big_endian_signed_numbers[15]));
  EXPECT_EQ(-3, mtx::math::to_signed(get_uint16_be(&big_endian_signed_numbers[16])));
  EXPECT_EQ(-3, mtx::math::to_signed(get_uint32_be(&big_endian_signed_numbers[18])));
  EXPECT_EQ(-3, mtx::math::to_signed(get_uint64_be(&big_endian_signed_numbers[22])));

  EXPECT_EQ(std::numeric_limits<int8_t>::min()  + 3, mtx::math::to_signed(std::numeric_limits<int8_t>::min()  + 3));
  EXPECT_EQ(std::numeric_limits<int16_t>::min() + 3, mtx::math::to_signed(std::numeric_limits<int16_t>::min() + 3));
  EXPECT_EQ(std::numeric_limits<int32_t>::min() + 3, mtx::math::to_signed(std::numeric_limits<int32_t>::min() + 3));
  EXPECT_EQ(std::numeric_limits<int64_t>::min() + 3, mtx::math::to_signed(std::numeric_limits<int64_t>::min() + 3));

  EXPECT_EQ(-3, mtx::math::to_signed(static_cast<int8_t>(-3)));
  EXPECT_EQ(-3, mtx::math::to_signed(static_cast<int16_t>(-3)));
  EXPECT_EQ(-3, mtx::math::to_signed(static_cast<int32_t>(-3)));
  EXPECT_EQ(-3, mtx::math::to_signed(static_cast<int64_t>(-3)));
}

}
