#include "common/common_pch.h"

#include <functional>

#include "common/list_utils.h"

#include "gtest/gtest.h"

namespace {

TEST(ListUtils, first_of) {
  EXPECT_FALSE(mtx::first_of<int>([](int val) { return val > 43; }, 42));

  EXPECT_TRUE(!!mtx::first_of<int>([](int val) { return val > 43; }, 48));
  EXPECT_TRUE(!!mtx::first_of<int>([](int val) { return val > 43; }, 42, 54));
  EXPECT_TRUE(!!mtx::first_of<int>([](int val) { return val > 43; }, 42, 41, 40, 39, 38, 12345));

  EXPECT_EQ(48,    mtx::first_of<int>([](int val) { return val > 43; }, 48));
  EXPECT_EQ(54,    mtx::first_of<int>([](int val) { return val > 43; }, 42, 54).value());
  EXPECT_EQ(12345, mtx::first_of<int>([](int val) { return val > 43; }, 42, 41, 40, 39, 38, 12345).value());

  EXPECT_TRUE(!!mtx::first_of<void *>([](void *p) { return !!p; }, static_cast<void *>(nullptr), reinterpret_cast<void *>(0x12345)));
  EXPECT_EQ(reinterpret_cast<void *>(0x12345), mtx::first_of<void *>([](void *p) { return !!p; }, static_cast<void *>(nullptr), reinterpret_cast<void *>(0x12345)));
}

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

TEST(ListUtils, any) {
  std::vector<int> vals{2, 4, 6, 8, 10};

  EXPECT_TRUE(mtx::any(vals, [](int i) { return i == 4; }));
  EXPECT_FALSE(mtx::any(vals, [](int i) { return (i % 2) == 1; }));

  EXPECT_FALSE(mtx::any(std::vector<int>{}, [](int i) { return i != 0; }));
}

TEST(ListUtils, none) {
  std::vector<int> vals{2, 4, 6, 8, 10};

  EXPECT_TRUE(mtx::none(vals, [](int i) { return (i % 2) == 1; }));
  EXPECT_FALSE(mtx::none(vals, [](int i) { return i == 4; }));

  EXPECT_TRUE(mtx::none(std::vector<int>{}, [](int i) { return i != 0; }));
}

TEST(ListUtils, all) {
  std::vector<int> vals{2, 4, 6, 8, 10};

  EXPECT_TRUE(mtx::all(vals, [](int i) { return (i % 2) == 0; }));
  EXPECT_FALSE(mtx::all(vals, [](int i) { return i > 7; }));

  EXPECT_TRUE(mtx::none(std::vector<int>{}, [](int i) { return i != 0; }));
}

}
