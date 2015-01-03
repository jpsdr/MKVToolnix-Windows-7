#include "common/common_pch.h"

#include "gtest/gtest.h"

#include "common/checksums/base.h"
#include "common/mm_io.h"

namespace {

uint32_t
calculate(mtx::checksum::algorithm_e algorithm,
          memory_c const &mem,
          uint32_t initial_value,
          size_t chunk_size) {
  auto ptr       = mem.get_buffer();
  auto remaining = mem.get_size();
  auto worker    = mtx::checksum::for_algorithm(algorithm, initial_value);

  while (remaining) {
    auto to_handle = std::min<size_t>(remaining, chunk_size);

    worker->add(ptr, to_handle);

    ptr       += to_handle;
    remaining -= to_handle;
  }

  worker->finish();

  return dynamic_cast<mtx::checksum::uint_result_c &>(*worker).get_result_as_uint();
}

TEST(ChecksumTest, OneTwoThree) {
  auto data = std::string{"123456789"};
  auto ptr  = reinterpret_cast<unsigned char const *>(data.c_str());

  EXPECT_EQ(0xf4,       mtx::checksum::calculate_as_uint(mtx::checksum::crc8_atm,      ptr, data.length(),          0));
  EXPECT_EQ(0xe8fe,     mtx::checksum::calculate_as_uint(mtx::checksum::crc16_ansi,    ptr, data.length(),          0));
  EXPECT_EQ(0xc331,     mtx::checksum::calculate_as_uint(mtx::checksum::crc16_ccitt,   ptr, data.length(),          0));
  EXPECT_EQ(0xe7e67603, mtx::checksum::calculate_as_uint(mtx::checksum::crc32_ieee,    ptr, data.length(), 0xffffffff));
  EXPECT_EQ(0x340bc6d9, mtx::checksum::calculate_as_uint(mtx::checksum::crc32_ieee_le, ptr, data.length(), 0xffffffff));
}

TEST(ChecksumTest, FileAllInOne) {
  auto data = mm_file_io_c::slurp("tests/unit/data/text/chapters-with-ebmlvoid.xml");

  EXPECT_EQ(0xab,       calculate(mtx::checksum::crc8_atm,      *data,          0, data->get_size()));
  EXPECT_EQ(0x18fe,     calculate(mtx::checksum::crc16_ansi,    *data,          0, data->get_size()));
  EXPECT_EQ(0x218f,     calculate(mtx::checksum::crc16_ccitt,   *data,          0, data->get_size()));
  EXPECT_EQ(0x5a0a3951, calculate(mtx::checksum::crc32_ieee,    *data, 0xffffffff, data->get_size()));
  EXPECT_EQ(0x88c5b46f, calculate(mtx::checksum::crc32_ieee_le, *data, 0xffffffff, data->get_size()));
}

TEST(ChecksumTest, FileChunked37) {
  auto data = mm_file_io_c::slurp("tests/unit/data/text/chapters-with-ebmlvoid.xml");

  EXPECT_EQ(0xab,       calculate(mtx::checksum::crc8_atm,      *data,          0, 37));
  EXPECT_EQ(0x18fe,     calculate(mtx::checksum::crc16_ansi,    *data,          0, 37));
  EXPECT_EQ(0x218f,     calculate(mtx::checksum::crc16_ccitt,   *data,          0, 37));
  EXPECT_EQ(0x5a0a3951, calculate(mtx::checksum::crc32_ieee,    *data, 0xffffffff, 37));
  EXPECT_EQ(0x88c5b46f, calculate(mtx::checksum::crc32_ieee_le, *data, 0xffffffff, 37));
}

TEST(ChecksumTest, FileChunked63) {
  auto data = mm_file_io_c::slurp("tests/unit/data/text/chapters-with-ebmlvoid.xml");

  EXPECT_EQ(0xab,       calculate(mtx::checksum::crc8_atm,      *data,          0, 63));
  EXPECT_EQ(0x18fe,     calculate(mtx::checksum::crc16_ansi,    *data,          0, 63));
  EXPECT_EQ(0x218f,     calculate(mtx::checksum::crc16_ccitt,   *data,          0, 63));
  EXPECT_EQ(0x5a0a3951, calculate(mtx::checksum::crc32_ieee,    *data, 0xffffffff, 63));
  EXPECT_EQ(0x88c5b46f, calculate(mtx::checksum::crc32_ieee_le, *data, 0xffffffff, 63));
}

TEST(ChecksumTest, FileChunked64) {
  auto data = mm_file_io_c::slurp("tests/unit/data/text/chapters-with-ebmlvoid.xml");

  EXPECT_EQ(0xab,       calculate(mtx::checksum::crc8_atm,      *data,          0, 64));
  EXPECT_EQ(0x18fe,     calculate(mtx::checksum::crc16_ansi,    *data,          0, 64));
  EXPECT_EQ(0x218f,     calculate(mtx::checksum::crc16_ccitt,   *data,          0, 64));
  EXPECT_EQ(0x5a0a3951, calculate(mtx::checksum::crc32_ieee,    *data, 0xffffffff, 64));
  EXPECT_EQ(0x88c5b46f, calculate(mtx::checksum::crc32_ieee_le, *data, 0xffffffff, 64));
}

TEST(ChecksumTest, FileChunked65) {
  auto data = mm_file_io_c::slurp("tests/unit/data/text/chapters-with-ebmlvoid.xml");

  EXPECT_EQ(0xab,       calculate(mtx::checksum::crc8_atm,      *data,          0, 65));
  EXPECT_EQ(0x18fe,     calculate(mtx::checksum::crc16_ansi,    *data,          0, 65));
  EXPECT_EQ(0x218f,     calculate(mtx::checksum::crc16_ccitt,   *data,          0, 65));
  EXPECT_EQ(0x5a0a3951, calculate(mtx::checksum::crc32_ieee,    *data, 0xffffffff, 65));
  EXPECT_EQ(0x88c5b46f, calculate(mtx::checksum::crc32_ieee_le, *data, 0xffffffff, 65));
}

TEST(ChecksumTest, FileChunked1000) {
  auto data = mm_file_io_c::slurp("tests/unit/data/text/chapters-with-ebmlvoid.xml");

  EXPECT_EQ(0xab,       calculate(mtx::checksum::crc8_atm,      *data,          0, 1000));
  EXPECT_EQ(0x18fe,     calculate(mtx::checksum::crc16_ansi,    *data,          0, 1000));
  EXPECT_EQ(0x218f,     calculate(mtx::checksum::crc16_ccitt,   *data,          0, 1000));
  EXPECT_EQ(0x5a0a3951, calculate(mtx::checksum::crc32_ieee,    *data, 0xffffffff, 1000));
  EXPECT_EQ(0x88c5b46f, calculate(mtx::checksum::crc32_ieee_le, *data, 0xffffffff, 1000));
}

}
