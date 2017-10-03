#include "common/common_pch.h"

#include "common/byte_buffer.h"

#include "gtest/gtest.h"

namespace {

TEST(ByteBuffer, Add) {
  mtx::bytes::buffer_c b;

  b.add(reinterpret_cast<unsigned char const *>("Hello"), 5);

  ASSERT_EQ(5, b.get_size());

  b.add(reinterpret_cast<unsigned char const *>("world!"), 6);

  ASSERT_EQ(11, b.get_size());

  auto s = std::string{reinterpret_cast<char *>(b.get_buffer()), b.get_size()};

  ASSERT_EQ(std::string{"Helloworld!"}, s);
}

TEST(ByteBuffer, Remove) {
  mtx::bytes::buffer_c b;

  b.add(reinterpret_cast<unsigned char const *>("Hello world"), 11);

  ASSERT_EQ(11, b.get_size());

  b.remove(3);

  ASSERT_EQ(8, b.get_size());

  auto s = std::string{reinterpret_cast<char *>(b.get_buffer()), b.get_size()};

  ASSERT_EQ(std::string{"lo world"}, s);

  b.remove(2, mtx::bytes::buffer_c::at_back);

  ASSERT_EQ(6, b.get_size());

  s = std::string{reinterpret_cast<char *>(b.get_buffer()), b.get_size()};

  ASSERT_EQ(std::string{"lo wor"}, s);

  b.add(reinterpret_cast<unsigned char const *>("meow"), 4);

  ASSERT_EQ(10, b.get_size());

  s = std::string{reinterpret_cast<char *>(b.get_buffer()), b.get_size()};

  ASSERT_EQ(std::string{"lo wormeow"}, s);
}

TEST(ByteBuffer, Prepend) {
  mtx::bytes::buffer_c b;

  b.add(reinterpret_cast<unsigned char const *>("You cruel world"), 15);

  ASSERT_EQ(15, b.get_size());

  b.remove(9);

  ASSERT_EQ(6, b.get_size());

  b.prepend(reinterpret_cast<unsigned char const *>("Hello"), 5);

  ASSERT_EQ(11, b.get_size());

  auto s = std::string{reinterpret_cast<char *>(b.get_buffer()), b.get_size()};

  ASSERT_EQ(std::string{"Hello world"}, s);
}

}
