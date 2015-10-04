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

TEST(StringsFormatting, FormatTimecodeWithFormat) {
  auto value = 567890000ll + 1000000000ll * (34 + 2 * 60 + 1 * 3600);

  EXPECT_EQ("1",                  format_timecode(value, "%h"));
  EXPECT_EQ("01",                 format_timecode(value, "%H"));
  EXPECT_EQ("2",                  format_timecode(value, "%m"));
  EXPECT_EQ("02",                 format_timecode(value, "%M"));
  EXPECT_EQ("34",                 format_timecode(value, "%s"));
  EXPECT_EQ("34",                 format_timecode(value, "%S"));
  EXPECT_EQ("567890000",          format_timecode(value, "%n"));

  EXPECT_EQ("567890000",          format_timecode(value, "%9n"));
  EXPECT_EQ("56789000",           format_timecode(value, "%8n"));
  EXPECT_EQ("5678900",            format_timecode(value, "%7n"));
  EXPECT_EQ("567890",             format_timecode(value, "%6n"));
  EXPECT_EQ("56789",              format_timecode(value, "%5n"));
  EXPECT_EQ("5678",               format_timecode(value, "%4n"));
  EXPECT_EQ("567",                format_timecode(value, "%3n"));
  EXPECT_EQ("56",                 format_timecode(value, "%2n"));
  EXPECT_EQ("5",                  format_timecode(value, "%1n"));
  EXPECT_EQ("567890000",          format_timecode(value, "%0n"));

  EXPECT_EQ("%",                  format_timecode(value, "%%"));
  EXPECT_EQ("z",                  format_timecode(value, "z"));
  EXPECT_EQ("q",                  format_timecode(value, "%q"));

  EXPECT_EQ("01:02:34.567890000", format_timecode(value, "%H:%M:%S.%n"));
}

}
