#include "common/common_pch.h"

#include "common/memory.h"

#include "gtest/gtest.h"

namespace {

TEST(Memory, OperatorEq) {
  auto m1 = memory_c::clone("hello");
  auto m2 = memory_c::alloc(5);
  auto m3 = memory_c::clone("world");
  std::memcpy(m2->get_buffer(), "hello", 5);

  EXPECT_TRUE(*m1 == *m2);
  EXPECT_TRUE(*m1 == "hello");
  EXPECT_FALSE(*m1 == *m3);
  EXPECT_FALSE(*m1 == "world");
}

TEST(Memory, OperatorNotEq) {
  auto m1 = memory_c::clone("hello");
  auto m2 = memory_c::alloc(5);
  auto m3 = memory_c::clone("world");
  std::memcpy(m2->get_buffer(), "hello", 5);

  EXPECT_FALSE(*m1 != *m2);
  EXPECT_FALSE(*m1 != "hello");
  EXPECT_TRUE(*m1 != *m3);
  EXPECT_TRUE(*m1 != "world");
}

}
