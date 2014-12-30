#include "common/common_pch.h"

#include "common/bit_cursor.h"
#include "common/endian.h"

#include "gtest/gtest.h"

namespace {

// 0xf    7    2    3    4    a    8    1
//   1111 0111 0010 0011 0100 1010 1000 0001

TEST(BitReader, Initialization) {
  unsigned char value[4];
  put_uint32_be(value, 0xf7234a81);
  auto b = bit_reader_c{value, 4};

  EXPECT_EQ(b.get_bit_position(),    0);
  EXPECT_EQ(b.get_remaining_bits(), 32);
  EXPECT_FALSE(b.eof());
}

TEST(BitReader, GetBit) {
  unsigned char value[4];
  put_uint32_be(value, 0xf7234a81);
  auto b = bit_reader_c{value, 4};

  // f
  EXPECT_TRUE(b.get_bit());
  EXPECT_TRUE(b.get_bit());
  EXPECT_TRUE(b.get_bit());
  EXPECT_TRUE(b.get_bit());
  // 7
  EXPECT_FALSE(b.get_bit());
  EXPECT_TRUE(b.get_bit());
  EXPECT_TRUE(b.get_bit());
  EXPECT_TRUE(b.get_bit());
  // 2
  EXPECT_FALSE(b.get_bit());
  EXPECT_FALSE(b.get_bit());
  EXPECT_TRUE(b.get_bit());
  EXPECT_FALSE(b.get_bit());
  // 3
  EXPECT_FALSE(b.get_bit());
  EXPECT_FALSE(b.get_bit());
  EXPECT_TRUE(b.get_bit());
  EXPECT_TRUE(b.get_bit());

  EXPECT_EQ(b.get_bit_position(),   16);
  EXPECT_EQ(b.get_remaining_bits(), 16);
  EXPECT_FALSE(b.eof());
}

TEST(BitReader, GetBits) {
  unsigned char value[4];
  put_uint32_be(value, 0xf7234a81);
  auto b = bit_reader_c{value, 4};

  // f7
  EXPECT_EQ(b.get_bits(4), 0x0f);
  EXPECT_EQ(b.get_bits(4), 0x07);

  // 234
  EXPECT_EQ(b.get_bits(12), 0x234);

  // a81 = 101 010 000
  EXPECT_EQ(b.get_bits(3), 0x05);
  EXPECT_EQ(b.get_bits(3), 0x02);
  EXPECT_EQ(b.get_bits(3), 0x00);

  EXPECT_EQ(b.get_bit_position(),   29);
  EXPECT_EQ(b.get_remaining_bits(),  3);
  EXPECT_FALSE(b.eof());
}

TEST(BitReader, GetUnsignedGolomb) {
  unsigned char value[4];
  put_uint32_be(value, 0xf7234a81);
  auto b = bit_reader_c{value, 4};

  // 1111 0111
  EXPECT_EQ(b.get_unsigned_golomb(), 0);
  EXPECT_EQ(b.get_bit_position(), 1);
  EXPECT_EQ(b.get_bits(3), 0x07);

  EXPECT_EQ(b.get_unsigned_golomb(), 2);
  EXPECT_EQ(b.get_bit_position(), 7);
  EXPECT_EQ(b.get_bits(1), 0x01);

  // 0010 0011
  EXPECT_EQ(b.get_unsigned_golomb(), 3);
  EXPECT_EQ(b.get_bit_position(), 13);
}

TEST(BitReader, GetSignedGolomb) {
  unsigned char value[4];
  put_uint32_be(value, 0xf7234a81);
  auto b = bit_reader_c{value, 4};

  // 1111 0111
  EXPECT_EQ(b.get_signed_golomb(), 0);
  EXPECT_EQ(b.get_bit_position(), 1);
  EXPECT_EQ(b.get_bits(3), 0x07);

  EXPECT_EQ(b.get_signed_golomb(), -1);
  EXPECT_EQ(b.get_bit_position(), 7);
  EXPECT_EQ(b.get_bits(1), 0x01);

  // 0010 0011
  EXPECT_EQ(b.get_signed_golomb(), 2);
  EXPECT_EQ(b.get_bit_position(), 13);
}

TEST(BitReader, PeekBits) {
  unsigned char value[4];
  put_uint32_be(value, 0xf7234a81);
  auto b = bit_reader_c{value, 4};

  // f7
  EXPECT_EQ(b.peek_bits(4), 0x0f);
  EXPECT_EQ(b.peek_bits(4), 0x0f);

  // f72
  EXPECT_EQ(b.peek_bits(12), 0xf72);

  EXPECT_EQ(b.get_bit_position(),    0);
  EXPECT_EQ(b.get_remaining_bits(), 32);
  EXPECT_FALSE(b.eof());
}

TEST(BitReader, GetUnary) {
  unsigned char value[4];
  put_uint32_be(value, 0xf7234a81);
  auto b = bit_reader_c{value, 4};

  // f7
  EXPECT_EQ(b.get_unary(1, 8), 0);
  EXPECT_EQ(b.get_unary(1, 8), 0);
  EXPECT_EQ(b.get_unary(1, 8), 0);
  EXPECT_EQ(b.get_unary(1, 8), 0);
  EXPECT_EQ(b.get_unary(1, 8), 1);

  EXPECT_EQ(b.get_bit_position(), 6);

  b.set_bit_position(0);
  EXPECT_EQ(b.get_unary(0, 3), 3);
  EXPECT_EQ(b.get_bit_position(), 3);

  b.set_bit_position(0);
  EXPECT_EQ(b.get_unary(0, 8), 4);
  EXPECT_EQ(b.get_bit_position(), 5);
}

TEST(BitReader, Get012) {
  unsigned char value[4];
  put_uint32_be(value, 0xf7234a81);
  auto b = bit_reader_c{value, 4};

  // f7
  EXPECT_EQ(b.get_012(), 2);
  EXPECT_EQ(b.get_012(), 2);
  EXPECT_EQ(b.get_012(), 0);
  EXPECT_EQ(b.get_012(), 2);
  EXPECT_EQ(b.get_012(), 1);

  EXPECT_EQ(b.get_bit_position(), 9);
}

TEST(BitReader, ByteAlign) {
  unsigned char value[4];
  put_uint32_be(value, 0xf7234a81);
  auto b = bit_reader_c{value, 4};

  // f7
  b.byte_align();
  EXPECT_EQ(b.peek_bits(8), 0xf7);
  EXPECT_EQ(b.get_bit_position(), 0);

  EXPECT_EQ(b.get_bits(2), 0x03);
  b.byte_align();
  EXPECT_EQ(b.peek_bits(8), 0x23);
  EXPECT_EQ(b.get_bit_position(), 8);

  EXPECT_EQ(b.get_bits(7), 0x11);
  b.byte_align();
  EXPECT_EQ(b.peek_bits(8), 0x4a);
  EXPECT_EQ(b.get_bit_position(), 16);
}

TEST(BitReader, GetBytes) {
  unsigned char value[4], target[2];
  put_uint32_be(value, 0xf7234a81);
  auto b = bit_reader_c{value, 4};

  // f7
  std::memset(target, 0, 2);
  EXPECT_NO_THROW(b.get_bytes(target, 1));
  EXPECT_EQ(target[0], 0xf7);
  EXPECT_EQ(b.get_bit_position(), 8);

  std::memset(target, 0, 2);
  EXPECT_EQ(b.get_bits(4), 0x02);
  EXPECT_NO_THROW(b.get_bytes(target, 2));
  EXPECT_EQ(target[0], 0x34);
  EXPECT_EQ(target[1], 0xa8);
  EXPECT_EQ(b.get_bit_position(), 28);
}

TEST(BitReader, SkipBits) {
  unsigned char value[4];
  put_uint32_be(value, 0xf7234a81);
  auto b = bit_reader_c{value, 4};

  // f7
  EXPECT_EQ(b.get_bit_position(), 0);
  EXPECT_NO_THROW(b.skip_bits(3));
  EXPECT_EQ(b.get_bit_position(), 3);
  EXPECT_NO_THROW(b.skip_bits(7));
  EXPECT_EQ(b.get_bit_position(), 10);
  EXPECT_NO_THROW(b.skip_bit());
  EXPECT_EQ(b.get_bit_position(), 11);
}

TEST(BitReader, SetBitPosition) {
  unsigned char value[4];
  put_uint32_be(value, 0xf7234a81);
  auto b = bit_reader_c{value, 4};

  // f7
  EXPECT_EQ(b.get_bit_position(), 0);

  EXPECT_NO_THROW(b.set_bit_position(0));
  EXPECT_EQ(b.get_bit_position(), 0);

  EXPECT_NO_THROW(b.set_bit_position(8));
  EXPECT_EQ(b.get_bit_position(), 8);

  EXPECT_NO_THROW(b.set_bit_position(0));
  EXPECT_EQ(b.get_bit_position(), 0);

  EXPECT_NO_THROW(b.set_bit_position(31));
  EXPECT_EQ(b.get_bit_position(), 31);

  EXPECT_THROW(b.set_bit_position(32), mtx::mm_io::end_of_file_x);
}

TEST(BitReader, ExceptionOnReadingBeyondEOF) {
  unsigned char value[4], target[2];
  put_uint32_be(value, 0xf7234a81);
  auto b = bit_reader_c{value, 4};

  EXPECT_NO_THROW(b.set_bit_position(31));
  EXPECT_TRUE(b.get_bit());
  EXPECT_THROW(b.get_bit(), mtx::mm_io::end_of_file_x);

  b = bit_reader_c{value, 4};
  EXPECT_NO_THROW(b.set_bit_position(24));
  EXPECT_EQ(b.get_bits(8), 0x81);
  EXPECT_THROW(b.get_bit(), mtx::mm_io::end_of_file_x);

  b = bit_reader_c{value, 4};
  EXPECT_NO_THROW(b.set_bit_position(25));
  EXPECT_THROW(b.get_bits(8), mtx::mm_io::end_of_file_x);

  std::memset(target, 0, 2);
  b = bit_reader_c{value, 4};
  EXPECT_NO_THROW(b.set_bit_position(0));
  EXPECT_NO_THROW(b.get_bytes(target, 1));
  EXPECT_EQ(target[0], 0xf7);

  std::memset(target, 0, 2);
  b = bit_reader_c{value, 4};
  EXPECT_NO_THROW(b.set_bit_position(0));
  EXPECT_NO_THROW(b.get_bytes(target, 2));
  EXPECT_EQ(get_uint16_be(target), 0xf723);

  std::memset(target, 0, 2);
  b = bit_reader_c{value, 4};
  EXPECT_NO_THROW(b.set_bit_position(4));
  EXPECT_NO_THROW(b.get_bytes(target, 2));
  EXPECT_EQ(get_uint16_be(target), 0x7234);

  std::memset(target, 0, 2);
  b = bit_reader_c{value, 4};
  EXPECT_NO_THROW(b.set_bit_position(24));
  EXPECT_NO_THROW(b.get_bytes(target, 1));
  EXPECT_EQ(target[0], 0x81);

  std::memset(target, 0, 2);
  b = bit_reader_c{value, 4};
  EXPECT_NO_THROW(b.set_bit_position(16));
  EXPECT_NO_THROW(b.get_bytes(target, 2));
  EXPECT_EQ(get_uint16_be(target), 0x4a81);

  std::memset(target, 0, 2);
  b = bit_reader_c{value, 4};
  EXPECT_NO_THROW(b.set_bit_position(25));
  EXPECT_THROW(b.get_bytes(target, 1), mtx::mm_io::end_of_file_x);

  std::memset(target, 0, 2);
  b = bit_reader_c{value, 4};
  EXPECT_NO_THROW(b.set_bit_position(24));
  EXPECT_THROW(b.get_bytes(target, 2), mtx::mm_io::end_of_file_x);
}

}
