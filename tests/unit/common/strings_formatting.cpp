#include "common/common_pch.h"

#include "common/strings/formatting.h"

#include "tests/unit/init.h"

namespace {

TEST(StringsFormatting, FileSize) {
  EXPECT_EQ(  "123 bytes", mtx::string::format_file_size(       123ll));
  EXPECT_EQ("1,023 bytes", mtx::string::format_file_size(      1023ll));
  EXPECT_EQ(    "1.0 KiB", mtx::string::format_file_size(      1024ll));
  EXPECT_EQ(    "1.9 KiB", mtx::string::format_file_size(      2047ll));
  EXPECT_EQ(    "2.0 KiB", mtx::string::format_file_size(      2048ll));
  EXPECT_EQ("1,023.9 KiB", mtx::string::format_file_size(   1048575ll));
  EXPECT_EQ(    "1.0 MiB", mtx::string::format_file_size(   1048576ll));
  EXPECT_EQ(    "1.9 MiB", mtx::string::format_file_size(   2097151ll));
  EXPECT_EQ(    "2.0 MiB", mtx::string::format_file_size(   2097152ll));
  EXPECT_EQ("1,023.9 MiB", mtx::string::format_file_size(1073741823ll));
  EXPECT_EQ(    "1.0 GiB", mtx::string::format_file_size(1073741824ll));
  EXPECT_EQ(    "1.9 GiB", mtx::string::format_file_size(2147483647ll));
  EXPECT_EQ(    "2.0 GiB", mtx::string::format_file_size(2147483648ll));

  EXPECT_EQ("1,023 bytes",                   mtx::string::format_file_size(      1023ll, mtx::string::file_size_format_e::full));
  EXPECT_EQ("2.0 GiB (2,147,483,648 bytes)", mtx::string::format_file_size(2147483648ll, mtx::string::file_size_format_e::full));
}

TEST(StringsFormatting, FormatTimecodeWithPrecision) {
  auto value = 567891234ll + 1000000000ll * (34 + 2 * 60 + 1 * 3600);

  EXPECT_EQ("01:02:34.567891234", mtx::string::format_timestamp(value));
  EXPECT_EQ("01:02:34.567891234", mtx::string::format_timestamp(value, 10));
  EXPECT_EQ("01:02:34.567891234", mtx::string::format_timestamp(value, 9));
  EXPECT_EQ("01:02:34.56789123",  mtx::string::format_timestamp(value, 8));
  EXPECT_EQ("01:02:34.5678912",   mtx::string::format_timestamp(value, 7));
  EXPECT_EQ("01:02:34.567891",    mtx::string::format_timestamp(value, 6));
  EXPECT_EQ("01:02:34.56789",     mtx::string::format_timestamp(value, 5));
  EXPECT_EQ("01:02:34.5679",      mtx::string::format_timestamp(value, 4));
  EXPECT_EQ("01:02:34.568",       mtx::string::format_timestamp(value, 3));
  EXPECT_EQ("01:02:34.57",        mtx::string::format_timestamp(value, 2));
  EXPECT_EQ("01:02:34.6",         mtx::string::format_timestamp(value, 1));
  EXPECT_EQ("01:02:35",           mtx::string::format_timestamp(value, 0));
}

TEST(StringsFormatting, FormatTimecodeWithFormat) {
  auto value = 567890000ll + 1000000000ll * (34 + 2 * 60 + 1 * 3600);

  EXPECT_EQ("1",                  mtx::string::format_timestamp(value, "%h"));
  EXPECT_EQ("01",                 mtx::string::format_timestamp(value, "%H"));
  EXPECT_EQ("2",                  mtx::string::format_timestamp(value, "%m"));
  EXPECT_EQ("02",                 mtx::string::format_timestamp(value, "%M"));
  EXPECT_EQ("34",                 mtx::string::format_timestamp(value, "%s"));
  EXPECT_EQ("34",                 mtx::string::format_timestamp(value, "%S"));
  EXPECT_EQ("567890000",          mtx::string::format_timestamp(value, "%n"));

  EXPECT_EQ("567890000",          mtx::string::format_timestamp(value, "%9n"));
  EXPECT_EQ("56789000",           mtx::string::format_timestamp(value, "%8n"));
  EXPECT_EQ("5678900",            mtx::string::format_timestamp(value, "%7n"));
  EXPECT_EQ("567890",             mtx::string::format_timestamp(value, "%6n"));
  EXPECT_EQ("56789",              mtx::string::format_timestamp(value, "%5n"));
  EXPECT_EQ("5678",               mtx::string::format_timestamp(value, "%4n"));
  EXPECT_EQ("567",                mtx::string::format_timestamp(value, "%3n"));
  EXPECT_EQ("56",                 mtx::string::format_timestamp(value, "%2n"));
  EXPECT_EQ("5",                  mtx::string::format_timestamp(value, "%1n"));
  EXPECT_EQ("567890000",          mtx::string::format_timestamp(value, "%0n"));

  EXPECT_EQ("%",                  mtx::string::format_timestamp(value, "%%"));
  EXPECT_EQ("z",                  mtx::string::format_timestamp(value, "z"));
  EXPECT_EQ("q",                  mtx::string::format_timestamp(value, "%q"));

  EXPECT_EQ("01:02:34.567890000", mtx::string::format_timestamp(value, "%H:%M:%S.%n"));
}

TEST(StringFormatting, FormatUnsignedInteger) {
  EXPECT_EQ("0",             mtx::string::format_number(static_cast<uint64_t>(0ull)));
  EXPECT_EQ("12",            mtx::string::format_number(static_cast<uint64_t>(12ull)));
  EXPECT_EQ("123",           mtx::string::format_number(static_cast<uint64_t>(123ull)));
  EXPECT_EQ("1.234",         mtx::string::format_number(static_cast<uint64_t>(1234ull)));
  EXPECT_EQ("1.234.567.890", mtx::string::format_number(static_cast<uint64_t>(1234567890ull)));
}

TEST(StringFormatting, FormatPositiveSignedInteger) {
  EXPECT_EQ("0",             mtx::string::format_number(static_cast<int64_t>(0ll)));
  EXPECT_EQ("12",            mtx::string::format_number(static_cast<int64_t>(12ll)));
  EXPECT_EQ("123",           mtx::string::format_number(static_cast<int64_t>(123ll)));
  EXPECT_EQ("1.234",         mtx::string::format_number(static_cast<int64_t>(1234ll)));
  EXPECT_EQ("1.234.567.890", mtx::string::format_number(static_cast<int64_t>(1234567890ll)));
}

TEST(StringFormatting, FormatNegativeSignedInteger) {
  EXPECT_EQ("0",              mtx::string::format_number(static_cast<int64_t>(-0ll)));
  EXPECT_EQ("-12",            mtx::string::format_number(static_cast<int64_t>(-12ll)));
  EXPECT_EQ("-123",           mtx::string::format_number(static_cast<int64_t>(-123ll)));
  EXPECT_EQ("-1.234",         mtx::string::format_number(static_cast<int64_t>(-1234ll)));
  EXPECT_EQ("-1.234.567.890", mtx::string::format_number(static_cast<int64_t>(-1234567890ll)));
}

}
