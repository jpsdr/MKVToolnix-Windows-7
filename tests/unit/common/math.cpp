#include "common/common_pch.h"

#include <limits>

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

}
