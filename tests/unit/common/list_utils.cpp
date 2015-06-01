#include "common/common_pch.h"

#include "common/list_utils.h"

#include "gtest/gtest.h"

namespace {

TEST(ListUtils, included_in) {
  EXPECT_TRUE(mtx::included_in(42, 42));
  EXPECT_TRUE(mtx::included_in(42, 54, 42, 48));
  EXPECT_TRUE(mtx::included_in(42, 42, 54, 48));
  EXPECT_TRUE(mtx::included_in(42, 54, 48, 42));

  EXPECT_FALSE(mtx::included_in(23, 42));
  EXPECT_FALSE(mtx::included_in(23, 54, 42, 48));
  EXPECT_FALSE(mtx::included_in(23, 42, 54, 48));
  EXPECT_FALSE(mtx::included_in(23, 54, 48, 42));
}

TEST(ListUtils, any_of) {
  EXPECT_TRUE(mtx::any_of<int>([](int val) { return val < 43; }, 42));
  EXPECT_TRUE(mtx::any_of<int>([](int val) { return val < 43; }, 54, 42, 48));
  EXPECT_TRUE(mtx::any_of<int>([](int val) { return val < 43; }, 42, 54, 48));
  EXPECT_TRUE(mtx::any_of<int>([](int val) { return val < 43; }, 54, 48, 42));

  EXPECT_FALSE(mtx::any_of<int>([](int val) { return val > 64; }, 42));
  EXPECT_FALSE(mtx::any_of<int>([](int val) { return val > 64; }, 54, 42, 48));
  EXPECT_FALSE(mtx::any_of<int>([](int val) { return val > 64; }, 42, 54, 48));
  EXPECT_FALSE(mtx::any_of<int>([](int val) { return val > 64; }, 54, 48, 42));
}

TEST(ListUtils, all_of) {
  EXPECT_TRUE(mtx::all_of<int>([](int val) { return val < 64; }, 42));
  EXPECT_TRUE(mtx::all_of<int>([](int val) { return val < 64; }, 54, 42, 48));
  EXPECT_TRUE(mtx::all_of<int>([](int val) { return val < 64; }, 42, 54, 48));
  EXPECT_TRUE(mtx::all_of<int>([](int val) { return val < 64; }, 54, 48, 42));

  EXPECT_FALSE(mtx::all_of<int>([](int val) { return val < 42; }, 42));
  EXPECT_FALSE(mtx::all_of<int>([](int val) { return val < 42; }, 54, 42, 48));
  EXPECT_FALSE(mtx::all_of<int>([](int val) { return val < 42; }, 42, 54, 48));
  EXPECT_FALSE(mtx::all_of<int>([](int val) { return val < 42; }, 54, 48, 42));
}

TEST(ListUtils, none_of) {
  EXPECT_TRUE(mtx::none_of<int>([](int val) { return val > 64; }, 42));
  EXPECT_TRUE(mtx::none_of<int>([](int val) { return val > 64; }, 54, 42, 48));
  EXPECT_TRUE(mtx::none_of<int>([](int val) { return val > 64; }, 42, 54, 48));
  EXPECT_TRUE(mtx::none_of<int>([](int val) { return val > 64; }, 54, 48, 42));

  EXPECT_FALSE(mtx::none_of<int>([](int val) { return val < 56; }, 42));
  EXPECT_FALSE(mtx::none_of<int>([](int val) { return val < 56; }, 54, 42, 48));
  EXPECT_FALSE(mtx::none_of<int>([](int val) { return val < 56; }, 42, 54, 48));
  EXPECT_FALSE(mtx::none_of<int>([](int val) { return val < 56; }, 54, 48, 42));
}

}
