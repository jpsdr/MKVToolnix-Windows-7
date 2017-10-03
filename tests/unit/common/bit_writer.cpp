#include "common/common_pch.h"

#include "common/bit_reader.h"
#include "common/bit_writer.h"
#include "common/endian.h"

#include "gtest/gtest.h"

namespace {

// 0xf    7    2    3    4    a    8    1
//   1111 0111 0010 0011 0100 1010 1000 0001

TEST(BitWriter, PutBit) {
  auto b = mtx::bits::writer_c{};

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

  auto buffer = b.get_buffer();
  ASSERT_EQ(2, buffer->get_size());
  EXPECT_EQ(0xf723, get_uint16_be(buffer->get_buffer()));
}

TEST(BitWriter, PutBits) {
  auto b = mtx::bits::writer_c{};

  // f7
  EXPECT_NO_THROW(b.put_bits(4, 0x0f));
  EXPECT_NO_THROW(b.put_bits(4, 0x07));

  auto buffer = b.get_buffer();
  ASSERT_EQ(1, buffer->get_size());
  EXPECT_EQ(0xf7, buffer->get_buffer()[0]);

  // 234
  EXPECT_NO_THROW(b.put_bits(12, 0x234));
  buffer = b.get_buffer();
  ASSERT_EQ(3, buffer->get_size());
  EXPECT_EQ(0xf72340, get_uint24_be(buffer->get_buffer()));

  // a81 = 101 010 000…
  EXPECT_NO_THROW(b.put_bits(3, 0x05));
  EXPECT_NO_THROW(b.put_bits(3, 0x02));
  EXPECT_NO_THROW(b.put_bits(3, 0x00));

  EXPECT_EQ(29, b.get_bit_position());
  buffer = b.get_buffer();
  ASSERT_EQ(4, buffer->get_size());
  EXPECT_EQ(0xf7234a80, get_uint32_be(buffer->get_buffer()));
}

TEST(BitWriter, ByteAlign) {
  auto b = mtx::bits::writer_c{};

  // f7
  b.byte_align();
  EXPECT_EQ(0, b.get_bit_position());
  ASSERT_EQ(0, b.get_buffer()->get_size());

  EXPECT_NO_THROW(b.put_bits(2, 0x03));
  b.byte_align();
  auto buffer = b.get_buffer();
  EXPECT_EQ(8,    b.get_bit_position());
  ASSERT_EQ(1,    buffer->get_size());
  EXPECT_EQ(0xc0, buffer->get_buffer()[0]);

  EXPECT_NO_THROW(b.put_bits(7, 0x11));
  b.byte_align();
  buffer = b.get_buffer();
  EXPECT_EQ(        16, b.get_bit_position());
  ASSERT_EQ(2,      buffer->get_size());
  EXPECT_EQ(0xc022, get_uint16_be(buffer->get_buffer()));
}

TEST(BitWriter, SkipBits) {
  auto b = mtx::bits::writer_c{};

  EXPECT_EQ(0, b.get_bit_position());
  EXPECT_NO_THROW(b.skip_bits(3));
  EXPECT_EQ(3, b.get_bit_position());
  EXPECT_NO_THROW(b.skip_bits(7));
  EXPECT_EQ(10, b.get_bit_position());
  EXPECT_NO_THROW(b.skip_bit());
  EXPECT_EQ(11, b.get_bit_position());
  ASSERT_EQ(0,  b.get_buffer()->get_size());
}

TEST(BitWriter, SetBitPosition) {
  auto b = mtx::bits::writer_c{};

  EXPECT_EQ(0, b.get_bit_position());

  EXPECT_NO_THROW(b.set_bit_position(0));
  EXPECT_EQ(0, b.get_bit_position());

  EXPECT_NO_THROW(b.set_bit_position(8));
  EXPECT_EQ(8, b.get_bit_position());

  EXPECT_NO_THROW(b.set_bit_position(0));
  EXPECT_EQ(0, b.get_bit_position());

  EXPECT_NO_THROW(b.set_bit_position(31));
  EXPECT_EQ(31, b.get_bit_position());

  ASSERT_EQ(0, b.get_buffer()->get_size());

  EXPECT_NO_THROW(b.put_bit(1));
  auto buffer = b.get_buffer();
  ASSERT_EQ(4,          buffer->get_size());
  EXPECT_EQ(0x00000001, get_uint32_be(buffer->get_buffer()));
}

TEST(BitWriter, ExtendingTheBuffer) {
  auto b = mtx::bits::writer_c{};

  for (int idx = 0; idx < 24; ++idx)
    ASSERT_NO_THROW(b.put_bits(32, 0x01234567u));

  ASSERT_EQ(96 * 8, b.get_bit_position());

  ASSERT_NO_THROW(b.put_bits(64, 0x0123456789abcdefull));

  ASSERT_EQ(104 * 8, b.get_bit_position());

  auto buffer = b.get_buffer();

  ASSERT_EQ(104, buffer->get_size());
  EXPECT_EQ(0x01234567u, get_uint32_be(&buffer->get_buffer()[92]));
  EXPECT_EQ(0x01234567u, get_uint32_be(&buffer->get_buffer()[96]));
  EXPECT_EQ(0x89abcdefu, get_uint32_be(&buffer->get_buffer()[100]));
}

}
