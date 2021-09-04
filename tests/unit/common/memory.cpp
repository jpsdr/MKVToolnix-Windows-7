#include "common/common_pch.h"

#include "common/endian.h"
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
  ASSERT_EQ("qwe56789"s, buffer->to_string());
}

TEST(Memory, SpliceInsertionIsEqualAtBeginning) {
  auto buffer    = memory_c::clone("0123456789");
  auto to_insert = memory_c::clone("qwe");

  memory_c::splice(*buffer, 0, 3, *to_insert);

  ASSERT_EQ(10,                        buffer->get_size());
  ASSERT_EQ("qwe3456789"s, buffer->to_string());
}

TEST(Memory, SpliceInsertionIsBiggerAtBeginning) {
  auto buffer    = memory_c::clone("0123456789");
  auto to_insert = memory_c::clone("qwert");

  memory_c::splice(*buffer, 0, 3, *to_insert);

  ASSERT_EQ(12,                          buffer->get_size());
  ASSERT_EQ("qwert3456789"s, buffer->to_string());
}

TEST(Memory, SpliceNoRemovalAtBeginning) {
  auto buffer    = memory_c::clone("0123456789");
  auto to_insert = memory_c::clone("qwert");

  memory_c::splice(*buffer, 0, 0, *to_insert);

  ASSERT_EQ(15,                             buffer->get_size());
  ASSERT_EQ("qwert0123456789"s, buffer->to_string());
}

TEST(Memory, SpliceNoInsertionAtBeginning) {
  auto buffer = memory_c::clone("0123456789");

  memory_c::splice(*buffer, 0, 3);

  ASSERT_EQ(7,                      buffer->get_size());
  ASSERT_EQ("3456789"s, buffer->to_string());
}

TEST(Memory, SpliceInsertionIsSmallerInMiddle) {
  auto buffer    = memory_c::clone("0123456789");
  auto to_insert = memory_c::clone("qwe");

  memory_c::splice(*buffer, 2, 5, *to_insert);

  ASSERT_EQ(8,                       buffer->get_size());
  ASSERT_EQ("01qwe789"s, buffer->to_string());
}

TEST(Memory, SpliceInsertionIsEqualInMiddle) {
  auto buffer    = memory_c::clone("0123456789");
  auto to_insert = memory_c::clone("qwe");

  memory_c::splice(*buffer, 2, 3, *to_insert);

  ASSERT_EQ(10,                        buffer->get_size());
  ASSERT_EQ("01qwe56789"s, buffer->to_string());
}

TEST(Memory, SpliceInsertionIsBiggerInMiddle) {
  auto buffer    = memory_c::clone("0123456789");
  auto to_insert = memory_c::clone("qwert");

  memory_c::splice(*buffer, 2, 3, *to_insert);

  ASSERT_EQ(12,                          buffer->get_size());
  ASSERT_EQ("01qwert56789"s, buffer->to_string());
}

TEST(Memory, SpliceNoRemovalInMiddle) {
  auto buffer    = memory_c::clone("0123456789");
  auto to_insert = memory_c::clone("qwert");

  memory_c::splice(*buffer, 2, 0, *to_insert);

  ASSERT_EQ(15,                             buffer->get_size());
  ASSERT_EQ("01qwert23456789"s, buffer->to_string());
}

TEST(Memory, SpliceNoInsertionInMiddle) {
  auto buffer = memory_c::clone("0123456789");

  memory_c::splice(*buffer, 2, 3);

  ASSERT_EQ(7,                      buffer->get_size());
  ASSERT_EQ("0156789"s, buffer->to_string());
}

TEST(Memory, SpliceInsertionIsSmallerAtEnd) {
  auto buffer    = memory_c::clone("0123456789");
  auto to_insert = memory_c::clone("qwe");

  memory_c::splice(*buffer, 5, 5, *to_insert);

  ASSERT_EQ(8,                       buffer->get_size());
  ASSERT_EQ("01234qwe"s, buffer->to_string());
}

TEST(Memory, SpliceInsertionIsEqualAtEnd) {
  auto buffer    = memory_c::clone("0123456789");
  auto to_insert = memory_c::clone("qwe");

  memory_c::splice(*buffer, 7, 3, *to_insert);

  ASSERT_EQ(10,                        buffer->get_size());
  ASSERT_EQ("0123456qwe"s, buffer->to_string());
}

