#include "common/common_pch.h"

#include "common/timestamp.h"

#include "gtest/gtest.h"

namespace {

TEST(BasicTimecode, Creation) {
  EXPECT_EQ(            1ll, timestamp_c::factor(1).to_ns());
  EXPECT_EQ(            1ll, timestamp_c::ns(1).to_ns());
  EXPECT_EQ(         1000ll, timestamp_c::us(1).to_ns());
  EXPECT_EQ(      1000000ll, timestamp_c::ms(1).to_ns());
  EXPECT_EQ(   1000000000ll, timestamp_c::s(1).to_ns());
  EXPECT_EQ(  60000000000ll, timestamp_c::m(1).to_ns());
  EXPECT_EQ(3600000000000ll, timestamp_c::h(1).to_ns());
  EXPECT_EQ(        11111ll, timestamp_c::mpeg(1).to_ns());
}

TEST(BasicTimecode, Deconstruction) {
  EXPECT_EQ(7204003002001ll, timestamp_c::ns(7204003002001ll).to_ns());
  EXPECT_EQ(   7204003002ll, timestamp_c::ns(7204003002001ll).to_us());
  EXPECT_EQ(      7204003ll, timestamp_c::ns(7204003002001ll).to_ms());
  EXPECT_EQ(         7204ll, timestamp_c::ns(7204003002001ll).to_s());
  EXPECT_EQ(          120ll, timestamp_c::ns(7204003002001ll).to_m());
  EXPECT_EQ(            2ll, timestamp_c::ns(7204003002001ll).to_h());
  EXPECT_EQ(    648360270ll, timestamp_c::ns(7204003002001ll).to_mpeg());
}

TEST(BasicTimecode, ArithmeticBothValid) {
  EXPECT_TRUE((timestamp_c::s(2)        + timestamp_c::us(500000)).valid());
  EXPECT_TRUE((timestamp_c::s(2)        - timestamp_c::us(500000)).valid());
  EXPECT_TRUE((timestamp_c::us(1250000) * timestamp_c::factor(2)).valid());
  EXPECT_TRUE((timestamp_c::us(9900000) / timestamp_c::factor(3)).valid());

  EXPECT_TRUE(timestamp_c::s(-3).abs().valid());
}

TEST(BasicTimecode, ArithmenticResults) {
  EXPECT_EQ(timestamp_c::ms(2500), timestamp_c::s(2)        + timestamp_c::us(500000));
  EXPECT_EQ(timestamp_c::ms(1500), timestamp_c::s(2)        - timestamp_c::us(500000));
  EXPECT_EQ(timestamp_c::ms(2500), timestamp_c::us(1250000) * timestamp_c::factor(2));
  EXPECT_EQ(timestamp_c::ms(3300), timestamp_c::us(9900000) / timestamp_c::factor(3));

  EXPECT_EQ(timestamp_c::s(-3).abs(), timestamp_c::s(3));
  EXPECT_EQ(timestamp_c::s(-3).abs(), timestamp_c::s(3).abs());

  EXPECT_EQ(timestamp_c::s(-3).negate(), timestamp_c::s(+3));
  EXPECT_EQ(timestamp_c::s(+3).negate(), timestamp_c::s(-3));
}

TEST(BasicTimecode, ArithmenticLHSInvalid) {
  EXPECT_FALSE((timestamp_c{} + timestamp_c::m(2)).valid());
  EXPECT_FALSE((timestamp_c{} - timestamp_c::m(3)).valid());
  EXPECT_FALSE((timestamp_c{} * timestamp_c::factor(2)).valid());
  EXPECT_FALSE((timestamp_c{} / timestamp_c::factor(4)).valid());
}

TEST(BasicTimecode, ArithmenticRHSInvalid) {
  EXPECT_FALSE((timestamp_c::m(2)      + timestamp_c{}).valid());
  EXPECT_FALSE((timestamp_c::m(3)      - timestamp_c{}).valid());
  EXPECT_FALSE((timestamp_c::factor(2) * timestamp_c{}).valid());
  EXPECT_FALSE((timestamp_c::factor(4) / timestamp_c{}).valid());
}

TEST(BasicTimecode, ArithmenticBothInvalid) {
  EXPECT_FALSE((timestamp_c{} + timestamp_c{}).valid());
  EXPECT_FALSE((timestamp_c{} - timestamp_c{}).valid());
  EXPECT_FALSE((timestamp_c{} * timestamp_c{}).valid());
  EXPECT_FALSE((timestamp_c{} / timestamp_c{}).valid());
}

TEST(BasicTimecode, ComparisonBothValid) {
  EXPECT_TRUE( timestamp_c::ms(2500) <  timestamp_c::s(3));
  EXPECT_TRUE( timestamp_c::ms(2500) <= timestamp_c::s(3));
  EXPECT_FALSE(timestamp_c::ms(2500) >  timestamp_c::s(3));
  EXPECT_FALSE(timestamp_c::ms(2500) >= timestamp_c::s(3));
  EXPECT_TRUE( timestamp_c::ms(2500) != timestamp_c::s(3));
  EXPECT_FALSE(timestamp_c::ms(2500) == timestamp_c::s(3));

  EXPECT_FALSE(timestamp_c::ms(3000) <  timestamp_c::s(3));
  EXPECT_TRUE( timestamp_c::ms(3000) <= timestamp_c::s(3));
  EXPECT_FALSE(timestamp_c::ms(3000) >  timestamp_c::s(3));
  EXPECT_TRUE( timestamp_c::ms(3000) >= timestamp_c::s(3));
  EXPECT_FALSE(timestamp_c::ms(3000) != timestamp_c::s(3));
  EXPECT_TRUE( timestamp_c::ms(3000) == timestamp_c::s(3));

  EXPECT_FALSE(timestamp_c::ms(4000) <= timestamp_c::s(3));
  EXPECT_TRUE( timestamp_c::ms(4000) >  timestamp_c::s(3));
}

TEST(BasicTimecode, ComparisonLHSInvalid) {
  EXPECT_TRUE( timestamp_c{} <  timestamp_c::ns(3));
  EXPECT_FALSE(timestamp_c{} == timestamp_c::ns(3));
}

TEST(BasicTimecode, ComparisonRHSInvalid) {
  EXPECT_FALSE(timestamp_c::ns(3) <  timestamp_c{});
  EXPECT_FALSE(timestamp_c::ns(3) == timestamp_c{});
}

TEST(BasicTimecode, ComparisonBothInvalid) {
  EXPECT_FALSE(timestamp_c{} <  timestamp_c{});
  EXPECT_TRUE( timestamp_c{} == timestamp_c{});
}

TEST(BasicTimecode, ThrowOnDeconstructionOfInvalid) {
  EXPECT_THROW(timestamp_c{}.to_ns(),   std::domain_error);
  EXPECT_THROW(timestamp_c{}.to_us(),   std::domain_error);
  EXPECT_THROW(timestamp_c{}.to_ms(),   std::domain_error);
  EXPECT_THROW(timestamp_c{}.to_s(),    std::domain_error);
  EXPECT_THROW(timestamp_c{}.to_m(),    std::domain_error);
  EXPECT_THROW(timestamp_c{}.to_h(),    std::domain_error);
  EXPECT_THROW(timestamp_c{}.to_mpeg(), std::domain_error);

  EXPECT_NO_THROW(timestamp_c{}.to_ns(1));
}

TEST(BasicTimecode, ConstructFromSamples) {
  EXPECT_EQ(         0, timestamp_c::samples(     0, 48000).to_ns());
  EXPECT_EQ( 416645833, timestamp_c::samples( 19999, 48000).to_ns());
  EXPECT_EQ( 416666667, timestamp_c::samples( 20000, 48000).to_ns());
  EXPECT_EQ(1000000000, timestamp_c::samples( 48000, 48000).to_ns());
  EXPECT_EQ(2572000000, timestamp_c::samples(123456, 48000).to_ns());

  EXPECT_THROW(timestamp_c::samples(123, 0), std::domain_error);
}

TEST(BasicTimecode, DeconstructToSamples) {
  EXPECT_EQ(     0, timestamp_c::ns(         0).to_samples(48000));
  EXPECT_EQ( 19999, timestamp_c::ns( 416645833).to_samples(48000));
  EXPECT_EQ( 20000, timestamp_c::ns( 416666667).to_samples(48000));
  EXPECT_EQ( 48000, timestamp_c::ns(1000000000).to_samples(48000));
  EXPECT_EQ(123456, timestamp_c::ns(2572000000).to_samples(48000));

  EXPECT_THROW(timestamp_c::ns(123).to_samples(0), std::domain_error);
}

TEST(BasicTimecode, Resetting) {
  auto v = timestamp_c::ns(1);

  EXPECT_TRUE(v.valid());
  EXPECT_NO_THROW(v.to_ns());

  v.reset();
  EXPECT_FALSE(v.valid());
  EXPECT_THROW(v.to_ns(), std::domain_error);
}

TEST(BasicTimecode, MinMax) {
  EXPECT_TRUE(timestamp_c::min()                 <  timestamp_c::max());
  EXPECT_TRUE(timestamp_c::min()                 == timestamp_c::ns(std::numeric_limits<int64_t>::min()));
  EXPECT_TRUE(timestamp_c::max()                 == timestamp_c::ns(std::numeric_limits<int64_t>::max()));
  EXPECT_TRUE(timestamp_c{}.value_or_min()       == timestamp_c::min());
  EXPECT_TRUE(timestamp_c{}.value_or_max()       == timestamp_c::max());
  EXPECT_TRUE(timestamp_c{}.value_or_zero()      == timestamp_c::ns(0));
  EXPECT_TRUE(timestamp_c::ns(1).value_or_min()  == timestamp_c::ns(1));
  EXPECT_TRUE(timestamp_c::ns(1).value_or_max()  == timestamp_c::ns(1));
  EXPECT_TRUE(timestamp_c::ns(1).value_or_zero() == timestamp_c::ns(1));
}

}
