#include "common/common_pch.h"

#include "common/checksums/base.h"
#include "common/mm_file_io.h"
#include "common/mm_proxy_io.h"
#include "common/mm_text_io.h"

#include "tests/unit/init.h"
#include "tests/unit/util.h"

namespace {

class ChecksumTest: public ::testing::Test {
public:
  memory_cptr m_data, m_data_md5, m_onetwothree_md5;
  std::string m_onetwothree;

  ChecksumTest()
    : m_data{mm_file_io_c::slurp("tests/unit/data/text/chapters-with-ebmlvoid.xml")}
    , m_onetwothree{"123456789"}
  {
    unsigned char data_md5_raw[16]        = { 0x43, 0x3a, 0x9a, 0x3e, 0x62, 0xe4, 0x93, 0xb5, 0x15, 0x9d, 0xf9, 0x1f, 0x58, 0x5b, 0x26, 0x91 };
    m_data_md5                            = memory_c::clone(data_md5_raw, 16);

    unsigned char onetwothree_md5_raw[16] = { 0x25, 0xf9, 0xe7, 0x94, 0x32, 0x3b, 0x45, 0x38, 0x85, 0xf5, 0x18, 0x1f, 0x1b, 0x62, 0x4d, 0x0b };
    m_onetwothree_md5                     = memory_c::clone(onetwothree_md5_raw, 16);
  }

