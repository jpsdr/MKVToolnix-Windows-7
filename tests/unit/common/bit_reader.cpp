#include "common/common_pch.h"

#include "common/bit_reader.h"
#include "common/endian.h"

#include "gtest/gtest.h"

namespace {

// 0xf    7    2    3    4    a    8    1
//   1111 0111 0010 0011 0100 1010 1000 0001

TEST(BitReader, Initialization) {
  unsigned char value[4];
  put_uint32_be(value, 0xf7234a81);
  auto b = bit_reader_c{value, 4};

  EXPECT_EQ( 0, b.get_bit_position());
  EXPECT_EQ(32, b.get_remaining_bits());
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

  EXPECT_EQ(16, b.get_bit_position());
  EXPECT_EQ(16, b.get_remaining_bits());
  EXPECT_FALSE(b.eof());
}

TEST(BitReader, GetBits) {
  unsigned char value[4];
  put_uint32_be(value, 0xf7234a81);
  auto b = bit_reader_c{value, 4};

  // f7
  EXPECT_EQ(0x0f, b.get_bits(4));
  EXPECT_EQ(0x07, b.get_bits(4));

  // 234
  EXPECT_EQ(0x234, b.get_bits(12));

  // a81 = 101 010 000
  EXPECT_EQ(0x05, b.get_bits(3));
  EXPECT_EQ(0x02, b.get_bits(3));
  EXPECT_EQ(0x00, b.get_bits(3));

  EXPECT_EQ(29, b.get_bit_position());
  EXPECT_EQ( 3, b.get_remaining_bits());
  EXPECT_FALSE(b.eof());
}

TEST(BitReader, GetUnsignedGolomb) {
  unsigned char value[4];
  put_uint32_be(value, 0xf7234a81);
  auto b = bit_reader_c{value, 4};

  // 1111 0111
  EXPECT_EQ(   0, b.get_unsigned_golomb());
  EXPECT_EQ(   1, b.get_bit_position());
  EXPECT_EQ(0x07, b.get_bits(3));

  EXPECT_EQ(   2, b.get_unsigned_golomb());
  EXPECT_EQ(   7, b.get_bit_position());
  EXPECT_EQ(0x01, b.get_bits(1));

  // 0010 0011
  EXPECT_EQ( 3, b.get_unsigned_golomb());
  EXPECT_EQ(13, b.get_bit_position());
}

TEST(BitReader, GetSignedGolomb) {
  unsigned char value[4];
  put_uint32_be(value, 0xf7234a81);
  auto b = bit_reader_c{value, 4};

  // 1111 0111
  EXPECT_EQ(   0, b.get_signed_golomb());
  EXPECT_EQ(   1, b.get_bit_position());
  EXPECT_EQ(0x07, b.get_bits(3));

  EXPECT_EQ(  -1, b.get_signed_golomb());
  EXPECT_EQ(   7, b.get_bit_position());
  EXPECT_EQ(0x01, b.get_bits(1));

  // 0010 0011
  EXPECT_EQ( 2, b.get_signed_golomb());
  EXPECT_EQ(13, b.get_bit_position());
}

TEST(BitReader, PeekBits) {
  unsigned char value[4];
  put_uint32_be(value, 0xf7234a81);
  auto b = bit_reader_c{value, 4};

  // f7
  EXPECT_EQ(0x0f, b.peek_bits(4));
  EXPECT_EQ(0x0f, b.peek_bits(4));

  // f72
  EXPECT_EQ(0xf72, b.peek_bits(12));

  EXPECT_EQ( 0, b.get_bit_position());
  EXPECT_EQ(32, b.get_remaining_bits());
  EXPECT_FALSE(b.eof());
}

TEST(BitReader, GetUnary) {
  unsigned char value[4];
  put_uint32_be(value, 0xf7234a81);
  auto b = bit_reader_c{value, 4};

  // f7
  EXPECT_EQ(0, b.get_unary(1, 8));
  EXPECT_EQ(0, b.get_unary(1, 8));
  EXPECT_EQ(0, b.get_unary(1, 8));
  EXPECT_EQ(0, b.get_unary(1, 8));
  EXPECT_EQ(1, b.get_unary(1, 8));

  EXPECT_EQ(6, b.get_bit_position());

  b.set_bit_position(0);
  EXPECT_EQ(3, b.get_unary(0, 3));
  EXPECT_EQ(3, b.get_bit_position());

  b.set_bit_position(0);
  EXPECT_EQ(4, b.get_unary(0, 8));
  EXPECT_EQ(5, b.get_bit_position());
}

TEST(BitReader, Get012) {
  unsigned char value[4];
  put_uint32_be(value, 0xf7234a81);
  auto b = bit_reader_c{value, 4};

  // f7
  EXPECT_EQ(2, b.get_012());
  EXPECT_EQ(2, b.get_012());
  EXPECT_EQ(0, b.get_012());
  EXPECT_EQ(2, b.get_012());
  EXPECT_EQ(1, b.get_012());

  EXPECT_EQ(9, b.get_bit_position());
}

TEST(BitReader, ByteAlign) {
  unsigned char value[4];
  put_uint32_be(value, 0xf7234a81);
  auto b = bit_reader_c{value, 4};

  // f7
  b.byte_align();
  EXPECT_EQ(0xf7, b.peek_bits(8));
  EXPECT_EQ(   0, b.get_bit_position());

  EXPECT_EQ(0x03, b.get_bits(2));
  b.byte_align();
  EXPECT_EQ(0x23, b.peek_bits(8));
  EXPECT_EQ(   8, b.get_bit_position());

  EXPECT_EQ(0x11, b.get_bits(7));
  b.byte_align();
  EXPECT_EQ(0x4a, b.peek_bits(8));
  EXPECT_EQ(  16, b.get_bit_position());
}

TEST(BitReader, GetBytes) {
  unsigned char value[4], target[2];
  put_uint32_be(value, 0xf7234a81);
  auto b = bit_reader_c{value, 4};

  // f7
  std::memset(target, 0, 2);
  EXPECT_NO_THROW(b.get_bytes(target, 1));
  EXPECT_EQ(0xf7, target[0]);
  EXPECT_EQ(   8, b.get_bit_position());

  std::memset(target, 0, 2);
  EXPECT_EQ(0x02, b.get_bits(4));
  EXPECT_NO_THROW(b.get_bytes(target, 2));
  EXPECT_EQ(0x34, target[0]);
  EXPECT_EQ(0xa8, target[1]);
  EXPECT_EQ(  28, b.get_bit_position());
}

TEST(BitReader, SkipBits) {
  unsigned char value[4];
  put_uint32_be(value, 0xf7234a81);
  auto b = bit_reader_c{value, 4};

  // f7
  EXPECT_EQ(0, b.get_bit_position());
  EXPECT_NO_THROW(b.skip_bits(3));
  EXPECT_EQ(3, b.get_bit_position());
  EXPECT_NO_THROW(b.skip_bits(7));
  EXPECT_EQ(10, b.get_bit_position());
  EXPECT_NO_THROW(b.skip_bit());
  EXPECT_EQ(11, b.get_bit_position());
}

TEST(BitReader, SetBitPosition) {
  unsigned char value[4];
  put_uint32_be(value, 0xf7234a81);
  auto b = bit_reader_c{value, 4};

  // f7
  EXPECT_EQ(0, b.get_bit_position());

  EXPECT_NO_THROW(b.set_bit_position(0));
  EXPECT_EQ(0, b.get_bit_position());

  EXPECT_NO_THROW(b.set_bit_position(8));
  EXPECT_EQ(8, b.get_bit_position());

  EXPECT_NO_THROW(b.set_bit_position(0));
  EXPECT_EQ(0, b.get_bit_position());

  EXPECT_NO_THROW(b.set_bit_position(31));
  EXPECT_EQ(31, b.get_bit_position());

  EXPECT_NO_THROW(b.set_bit_position(32));
  EXPECT_THROW(b.set_bit_position(33), mtx::mm_io::end_of_file_x);
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
  EXPECT_EQ(0x81, b.get_bits(8));
  EXPECT_THROW(b.get_bit(), mtx::mm_io::end_of_file_x);

  b = bit_reader_c{value, 4};
  EXPECT_NO_THROW(b.set_bit_position(25));
  EXPECT_THROW(b.get_bits(8), mtx::mm_io::end_of_file_x);

  std::memset(target, 0, 2);
  b = bit_reader_c{value, 4};
  EXPECT_NO_THROW(b.set_bit_position(0));
  EXPECT_NO_THROW(b.get_bytes(target, 1));
  EXPECT_EQ(0xf7, target[0]);

  std::memset(target, 0, 2);
  b = bit_reader_c{value, 4};
  EXPECT_NO_THROW(b.set_bit_position(0));
  EXPECT_NO_THROW(b.get_bytes(target, 2));
  EXPECT_EQ(0xf723, get_uint16_be(target));

  std::memset(target, 0, 2);
  b = bit_reader_c{value, 4};
  EXPECT_NO_THROW(b.set_bit_position(4));
  EXPECT_NO_THROW(b.get_bytes(target, 2));
  EXPECT_EQ(0x7234, get_uint16_be(target));

  std::memset(target, 0, 2);
  b = bit_reader_c{value, 4};
  EXPECT_NO_THROW(b.set_bit_position(24));
  EXPECT_NO_THROW(b.get_bytes(target, 1));
  EXPECT_EQ(0x81, target[0]);

  std::memset(target, 0, 2);
  b = bit_reader_c{value, 4};
  EXPECT_NO_THROW(b.set_bit_position(16));
  EXPECT_NO_THROW(b.get_bytes(target, 2));
  EXPECT_EQ(0x4a81, get_uint16_be(target));

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
