#include "common/common_pch.h"

#include "common/strings/formatting.h"

#include "gtest/gtest.h"

namespace {

TEST(StringsFormatting, FileSize) {
  EXPECT_EQ("1023 bytes", format_file_size(      1023ll));
  EXPECT_EQ(   "1.0 KiB", format_file_size(      1024ll));
  EXPECT_EQ(   "1.9 KiB", format_file_size(      2047ll));
  EXPECT_EQ(   "2.0 KiB", format_file_size(      2048ll));
  EXPECT_EQ("1023.9 KiB", format_file_size(   1048575ll));
  EXPECT_EQ(   "1.0 MiB", format_file_size(   1048576ll));
  EXPECT_EQ(   "1.9 MiB", format_file_size(   2097151ll));
  EXPECT_EQ(   "2.0 MiB", format_file_size(   2097152ll));
  EXPECT_EQ("1023.9 MiB", format_file_size(1073741823ll));
  EXPECT_EQ(   "1.0 GiB", format_file_size(1073741824ll));
  EXPECT_EQ(   "1.9 GiB", format_file_size(2147483647ll));
  EXPECT_EQ(   "2.0 GiB", format_file_size(2147483648ll));
}

}