  uint32_t
  calculate_int(mtx::checksum::algorithm_e algorithm,
                uint32_t initial_value,
                size_t chunk_size) {
    auto ptr       = m_data->get_buffer();
    auto remaining = m_data->get_size();
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

  memory_cptr
  calculate_bin(mtx::checksum::algorithm_e algorithm,
                size_t chunk_size) {
    auto ptr       = m_data->get_buffer();
    auto remaining = m_data->get_size();
    auto worker    = mtx::checksum::for_algorithm(algorithm);

    while (remaining) {
      auto to_handle = std::min<size_t>(remaining, chunk_size);

      worker->add(ptr, to_handle);

      ptr       += to_handle;
      remaining -= to_handle;
    }

    worker->finish();

    return worker->get_result();
  }
};

TEST_F(ChecksumTest, OneTwoThree) {
  auto ptr  = reinterpret_cast<unsigned char const *>(m_onetwothree.c_str());

  EXPECT_EQ(0xf4,       mtx::checksum::calculate_as_uint(mtx::checksum::algorithm_e::crc8_atm,      ptr, m_onetwothree.length(),          0));
  EXPECT_EQ(0xe8fe,     mtx::checksum::calculate_as_uint(mtx::checksum::algorithm_e::crc16_ansi,    ptr, m_onetwothree.length(),          0));
  EXPECT_EQ(0xc331,     mtx::checksum::calculate_as_uint(mtx::checksum::algorithm_e::crc16_ccitt,   ptr, m_onetwothree.length(),          0));
  EXPECT_EQ(0xe7e67603, mtx::checksum::calculate_as_uint(mtx::checksum::algorithm_e::crc32_ieee,    ptr, m_onetwothree.length(), 0xffffffff));
  EXPECT_EQ(0x340bc6d9, mtx::checksum::calculate_as_uint(mtx::checksum::algorithm_e::crc32_ieee_le, ptr, m_onetwothree.length(), 0xffffffff));

  EXPECT_EQ(*m_onetwothree_md5, *mtx::checksum::calculate(mtx::checksum::algorithm_e::md5, ptr, m_onetwothree.length()));
}

TEST_F(ChecksumTest, FileAllInOne) {
  EXPECT_EQ(0xab,         calculate_int(mtx::checksum::algorithm_e::crc8_atm,               0, m_data->get_size()));
  EXPECT_EQ(0x18fe,       calculate_int(mtx::checksum::algorithm_e::crc16_ansi,             0, m_data->get_size()));
  EXPECT_EQ(0x218f,       calculate_int(mtx::checksum::algorithm_e::crc16_ccitt,            0, m_data->get_size()));
  EXPECT_EQ(0x5a0a3951,   calculate_int(mtx::checksum::algorithm_e::crc32_ieee,    0xffffffff, m_data->get_size()));
  EXPECT_EQ(0x88c5b46f,   calculate_int(mtx::checksum::algorithm_e::crc32_ieee_le, 0xffffffff, m_data->get_size()));
  EXPECT_EQ(*m_data_md5, *calculate_bin(mtx::checksum::algorithm_e::md5,                       m_data->get_size()));
}

TEST_F(ChecksumTest, FileChunked37) {
  EXPECT_EQ(0xab,         calculate_int(mtx::checksum::algorithm_e::crc8_atm,               0, 37));
  EXPECT_EQ(0x18fe,       calculate_int(mtx::checksum::algorithm_e::crc16_ansi,             0, 37));
  EXPECT_EQ(0x218f,       calculate_int(mtx::checksum::algorithm_e::crc16_ccitt,            0, 37));
  EXPECT_EQ(0x5a0a3951,   calculate_int(mtx::checksum::algorithm_e::crc32_ieee,    0xffffffff, 37));
  EXPECT_EQ(0x88c5b46f,   calculate_int(mtx::checksum::algorithm_e::crc32_ieee_le, 0xffffffff, 37));
  EXPECT_EQ(*m_data_md5, *calculate_bin(mtx::checksum::algorithm_e::md5,                       37));
}

TEST_F(ChecksumTest, FileChunked63) {
  EXPECT_EQ(0xab,         calculate_int(mtx::checksum::algorithm_e::crc8_atm,               0, 63));
  EXPECT_EQ(0x18fe,       calculate_int(mtx::checksum::algorithm_e::crc16_ansi,             0, 63));
  EXPECT_EQ(0x218f,       calculate_int(mtx::checksum::algorithm_e::crc16_ccitt,            0, 63));
  EXPECT_EQ(0x5a0a3951,   calculate_int(mtx::checksum::algorithm_e::crc32_ieee,    0xffffffff, 63));
  EXPECT_EQ(0x88c5b46f,   calculate_int(mtx::checksum::algorithm_e::crc32_ieee_le, 0xffffffff, 63));
  EXPECT_EQ(*m_data_md5, *calculate_bin(mtx::checksum::algorithm_e::md5,                       63));
}

TEST_F(ChecksumTest, FileChunked64) {
  EXPECT_EQ(0xab,         calculate_int(mtx::checksum::algorithm_e::crc8_atm,               0, 64));
  EXPECT_EQ(0x18fe,       calculate_int(mtx::checksum::algorithm_e::crc16_ansi,             0, 64));
  EXPECT_EQ(0x218f,       calculate_int(mtx::checksum::algorithm_e::crc16_ccitt,            0, 64));
  EXPECT_EQ(0x5a0a3951,   calculate_int(mtx::checksum::algorithm_e::crc32_ieee,    0xffffffff, 64));
  EXPECT_EQ(0x88c5b46f,   calculate_int(mtx::checksum::algorithm_e::crc32_ieee_le, 0xffffffff, 64));
  EXPECT_EQ(*m_data_md5, *calculate_bin(mtx::checksum::algorithm_e::md5,                       64));
}

TEST_F(ChecksumTest, FileChunked65) {
  EXPECT_EQ(0xab,         calculate_int(mtx::checksum::algorithm_e::crc8_atm,               0, 65));
  EXPECT_EQ(0x18fe,       calculate_int(mtx::checksum::algorithm_e::crc16_ansi,             0, 65));
  EXPECT_EQ(0x218f,       calculate_int(mtx::checksum::algorithm_e::crc16_ccitt,            0, 65));
  EXPECT_EQ(0x5a0a3951,   calculate_int(mtx::checksum::algorithm_e::crc32_ieee,    0xffffffff, 65));
  EXPECT_EQ(0x88c5b46f,   calculate_int(mtx::checksum::algorithm_e::crc32_ieee_le, 0xffffffff, 65));
  EXPECT_EQ(*m_data_md5, *calculate_bin(mtx::checksum::algorithm_e::md5,                       65));
}

TEST_F(ChecksumTest, FileChunked1000) {
  EXPECT_EQ(0xab,         calculate_int(mtx::checksum::algorithm_e::crc8_atm,               0, 1000));
  EXPECT_EQ(0x18fe,       calculate_int(mtx::checksum::algorithm_e::crc16_ansi,             0, 1000));
  EXPECT_EQ(0x218f,       calculate_int(mtx::checksum::algorithm_e::crc16_ccitt,            0, 1000));
  EXPECT_EQ(0x5a0a3951,   calculate_int(mtx::checksum::algorithm_e::crc32_ieee,    0xffffffff, 1000));
  EXPECT_EQ(0x88c5b46f,   calculate_int(mtx::checksum::algorithm_e::crc32_ieee_le, 0xffffffff, 1000));
  EXPECT_EQ(*m_data_md5, *calculate_bin(mtx::checksum::algorithm_e::md5,                       1000));
}

}