TEST(Memory, SpliceInsertionIsBiggerAtEnd) {
  auto buffer    = memory_c::clone("0123456789");
  auto to_insert = memory_c::clone("qwert");

  memory_c::splice(*buffer, 7, 3, *to_insert);

  ASSERT_EQ(12,                          buffer->get_size());
  ASSERT_EQ("0123456qwert"s, buffer->to_string());
}

TEST(Memory, SpliceNoRemovalAtEnd) {
  auto buffer    = memory_c::clone("0123456789");
  auto to_insert = memory_c::clone("qwert");

  memory_c::splice(*buffer, 10, 0, *to_insert);

  ASSERT_EQ(15,                             buffer->get_size());
  ASSERT_EQ("0123456789qwert"s, buffer->to_string());
}

TEST(Memory, SpliceNoInsertionAtEnd) {
  auto buffer = memory_c::clone("0123456789");

  memory_c::splice(*buffer, 7, 3);

  ASSERT_EQ(7,                      buffer->get_size());
  ASSERT_EQ("0123456"s, buffer->to_string());
}

TEST(Memory, SpliceInvalidParameters) {
  ASSERT_NO_THROW(memory_c::splice(*memory_c::clone("0123456789"), 2, 3));
  ASSERT_THROW(memory_c::splice(*memory_c::clone("0123456789"), 11, 0), std::invalid_argument);
  ASSERT_THROW(memory_c::splice(*memory_c::clone("0123456789"), 10, 1), std::invalid_argument);
  ASSERT_NO_THROW(memory_c::splice(*memory_c::clone("0123456789"), 10, 0));
}

TEST(Memory, Add) {
  auto buffer1 = memory_c::clone("0123456");
  auto buffer2 = memory_c::clone("789");

  buffer1->add(buffer2);

  ASSERT_EQ(10,            buffer1->get_size());
  ASSERT_EQ("0123456789"s, buffer1->to_string());
}

TEST(Memory, AddNothing) {
  auto buffer = memory_c::clone("0123456");

  buffer->add(nullptr, 0);

  ASSERT_EQ(7,          buffer->get_size());
  ASSERT_EQ("0123456"s, buffer->to_string());

  buffer->add(nullptr, 2);

  ASSERT_EQ(7,          buffer->get_size());
  ASSERT_EQ("0123456"s, buffer->to_string());

  buffer->add(reinterpret_cast<unsigned char const *>("789"), 0);

  ASSERT_EQ(7,          buffer->get_size());
  ASSERT_EQ("0123456"s, buffer->to_string());
}

TEST(Memory, Prepend) {
  auto buffer1 = memory_c::clone("0123456");
  auto buffer2 = memory_c::clone("789");

  buffer1->prepend(buffer2);

  ASSERT_EQ(10,            buffer1->get_size());
  ASSERT_EQ("7890123456"s, buffer1->to_string());
}

TEST(Memory, PrependNothing) {
  auto buffer = memory_c::clone("0123456");

  buffer->prepend(nullptr, 0);

  ASSERT_EQ(7,          buffer->get_size());
  ASSERT_EQ("0123456"s, buffer->to_string());

  buffer->prepend(nullptr, 2);

  ASSERT_EQ(7,          buffer->get_size());
  ASSERT_EQ("0123456"s, buffer->to_string());

  buffer->prepend(reinterpret_cast<unsigned char const *>("789"), 0);

  ASSERT_EQ(7,          buffer->get_size());
  ASSERT_EQ("0123456"s, buffer->to_string());
}

TEST(Memory, OperatorBrackets) {
  auto buffer1 = memory_c::alloc(4);

  for (unsigned char idx = 0; idx < 4; ++idx)
    (*buffer1)[idx] = idx + 1;

  ASSERT_EQ(1, (*buffer1)[0]);
  ASSERT_EQ(2, (*buffer1)[1]);
  ASSERT_EQ(3, (*buffer1)[2]);
  ASSERT_EQ(4, (*buffer1)[3]);
  ASSERT_EQ(0x01020304, get_uint32_be(buffer1->get_buffer()));
  ASSERT_EQ(0x01020304, get_uint32_be(buffer1->get_buffer()));
  ASSERT_EQ(0x01020304, get_uint32_be(buffer1->get_buffer()));
  ASSERT_EQ(0x01020304, get_uint32_be(buffer1->get_buffer()));

  auto buffer2        = memory_c::clone("hello");
  auto const &buffer3 = *buffer2;

  ASSERT_EQ('h', buffer3[0]);
  ASSERT_EQ('e', buffer3[1]);
  ASSERT_EQ('l', buffer3[2]);
  ASSERT_EQ('l', buffer3[3]);
  ASSERT_EQ('o', buffer3[4]);
}

}
