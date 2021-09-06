#include "common/common_pch.h"

#include "common/endian.h"

#include "tests/unit/init.h"

namespace {

TEST(Endian, GetUIntXYBE) {
  unsigned char buffer[8] = { 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef };

  EXPECT_EQ(              0x0123u, get_uint16_be(buffer));
  EXPECT_EQ(            0x012345u, get_uint24_be(buffer));
  EXPECT_EQ(          0x01234567u, get_uint32_be(buffer));
  EXPECT_EQ(0x0123456789abcdefull, get_uint64_be(buffer));

  EXPECT_EQ(              0x01ull, get_uint_be(buffer, 0));
  EXPECT_EQ(              0x01ull, get_uint_be(buffer, 1));
  EXPECT_EQ(            0x0123ull, get_uint_be(buffer, 2));
  EXPECT_EQ(          0x012345ull, get_uint_be(buffer, 3));
  EXPECT_EQ(        0x01234567ull, get_uint_be(buffer, 4));
  EXPECT_EQ(      0x0123456789ull, get_uint_be(buffer, 5));
  EXPECT_EQ(    0x0123456789abull, get_uint_be(buffer, 6));
  EXPECT_EQ(  0x0123456789abcdull, get_uint_be(buffer, 7));
  EXPECT_EQ(0x0123456789abcdefull, get_uint_be(buffer, 8));
  EXPECT_EQ(0x0123456789abcdefull, get_uint_be(buffer, 9));
}

TEST(Endian, GetUIntXYLE) {
  unsigned char buffer[8] = { 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef };

  EXPECT_EQ(              0x2301u, get_uint16_le(buffer));
  EXPECT_EQ(            0x452301u, get_uint24_le(buffer));
  EXPECT_EQ(          0x67452301u, get_uint32_le(buffer));
  EXPECT_EQ(0xefcdab8967452301ull, get_uint64_le(buffer));

  EXPECT_EQ(              0x01ull, get_uint_le(buffer, 0));
  EXPECT_EQ(              0x01ull, get_uint_le(buffer, 1));
  EXPECT_EQ(            0x2301ull, get_uint_le(buffer, 2));
  EXPECT_EQ(          0x452301ull, get_uint_le(buffer, 3));
  EXPECT_EQ(        0x67452301ull, get_uint_le(buffer, 4));
  EXPECT_EQ(      0x8967452301ull, get_uint_le(buffer, 5));
  EXPECT_EQ(    0xab8967452301ull, get_uint_le(buffer, 6));
  EXPECT_EQ(  0xcdab8967452301ull, get_uint_le(buffer, 7));
  EXPECT_EQ(0xefcdab8967452301ull, get_uint_le(buffer, 8));
  EXPECT_EQ(0xefcdab8967452301ull, get_uint_le(buffer, 9));
}

TEST(Endian, PutUIntXYBE) {
  unsigned char buffer[8];

  std::memset(buffer, 0, 8);
  unsigned char res16be[8] = { 0x01, 0x23, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
  put_uint16_be(buffer, 0x0123u);
  EXPECT_EQ(0, std::memcmp(buffer, res16be, 8));

  std::memset(buffer, 0, 8);
  unsigned char res24be[8] = { 0x01, 0x23, 0x45, 0x00, 0x00, 0x00, 0x00, 0x00 };
  put_uint24_be(buffer, 0x012345u);
  EXPECT_EQ(0, std::memcmp(buffer, res24be, 8));

  std::memset(buffer, 0, 8);
  unsigned char res32be[8] = { 0x01, 0x23, 0x45, 0x67, 0x00, 0x00, 0x00, 0x00 };
  put_uint32_be(buffer, 0x01234567u);
  EXPECT_EQ(0, std::memcmp(buffer, res32be, 8));

  std::memset(buffer, 0, 8);
  unsigned char res64be[8] = { 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef };
  put_uint64_be(buffer, 0x0123456789abcdefu);
  EXPECT_EQ(0, std::memcmp(buffer, res64be, 8));

  unsigned char resbe1[8] = { 0xef, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
  unsigned char resbe2[8] = { 0xcd, 0xef, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
  unsigned char resbe3[8] = { 0xab, 0xcd, 0xef, 0x00, 0x00, 0x00, 0x00, 0x00 };
  unsigned char resbe4[8] = { 0x89, 0xab, 0xcd, 0xef, 0x00, 0x00, 0x00, 0x00 };
  unsigned char resbe5[8] = { 0x67, 0x89, 0xab, 0xcd, 0xef, 0x00, 0x00, 0x00 };
  unsigned char resbe6[8] = { 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef, 0x00, 0x00 };
  unsigned char resbe7[8] = { 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef, 0x00 };
  unsigned char resbe8[8] = { 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef };
  auto value              = 0x0123456789abcdefu;

  std::memset(buffer, 0, 8);
  put_uint_be(buffer, value, 0);
  EXPECT_EQ(0, std::memcmp(buffer, resbe1, 8));

  std::memset(buffer, 0, 8);
  put_uint_be(buffer, value, 1);
  EXPECT_EQ(0, std::memcmp(buffer, resbe1, 8));

  std::memset(buffer, 0, 8);
  put_uint_be(buffer, value, 2);
  EXPECT_EQ(0, std::memcmp(buffer, resbe2, 8));

  std::memset(buffer, 0, 8);
  put_uint_be(buffer, value, 3);
  EXPECT_EQ(0, std::memcmp(buffer, resbe3, 8));

  std::memset(buffer, 0, 8);
  put_uint_be(buffer, value, 4);
  EXPECT_EQ(0, std::memcmp(buffer, resbe4, 8));

  std::memset(buffer, 0, 8);
  put_uint_be(buffer, value, 5);
  EXPECT_EQ(0, std::memcmp(buffer, resbe5, 8));

  std::memset(buffer, 0, 8);
  put_uint_be(buffer, value, 6);
  EXPECT_EQ(0, std::memcmp(buffer, resbe6, 8));

  std::memset(buffer, 0, 8);
  put_uint_be(buffer, value, 7);
  EXPECT_EQ(0, std::memcmp(buffer, resbe7, 8));

  std::memset(buffer, 0, 8);
  put_uint_be(buffer, value, 8);
  EXPECT_EQ(0, std::memcmp(buffer, resbe8, 8));

  std::memset(buffer, 0, 8);
  put_uint_be(buffer, value, 9);
  EXPECT_EQ(0, std::memcmp(buffer, resbe8, 8));
}

TEST(Endian, PutUIntXYLE) {
  unsigned char buffer[8];

  std::memset(buffer, 0, 8);
  unsigned char res16le[8] = { 0x23, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
  put_uint16_le(buffer, 0x0123u);
  EXPECT_EQ(0, std::memcmp(buffer, res16le, 8));

  std::memset(buffer, 0, 8);
  unsigned char res24le[8] = { 0x45, 0x23, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00 };
  put_uint24_le(buffer, 0x012345u);
  EXPECT_EQ(0, std::memcmp(buffer, res24le, 8));

  std::memset(buffer, 0, 8);
  unsigned char res32le[8] = { 0x67, 0x45, 0x23, 0x01, 0x00, 0x00, 0x00, 0x00 };
  put_uint32_le(buffer, 0x01234567u);
  EXPECT_EQ(0, std::memcmp(buffer, res32le, 8));

  std::memset(buffer, 0, 8);
  unsigned char res64le[8] = { 0xef, 0xcd, 0xab, 0x89, 0x67, 0x45, 0x23, 0x01 };
  put_uint64_le(buffer, 0x0123456789abcdefu);
  EXPECT_EQ(0, std::memcmp(buffer, res64le, 8));

  unsigned char resle1[8] = { 0xef, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
  unsigned char resle2[8] = { 0xef, 0xcd, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
  unsigned char resle3[8] = { 0xef, 0xcd, 0xab, 0x00, 0x00, 0x00, 0x00, 0x00 };
  unsigned char resle4[8] = { 0xef, 0xcd, 0xab, 0x89, 0x00, 0x00, 0x00, 0x00 };
  unsigned char resle5[8] = { 0xef, 0xcd, 0xab, 0x89, 0x67, 0x00, 0x00, 0x00 };
  unsigned char resle6[8] = { 0xef, 0xcd, 0xab, 0x89, 0x67, 0x45, 0x00, 0x00 };
  unsigned char resle7[8] = { 0xef, 0xcd, 0xab, 0x89, 0x67, 0x45, 0x23, 0x00 };
  unsigned char resle8[8] = { 0xef, 0xcd, 0xab, 0x89, 0x67, 0x45, 0x23, 0x01 };
  auto value              = 0x0123456789abcdefu;

  std::memset(buffer, 0, 8);
  put_uint_le(buffer, value, 0);
  EXPECT_EQ(0, std::memcmp(buffer, resle1, 8));

  std::memset(buffer, 0, 8);
  put_uint_le(buffer, value, 1);
  EXPECT_EQ(0, std::memcmp(buffer, resle1, 8));

  std::memset(buffer, 0, 8);
  put_uint_le(buffer, value, 2);
  EXPECT_EQ(0, std::memcmp(buffer, resle2, 8));

  std::memset(buffer, 0, 8);
  put_uint_le(buffer, value, 3);
  EXPECT_EQ(0, std::memcmp(buffer, resle3, 8));

  std::memset(buffer, 0, 8);
  put_uint_le(buffer, value, 4);
  EXPECT_EQ(0, std::memcmp(buffer, resle4, 8));

  std::memset(buffer, 0, 8);
  put_uint_le(buffer, value, 5);
  EXPECT_EQ(0, std::memcmp(buffer, resle5, 8));

  std::memset(buffer, 0, 8);
  put_uint_le(buffer, value, 6);
  EXPECT_EQ(0, std::memcmp(buffer, resle6, 8));

  std::memset(buffer, 0, 8);
  put_uint_le(buffer, value, 7);
  EXPECT_EQ(0, std::memcmp(buffer, resle7, 8));

  std::memset(buffer, 0, 8);
  put_uint_le(buffer, value, 8);
  EXPECT_EQ(0, std::memcmp(buffer, resle8, 8));

  std::memset(buffer, 0, 8);
  put_uint_le(buffer, value, 9);
  EXPECT_EQ(0, std::memcmp(buffer, resle8, 8));
}

}
