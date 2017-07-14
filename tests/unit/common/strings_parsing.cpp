#include "common/common_pch.h"

#include "common/math.h"
#include "common/strings/editing.h"
#include "common/strings/parsing.h"

#include "gtest/gtest.h"

namespace {

TEST(StringsParsing, ParseDurationNumberWithUnitSecondUnitsIntegers) {
  int64_t value;

  EXPECT_TRUE(parse_duration_number_with_unit("12345ns", value));
  EXPECT_EQ(12345ll, value);

  EXPECT_TRUE(parse_duration_number_with_unit("12345nsec", value));
  EXPECT_EQ(12345ll, value);

  EXPECT_TRUE(parse_duration_number_with_unit("12345us", value));
  EXPECT_EQ(12345000ll, value);

  EXPECT_TRUE(parse_duration_number_with_unit("12345µs", value));
  EXPECT_EQ(12345000ll, value);

  EXPECT_TRUE(parse_duration_number_with_unit("12345ms", value));
  EXPECT_EQ(12345000000ll, value);

  EXPECT_TRUE(parse_duration_number_with_unit("12345msec", value));
  EXPECT_EQ(12345000000ll, value);

  EXPECT_TRUE(parse_duration_number_with_unit("12345s", value));
  EXPECT_EQ(12345000000000ll, value);

  EXPECT_TRUE(parse_duration_number_with_unit("123m", value));
  EXPECT_EQ(7380000000000ll, value);

  EXPECT_TRUE(parse_duration_number_with_unit("123min", value));
  EXPECT_EQ(7380000000000ll, value);

  EXPECT_TRUE(parse_duration_number_with_unit("12h", value));
  EXPECT_EQ(43200000000000ll, value);
}

TEST(StringsParsing, ParseDurationNumberWithUnitSecondUnitsFloats) {
  int64_t value;

  EXPECT_TRUE(parse_duration_number_with_unit("12345.678ns", value));
  EXPECT_EQ(12345ll, value);

  EXPECT_TRUE(parse_duration_number_with_unit("12345.678nsec", value));
  EXPECT_EQ(12345ll, value);

  EXPECT_TRUE(parse_duration_number_with_unit("12345.678us", value));
  EXPECT_EQ(12345678ll, value);

  EXPECT_TRUE(parse_duration_number_with_unit("12345.678µs", value));
  EXPECT_EQ(12345678ll, value);

  EXPECT_TRUE(parse_duration_number_with_unit("12345.678ms", value));
  EXPECT_EQ(12345678000ll, value);

  EXPECT_TRUE(parse_duration_number_with_unit("12345.678msec", value));
  EXPECT_EQ(12345678000ll, value);

  EXPECT_TRUE(parse_duration_number_with_unit("12345.678s", value));
  EXPECT_EQ(12345678000000ll, value);

  EXPECT_TRUE(parse_duration_number_with_unit("12345.678m", value));
  EXPECT_EQ(740740680000000, value);

  EXPECT_TRUE(parse_duration_number_with_unit("12345.678min", value));
  EXPECT_EQ(740740680000000, value);

  EXPECT_TRUE(parse_duration_number_with_unit("12345.678h", value));
  EXPECT_EQ(44444440800000000ll, value);
}

TEST(StringsParsing, ParseDurationNumberWithUnitSecondUnitsFractions) {
  int64_t value;

  EXPECT_TRUE(parse_duration_number_with_unit("2500/50ns", value));
  EXPECT_EQ(50ll, value);

  EXPECT_TRUE(parse_duration_number_with_unit("2500/50nsec", value));
  EXPECT_EQ(50ll, value);

  EXPECT_TRUE(parse_duration_number_with_unit("2500/50us", value));
  EXPECT_EQ(50000ll, value);

  EXPECT_TRUE(parse_duration_number_with_unit("2500/50µs", value));
  EXPECT_EQ(50000ll, value);

  EXPECT_TRUE(parse_duration_number_with_unit("2500/50ms", value));
  EXPECT_EQ(50000000ll, value);

  EXPECT_TRUE(parse_duration_number_with_unit("2500/50msec", value));
  EXPECT_EQ(50000000ll, value);

  EXPECT_TRUE(parse_duration_number_with_unit("2500/50ns", value));
  EXPECT_EQ(50, value);

  EXPECT_TRUE(parse_duration_number_with_unit("2500/50nsec", value));
  EXPECT_EQ(50, value);
}

TEST(StringsParsing, ParseDurationNumberWithUnitFrameUnitsIntegers) {
  int64_t value;

  EXPECT_TRUE(parse_duration_number_with_unit("20fps", value));
  EXPECT_EQ(1000000000ll / 20, value);

  EXPECT_TRUE(parse_duration_number_with_unit("20p", value));
  EXPECT_EQ(1000000000ll / 20, value);

  EXPECT_TRUE(parse_duration_number_with_unit("20i", value));
  EXPECT_EQ(1000000000ll / 10, value);
}

TEST(StringsParsing, ParseDurationNumberWithUnitFrameUnitsSpecialValues) {
  int64_t value;

  EXPECT_TRUE(parse_duration_number_with_unit("23.96fps", value));
  EXPECT_EQ(1000000000ll * 1001 / 24000, value);

  EXPECT_TRUE(parse_duration_number_with_unit("29.976fps", value));
  EXPECT_EQ(1000000000ll * 1001 / 30000, value);

  EXPECT_TRUE(parse_duration_number_with_unit("59.94fps", value));
  EXPECT_EQ(1000000000ll * 1001 / 60000, value);
}

TEST(StringsParsing, ParseDurationNumberWithUnitInvalid) {
  int64_t value;

  EXPECT_FALSE(parse_duration_number_with_unit("", value));
  EXPECT_FALSE(parse_duration_number_with_unit("20", value));
  EXPECT_FALSE(parse_duration_number_with_unit("fps", value));
  EXPECT_FALSE(parse_duration_number_with_unit("i", value));
  EXPECT_FALSE(parse_duration_number_with_unit("i20", value));
  EXPECT_FALSE(parse_duration_number_with_unit("20/s", value));
}

TEST(StringsParsing, ParseNumberToRationalInvalidPatterns) {
  int64_rational_c r;

  EXPECT_FALSE(parse_number("",        r));
  EXPECT_FALSE(parse_number("bad",     r));
  EXPECT_FALSE(parse_number("123.bad", r));
}

TEST(StringsParsing, ParseNumberToRationalValidPatterns) {
  int64_rational_c r;

  EXPECT_TRUE(parse_number("0", r));
  EXPECT_EQ(int64_rational_c(0ll, 1ll), r);

  EXPECT_TRUE(parse_number("0.0", r));
  EXPECT_EQ(int64_rational_c(0ll, 1ll), r);

  EXPECT_TRUE(parse_number("1", r));
  EXPECT_EQ(int64_rational_c(1ll, 1ll), r);

  EXPECT_TRUE(parse_number("1.", r));
  EXPECT_EQ(int64_rational_c(1ll, 1ll), r);

  EXPECT_TRUE(parse_number("1.0", r));
  EXPECT_EQ(int64_rational_c(1ll, 1ll), r);

  EXPECT_TRUE(parse_number("123456.789", r));
  EXPECT_EQ(int64_rational_c(123456789ll, 1000ll), r);

  EXPECT_TRUE(parse_number("123456.789", r));
  EXPECT_EQ(int64_rational_c(123456789ll, 1000ll), r);

  EXPECT_TRUE(parse_number("123456.789012345", r));
  EXPECT_EQ(int64_rational_c(123456789012345ll, 1000000000ll), r);
}

TEST(StringParsing, ParseTimecodeValidPatternsNumberWithUnit) {
  int64_t timestamp;

  EXPECT_TRUE(parse_timestamp("123h", timestamp, true));
  EXPECT_EQ(442800000000000ll, timestamp);

  EXPECT_TRUE(parse_timestamp("123m", timestamp, true));
  EXPECT_EQ(7380000000000ll, timestamp);

  EXPECT_TRUE(parse_timestamp("123min", timestamp, true));
  EXPECT_EQ(7380000000000ll, timestamp);

  EXPECT_TRUE(parse_timestamp("123ms", timestamp, true));
  EXPECT_EQ(123000000ll, timestamp);

  EXPECT_TRUE(parse_timestamp("123msec", timestamp, true));
  EXPECT_EQ(123000000ll, timestamp);

  EXPECT_TRUE(parse_timestamp("123us", timestamp, true));
  EXPECT_EQ(123000ll, timestamp);

  EXPECT_TRUE(parse_timestamp("123µs", timestamp, true));
  EXPECT_EQ(123000ll, timestamp);

  EXPECT_TRUE(parse_timestamp("123ns", timestamp, true));
  EXPECT_EQ(123, timestamp);

  EXPECT_TRUE(parse_timestamp("123nsec", timestamp, true));
  EXPECT_EQ(123, timestamp);
}

TEST(StringParsing, ParseTimestampValidPatternsNumberWithUnitNegative) {
  int64_t timestamp;

  EXPECT_TRUE(parse_timestamp("-123h", timestamp, true));
  EXPECT_EQ(-442800000000000ll, timestamp);

  EXPECT_TRUE(parse_timestamp("-123m", timestamp, true));
  EXPECT_EQ(-7380000000000ll, timestamp);

  EXPECT_TRUE(parse_timestamp("-123min", timestamp, true));
  EXPECT_EQ(-7380000000000ll, timestamp);

  EXPECT_TRUE(parse_timestamp("-123s", timestamp, true));
  EXPECT_EQ(-123000000000ll, timestamp);

  EXPECT_TRUE(parse_timestamp("-123ms", timestamp, true));
  EXPECT_EQ(-123000000ll, timestamp);

  EXPECT_TRUE(parse_timestamp("-123msec", timestamp, true));
  EXPECT_EQ(-123000000ll, timestamp);

  EXPECT_TRUE(parse_timestamp("-123us", timestamp, true));
  EXPECT_EQ(-123000ll, timestamp);

  EXPECT_TRUE(parse_timestamp("-123µs", timestamp, true));
  EXPECT_EQ(-123000ll, timestamp);

  EXPECT_TRUE(parse_timestamp("-123ns", timestamp, true));
  EXPECT_EQ(-123, timestamp);

  EXPECT_TRUE(parse_timestamp("-123ns", timestamp, true));
  EXPECT_EQ(-123, timestamp);

  EXPECT_TRUE(parse_timestamp("-123nsec", timestamp, true));
  EXPECT_EQ(-123, timestamp);
}

TEST(StringParsing, ParseTimestampValidPatternsHMSns) {
  int64_t timestamp;

  EXPECT_TRUE(parse_timestamp("12:34:56.789123456", timestamp, true));
  EXPECT_EQ(45296789123456ll, timestamp);

  EXPECT_TRUE(parse_timestamp("2:34:56.789123456", timestamp, true));
  EXPECT_EQ(9296789123456ll, timestamp);

  EXPECT_TRUE(parse_timestamp("34:56.789123456", timestamp, true));
  EXPECT_EQ(2096789123456ll, timestamp);

  EXPECT_TRUE(parse_timestamp("4:56.789123456", timestamp, true));
  EXPECT_EQ(296789123456ll, timestamp);

  EXPECT_TRUE(parse_timestamp("12:34:56.78912345", timestamp, true));
  EXPECT_EQ(45296789123450ll, timestamp);

  EXPECT_TRUE(parse_timestamp("12:34:56.7891234", timestamp, true));
  EXPECT_EQ(45296789123400ll, timestamp);

  EXPECT_TRUE(parse_timestamp("12:34:56.789123", timestamp, true));
  EXPECT_EQ(45296789123000ll, timestamp);

  EXPECT_TRUE(parse_timestamp("12:34:56.78912", timestamp, true));
  EXPECT_EQ(45296789120000ll, timestamp);

  EXPECT_TRUE(parse_timestamp("12:34:56.7891", timestamp, true));
  EXPECT_EQ(45296789100000ll, timestamp);

  EXPECT_TRUE(parse_timestamp("12:34:56.789", timestamp, true));
  EXPECT_EQ(45296789000000ll, timestamp);

  EXPECT_TRUE(parse_timestamp("12:34:56.78", timestamp, true));
  EXPECT_EQ(45296780000000ll, timestamp);

  EXPECT_TRUE(parse_timestamp("12:34:56.7", timestamp, true));
  EXPECT_EQ(45296700000000ll, timestamp);

  EXPECT_TRUE(parse_timestamp("12:34:56", timestamp, true));
  EXPECT_EQ(45296000000000ll, timestamp);

  EXPECT_TRUE(parse_timestamp("2:34:56", timestamp, true));
  EXPECT_EQ(9296000000000ll, timestamp);

  EXPECT_TRUE(parse_timestamp("34:56", timestamp, true));
  EXPECT_EQ(2096000000000ll, timestamp);

  EXPECT_TRUE(parse_timestamp("4:56", timestamp, true));
  EXPECT_EQ(296000000000ll, timestamp);
}

TEST(StringParsing, ParseTimestampValidPatternsHMSnsNegative) {
  int64_t timestamp;

  EXPECT_TRUE(parse_timestamp("-12:34:56.789123456", timestamp, true));
  EXPECT_EQ(-45296789123456ll, timestamp);

  EXPECT_TRUE(parse_timestamp("-2:34:56.789123456", timestamp, true));
  EXPECT_EQ(-9296789123456ll, timestamp);

  EXPECT_TRUE(parse_timestamp("-34:56.789123456", timestamp, true));
  EXPECT_EQ(-2096789123456ll, timestamp);

  EXPECT_TRUE(parse_timestamp("-4:56.789123456", timestamp, true));
  EXPECT_EQ(-296789123456ll, timestamp);

  EXPECT_TRUE(parse_timestamp("-12:34:56.78912345", timestamp, true));
  EXPECT_EQ(-45296789123450ll, timestamp);

  EXPECT_TRUE(parse_timestamp("-12:34:56.7891234", timestamp, true));
  EXPECT_EQ(-45296789123400ll, timestamp);

  EXPECT_TRUE(parse_timestamp("-12:34:56.789123", timestamp, true));
  EXPECT_EQ(-45296789123000ll, timestamp);

  EXPECT_TRUE(parse_timestamp("-12:34:56.78912", timestamp, true));
  EXPECT_EQ(-45296789120000ll, timestamp);

  EXPECT_TRUE(parse_timestamp("-12:34:56.7891", timestamp, true));
  EXPECT_EQ(-45296789100000ll, timestamp);

  EXPECT_TRUE(parse_timestamp("-12:34:56.789", timestamp, true));
  EXPECT_EQ(-45296789000000ll, timestamp);

  EXPECT_TRUE(parse_timestamp("-12:34:56.78", timestamp, true));
  EXPECT_EQ(-45296780000000ll, timestamp);

  EXPECT_TRUE(parse_timestamp("-12:34:56.7", timestamp, true));
  EXPECT_EQ(-45296700000000ll, timestamp);

  EXPECT_TRUE(parse_timestamp("-12:34:56", timestamp, true));
  EXPECT_EQ(-45296000000000ll, timestamp);

  EXPECT_TRUE(parse_timestamp("-2:34:56", timestamp, true));
  EXPECT_EQ(-9296000000000ll, timestamp);

  EXPECT_TRUE(parse_timestamp("-34:56", timestamp, true));
  EXPECT_EQ(-2096000000000ll, timestamp);

  EXPECT_TRUE(parse_timestamp("-4:56", timestamp, true));
  EXPECT_EQ(-296000000000ll, timestamp);
}

TEST(StringParsing, ParseTimestampInvalidPatterns) {
  int64_t timestamp;

  EXPECT_FALSE(parse_timestamp("12:34:56.789123456us", timestamp, true));  // HMS: unit after
  EXPECT_FALSE(parse_timestamp("12:34:56.789123456qq", timestamp, true));  // HMS: garbage after
  EXPECT_FALSE(parse_timestamp("12::56.789123456",     timestamp, true));  // HMS: empty minutes
  EXPECT_FALSE(parse_timestamp("56.789123456",         timestamp, true));  // HMS: no hours & minutes
  EXPECT_FALSE(parse_timestamp("qq56.789123456",       timestamp, true));  // HMS: garbage before
  EXPECT_FALSE(parse_timestamp("-12:34:56.789123456",  timestamp, false)); // HMS: negative but not allowed

  EXPECT_FALSE(parse_timestamp("-123s",                timestamp, false)); // number+unit: negative but not allowed
  EXPECT_FALSE(parse_timestamp("123",                  timestamp, false)); // number+unit: no unit
  EXPECT_FALSE(parse_timestamp("123q",                 timestamp, false)); // number+unit: invalid unit
  EXPECT_FALSE(parse_timestamp("123s q",               timestamp, false)); // number+unit: garbage after
  EXPECT_FALSE(parse_timestamp("q123s",                timestamp, false)); // number+unit: garbage before
}

}
