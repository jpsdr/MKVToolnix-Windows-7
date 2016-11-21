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

  EXPECT_TRUE(parse_duration_number_with_unit("12345us", value));
  EXPECT_EQ(12345000ll, value);

  EXPECT_TRUE(parse_duration_number_with_unit("12345ms", value));
  EXPECT_EQ(12345000000ll, value);

  EXPECT_TRUE(parse_duration_number_with_unit("12345msec", value));
  EXPECT_EQ(12345000000ll, value);

  EXPECT_TRUE(parse_duration_number_with_unit("12345s", value));
  EXPECT_EQ(12345000000000ll, value);
}

TEST(StringsParsing, ParseDurationNumberWithUnitSecondUnitsFloats) {
  int64_t value;

  EXPECT_TRUE(parse_duration_number_with_unit("12345.678ns", value));
  EXPECT_EQ(12345ll, value);

  EXPECT_TRUE(parse_duration_number_with_unit("12345.678us", value));
  EXPECT_EQ(12345678ll, value);

  EXPECT_TRUE(parse_duration_number_with_unit("12345.678ms", value));
  EXPECT_EQ(12345678000ll, value);

  EXPECT_TRUE(parse_duration_number_with_unit("12345.678msec", value));
  EXPECT_EQ(12345678000ll, value);

  EXPECT_TRUE(parse_duration_number_with_unit("12345.678s", value));
  EXPECT_EQ(12345678000000ll, value);
}

TEST(StringsParsing, ParseDurationNumberWithUnitSecondUnitsFractions) {
  int64_t value;

  EXPECT_TRUE(parse_duration_number_with_unit("2500/50ns", value));
  EXPECT_EQ(50ll, value);

  EXPECT_TRUE(parse_duration_number_with_unit("2500/50us", value));
  EXPECT_EQ(50000ll, value);

  EXPECT_TRUE(parse_duration_number_with_unit("2500/50ms", value));
  EXPECT_EQ(50000000ll, value);

  EXPECT_TRUE(parse_duration_number_with_unit("2500/50msec", value));
  EXPECT_EQ(50000000ll, value);

  EXPECT_TRUE(parse_duration_number_with_unit("2500/50ns", value));
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

}
