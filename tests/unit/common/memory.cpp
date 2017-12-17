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

TEST(Memory, SpliceInsertionIsSmallerAtBeginning) {
  auto buffer    = memory_c::clone("0123456789");
  auto to_insert = memory_c::clone("qwe");

  memory_c::splice(*buffer, 0, 5, *to_insert);

  ASSERT_EQ(8,                       buffer->get_size());
  ASSERT_EQ(std::string{"qwe56789"}, buffer->to_string());
}

TEST(Memory, SpliceInsertionIsEqualAtBeginning) {
  auto buffer    = memory_c::clone("0123456789");
  auto to_insert = memory_c::clone("qwe");

  memory_c::splice(*buffer, 0, 3, *to_insert);

  ASSERT_EQ(10,                        buffer->get_size());
  ASSERT_EQ(std::string{"qwe3456789"}, buffer->to_string());
}

TEST(Memory, SpliceInsertionIsBiggerAtBeginning) {
  auto buffer    = memory_c::clone("0123456789");
  auto to_insert = memory_c::clone("qwert");

  memory_c::splice(*buffer, 0, 3, *to_insert);

  ASSERT_EQ(12,                          buffer->get_size());
  ASSERT_EQ(std::string{"qwert3456789"}, buffer->to_string());
}

TEST(Memory, SpliceNoRemovalAtBeginning) {
  auto buffer    = memory_c::clone("0123456789");
  auto to_insert = memory_c::clone("qwert");

  memory_c::splice(*buffer, 0, 0, *to_insert);

  ASSERT_EQ(15,                             buffer->get_size());
  ASSERT_EQ(std::string{"qwert0123456789"}, buffer->to_string());
}

TEST(Memory, SpliceNoInsertionAtBeginning) {
  auto buffer = memory_c::clone("0123456789");

  memory_c::splice(*buffer, 0, 3);

  ASSERT_EQ(7,                      buffer->get_size());
  ASSERT_EQ(std::string{"3456789"}, buffer->to_string());
}

TEST(Memory, SpliceInsertionIsSmallerInMiddle) {
  auto buffer    = memory_c::clone("0123456789");
  auto to_insert = memory_c::clone("qwe");

  memory_c::splice(*buffer, 2, 5, *to_insert);

  ASSERT_EQ(8,                       buffer->get_size());
  ASSERT_EQ(std::string{"01qwe789"}, buffer->to_string());
}

TEST(Memory, SpliceInsertionIsEqualInMiddle) {
  auto buffer    = memory_c::clone("0123456789");
  auto to_insert = memory_c::clone("qwe");

  memory_c::splice(*buffer, 2, 3, *to_insert);

  ASSERT_EQ(10,                        buffer->get_size());
  ASSERT_EQ(std::string{"01qwe56789"}, buffer->to_string());
}

TEST(Memory, SpliceInsertionIsBiggerInMiddle) {
  auto buffer    = memory_c::clone("0123456789");
  auto to_insert = memory_c::clone("qwert");

  memory_c::splice(*buffer, 2, 3, *to_insert);

  ASSERT_EQ(12,                          buffer->get_size());
  ASSERT_EQ(std::string{"01qwert56789"}, buffer->to_string());
}

TEST(Memory, SpliceNoRemovalInMiddle) {
  auto buffer    = memory_c::clone("0123456789");
  auto to_insert = memory_c::clone("qwert");

  memory_c::splice(*buffer, 2, 0, *to_insert);

  ASSERT_EQ(15,                             buffer->get_size());
  ASSERT_EQ(std::string{"01qwert23456789"}, buffer->to_string());
}

TEST(Memory, SpliceNoInsertionInMiddle) {
  auto buffer = memory_c::clone("0123456789");

  memory_c::splice(*buffer, 2, 3);

  ASSERT_EQ(7,                      buffer->get_size());
  ASSERT_EQ(std::string{"0156789"}, buffer->to_string());
}

TEST(Memory, SpliceInsertionIsSmallerAtEnd) {
  auto buffer    = memory_c::clone("0123456789");
  auto to_insert = memory_c::clone("qwe");

  memory_c::splice(*buffer, 5, 5, *to_insert);

  ASSERT_EQ(8,                       buffer->get_size());
  ASSERT_EQ(std::string{"01234qwe"}, buffer->to_string());
}

TEST(Memory, SpliceInsertionIsEqualAtEnd) {
  auto buffer    = memory_c::clone("0123456789");
  auto to_insert = memory_c::clone("qwe");

  memory_c::splice(*buffer, 7, 3, *to_insert);

  ASSERT_EQ(10,                        buffer->get_size());
  ASSERT_EQ(std::string{"0123456qwe"}, buffer->to_string());
}

TEST(Memory, SpliceInsertionIsBiggerAtEnd) {
  auto buffer    = memory_c::clone("0123456789");
  auto to_insert = memory_c::clone("qwert");

  memory_c::splice(*buffer, 7, 3, *to_insert);

  ASSERT_EQ(12,                          buffer->get_size());
  ASSERT_EQ(std::string{"0123456qwert"}, buffer->to_string());
}

TEST(Memory, SpliceNoRemovalAtEnd) {
  auto buffer    = memory_c::clone("0123456789");
  auto to_insert = memory_c::clone("qwert");

  memory_c::splice(*buffer, 10, 0, *to_insert);

  ASSERT_EQ(15,                             buffer->get_size());
  ASSERT_EQ(std::string{"0123456789qwert"}, buffer->to_string());
}

TEST(Memory, SpliceNoInsertionAtEnd) {
  auto buffer = memory_c::clone("0123456789");

  memory_c::splice(*buffer, 7, 3);

  ASSERT_EQ(7,                      buffer->get_size());
  ASSERT_EQ(std::string{"0123456"}, buffer->to_string());
}

TEST(Memory, SpliceInvalidParameters) {
  ASSERT_NO_THROW(memory_c::splice(*memory_c::clone("0123456789"), 2, 3));
  ASSERT_THROW(memory_c::splice(*memory_c::clone("0123456789"), 11, 0), std::invalid_argument);
  ASSERT_THROW(memory_c::splice(*memory_c::clone("0123456789"), 10, 1), std::invalid_argument);
  ASSERT_NO_THROW(memory_c::splice(*memory_c::clone("0123456789"), 10, 0));
}

}
