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

TEST(StringsFormatting, FormatTimecodeWithPrecision) {
  auto value = 567891234ll + 1000000000ll * (34 + 2 * 60 + 1 * 3600);

  EXPECT_EQ("01:02:34.567891234", format_timecode(value));
  EXPECT_EQ("01:02:34.567891234", format_timecode(value, 10));
  EXPECT_EQ("01:02:34.567891234", format_timecode(value, 9));
  EXPECT_EQ("01:02:34.56789123",  format_timecode(value, 8));
  EXPECT_EQ("01:02:34.5678912",   format_timecode(value, 7));
  EXPECT_EQ("01:02:34.567891",    format_timecode(value, 6));
  EXPECT_EQ("01:02:34.56789",     format_timecode(value, 5));
  EXPECT_EQ("01:02:34.5679",      format_timecode(value, 4));
  EXPECT_EQ("01:02:34.568",       format_timecode(value, 3));
  EXPECT_EQ("01:02:34.57",        format_timecode(value, 2));
  EXPECT_EQ("01:02:34.6",         format_timecode(value, 1));
  EXPECT_EQ("01:02:35",           format_timecode(value, 0));
}

}
