#include "common/common_pch.h"

#include "common/bit_reader.h"
#include "common/bit_writer.h"
#include "common/endian.h"

#include "gtest/gtest.h"

namespace {

// 0xf    7    2    3    4    a    8    1
//   1111 0111 0010 0011 0100 1010 1000 0001

TEST(BitWriter, PutBit) {
  unsigned char value[4];
  std::memset(value, 0, 4);
  auto b = bit_writer_c{value, 4};

  // f
  EXPECT_NO_THROW(b.put_bit(1));
  EXPECT_NO_THROW(b.put_bit(1));
  EXPECT_NO_THROW(b.put_bit(1));
  EXPECT_NO_THROW(b.put_bit(1));
  // 7
  EXPECT_NO_THROW(b.put_bit(0));
  EXPECT_NO_THROW(b.put_bit(1));
  EXPECT_NO_THROW(b.put_bit(1));
  EXPECT_NO_THROW(b.put_bit(1));
  // 2
  EXPECT_NO_THROW(b.put_bit(0));
  EXPECT_NO_THROW(b.put_bit(0));
  EXPECT_NO_THROW(b.put_bit(1));
  EXPECT_NO_THROW(b.put_bit(0));
  // 3
  EXPECT_NO_THROW(b.put_bit(0));
  EXPECT_NO_THROW(b.put_bit(0));
  EXPECT_NO_THROW(b.put_bit(1));
  EXPECT_NO_THROW(b.put_bit(1));

  EXPECT_EQ(16, b.get_bit_position());
  EXPECT_EQ(16, b.get_remaining_bits());
  EXPECT_FALSE(b.eof());

  EXPECT_EQ(0xf723, get_uint16_be(&value[0]));
  EXPECT_EQ(0x0000, get_uint16_be(&value[2]));
}

TEST(BitWriter, PutBits) {
  unsigned char value[4];
  std::memset(value, 0, 4);
  auto b = bit_writer_c{value, 4};

  // f7
  EXPECT_NO_THROW(b.put_bits(4, 0x0f));
  EXPECT_NO_THROW(b.put_bits(4, 0x07));
  EXPECT_EQ(0xf7,     value[0]);
  EXPECT_EQ(0x000000, get_uint24_be(&value[1]));

  // 234
  EXPECT_NO_THROW(b.put_bits(12, 0x234));
  EXPECT_EQ(0xf72340, get_uint24_be(&value[0]));
  EXPECT_EQ(0x00,     value[3]);

  // a81 = 101 010 000…
  EXPECT_NO_THROW(b.put_bits(3, 0x05));
  EXPECT_NO_THROW(b.put_bits(3, 0x02));
  EXPECT_NO_THROW(b.put_bits(3, 0x00));

  EXPECT_EQ(29, b.get_bit_position());
  EXPECT_EQ( 3, b.get_remaining_bits());
  EXPECT_FALSE(b.eof());
  EXPECT_EQ(0xf7234a80, get_uint32_be(&value[0]));
}

TEST(BitWriter, ByteAlign) {
  unsigned char value[4];
  std::memset(value, 0, 4);
  auto b = bit_writer_c{value, 4};

  // f7
  b.byte_align();
  EXPECT_EQ(         0, b.get_bit_position());
  EXPECT_EQ(0x00000000, get_uint32_be(value));

  EXPECT_NO_THROW(b.put_bits(2, 0x03));
  b.byte_align();
  EXPECT_EQ(         8, b.get_bit_position());
  EXPECT_EQ(0xc0000000, get_uint32_be(value));

  EXPECT_NO_THROW(b.put_bits(7, 0x11));
  b.byte_align();
  EXPECT_EQ(        16, b.get_bit_position());
  EXPECT_EQ(0xc0220000, get_uint32_be(value));
}

TEST(BitWriter, SkipBits) {
  unsigned char value[4];
  std::memset(value, 0, 4);
  auto b = bit_writer_c{value, 4};

  EXPECT_EQ(0, b.get_bit_position());
  EXPECT_NO_THROW(b.skip_bits(3));
  EXPECT_EQ(3, b.get_bit_position());
  EXPECT_NO_THROW(b.skip_bits(7));
  EXPECT_EQ(10, b.get_bit_position());
  EXPECT_NO_THROW(b.skip_bit());
  EXPECT_EQ(11, b.get_bit_position());
}


TEST(BitWriter, SetBitPosition) {
  unsigned char value[4];
  std::memset(value, 0, 4);
  auto b = bit_writer_c{value, 4};

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
  EXPECT_THROW(b.set_bit_position(33), mtx::mm_io::seek_x);
}

TEST(BitWriter, ExceptionOnReadingBeyondEOF) {
  unsigned char value[4];
  std::memset(value, 0, 4);
  auto b = bit_writer_c{value, 4};

  EXPECT_NO_THROW(b.set_bit_position(31));
  EXPECT_NO_THROW(b.put_bit(1));
  EXPECT_THROW(b.put_bit(1), mtx::mm_io::end_of_file_x);

  b = bit_writer_c{value, 4};
  EXPECT_NO_THROW(b.set_bit_position(24));
  EXPECT_NO_THROW(b.put_bits(8, 0x81));
  EXPECT_THROW(b.put_bit(1), mtx::mm_io::end_of_file_x);
}

}
