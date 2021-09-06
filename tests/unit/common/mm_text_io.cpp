#include "common/common_pch.h"

#include "common/mm_io_x.h"
#include "common/mm_mem_io.h"
#include "common/mm_proxy_io.h"
#include "common/mm_text_io.h"

#include "tests/unit/init.h"
#include "tests/unit/util.h"

namespace {

TEST(MmTextIo, BomUtf8) {
  unsigned char const text[14] = { 0xef, 0xbb, 0xbf, 'h', 'e', 'l', 'l', 'o', '\n', 'w', 'o', 'r', 'l', 'd' };
  mm_text_io_c in{std::make_shared<mm_mem_io_c>(text, 14)};

  ASSERT_EQ("UTF-8"s, in.get_encoding());
  EXPECT_EQ("hello"s, in.getline());
  EXPECT_EQ("world"s, in.getline());
}

TEST(MmTextIo, BomUtf16Le) {
  unsigned char const text[24] = { 0xff, 0xfe, 'h', 0, 'e', 0, 'l', 0, 'l', 0, 'o', 0, '\n', 0, 'w', 0, 'o', 0, 'r', 0, 'l', 0, 'd', 0, };
  mm_text_io_c in{std::make_shared<mm_mem_io_c>(text, 24)};

  ASSERT_EQ("UTF-16LE"s, in.get_encoding());
  EXPECT_EQ("hello"s, in.getline());
  EXPECT_EQ("world"s, in.getline());
}

TEST(MmTextIo, BomUtf16Be) {
  unsigned char const text[24] = { 0xfe, 0xff, 0, 'h', 0, 'e', 0, 'l', 0, 'l', 0, 'o', 0, '\n', 0, 'w', 0, 'o', 0, 'r', 0, 'l', 0, 'd' };
  mm_text_io_c in{std::make_shared<mm_mem_io_c>(text, 24)};

  ASSERT_EQ("UTF-16BE"s, in.get_encoding());
  EXPECT_EQ("hello"s, in.getline());
  EXPECT_EQ("world"s, in.getline());
}

TEST(MmTextIo, BomUtf32Le) {
  unsigned char const text[48] = { 0xff, 0xfe, 0, 0, 'h', 0, 0, 0, 'e', 0, 0, 0, 'l', 0, 0, 0, 'l', 0, 0, 0, 'o', 0, 0, 0, '\n', 0, 0, 0, 'w', 0, 0, 0, 'o', 0, 0, 0, 'r', 0, 0, 0, 'l', 0, 0, 0, 'd', 0, 0, 0, };
  mm_text_io_c in{std::make_shared<mm_mem_io_c>(text, 48)};

  ASSERT_EQ("UTF-32LE"s, in.get_encoding());
  EXPECT_EQ("hello"s, in.getline());
  EXPECT_EQ("world"s, in.getline());
}

TEST(MmTextIo, BomUtf32Be) {
  unsigned char const text[48] = { 0x00, 0x00, 0xfe, 0xff, 0, 0, 0, 'h', 0, 0, 0, 'e', 0, 0, 0, 'l', 0, 0, 0, 'l', 0, 0, 0, 'o', 0, 0, 0, '\n', 0, 0, 0, 'w', 0, 0, 0, 'o', 0, 0, 0, 'r', 0, 0, 0, 'l', 0, 0, 0, 'd' };
  mm_text_io_c in{std::make_shared<mm_mem_io_c>(text, 48)};

  ASSERT_EQ("UTF-32BE"s, in.get_encoding());
  EXPECT_EQ("hello"s, in.getline());
  EXPECT_EQ("world"s, in.getline());
}

}
