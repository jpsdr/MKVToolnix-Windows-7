#include "common/common_pch.h"

#include "merge/packet.h"
#include "merge/timestamp_calculator.h"

#include "gtest/gtest.h"

namespace {

TEST(TimestampCalculator, NoTimestampsProvided) {
  auto calc = timestamp_calculator_c{48000ll};

  ASSERT_EQ(timestamp_c::s(0), calc.get_next_timestamp(0));
  ASSERT_EQ(timestamp_c::s(0), calc.get_next_timestamp(48000));
  ASSERT_EQ(timestamp_c::ms(1000), calc.get_next_timestamp(48000));
  ASSERT_EQ(timestamp_c::ms(2000), calc.get_next_timestamp(24000));
  ASSERT_EQ(timestamp_c::ms(2500), calc.get_next_timestamp(12345));
  ASSERT_EQ(timestamp_c::ns(2757187500), calc.get_next_timestamp(96000));
  ASSERT_EQ(timestamp_c::ns(4757187500), calc.get_next_timestamp(96000));
}

TEST(TimestampCalculator, WithTimestampsProvidedByTimestampC) {
  auto calc = timestamp_calculator_c{48000ll};

  calc.add_timestamp(timestamp_c::s(10));
  ASSERT_EQ(timestamp_c::s(10), calc.get_next_timestamp(0));
  ASSERT_EQ(timestamp_c::s(10), calc.get_next_timestamp(48000));
  ASSERT_EQ(timestamp_c::ms(11000), calc.get_next_timestamp(48000));
  ASSERT_EQ(timestamp_c::ms(12000), calc.get_next_timestamp(24000));
  ASSERT_EQ(timestamp_c::ms(12500), calc.get_next_timestamp(12345));
  ASSERT_EQ(timestamp_c::ns(12757187500), calc.get_next_timestamp(96000));
  ASSERT_EQ(timestamp_c::ns(14757187500), calc.get_next_timestamp(96000));

  calc.add_timestamp(timestamp_c::s(20));
  calc.add_timestamp(timestamp_c::s(21));
  calc.add_timestamp(timestamp_c::s(22));
  ASSERT_EQ(timestamp_c::s(20), calc.get_next_timestamp(96000));
  ASSERT_EQ(timestamp_c::s(21), calc.get_next_timestamp(96000));
  ASSERT_EQ(timestamp_c::s(22), calc.get_next_timestamp(96000));
  ASSERT_EQ(timestamp_c::s(24), calc.get_next_timestamp(96000));
}

TEST(TimestampCalculator, WithTimestampsProvidedByUInt) {
  auto calc = timestamp_calculator_c{48000ll};

  calc.add_timestamp(10000000000);
  ASSERT_EQ(timestamp_c::s(10), calc.get_next_timestamp(0));
  ASSERT_EQ(timestamp_c::s(10), calc.get_next_timestamp(48000));
  ASSERT_EQ(timestamp_c::ms(11000), calc.get_next_timestamp(48000));
  ASSERT_EQ(timestamp_c::ms(12000), calc.get_next_timestamp(24000));
  ASSERT_EQ(timestamp_c::ms(12500), calc.get_next_timestamp(12345));
  ASSERT_EQ(timestamp_c::ns(12757187500), calc.get_next_timestamp(96000));
  ASSERT_EQ(timestamp_c::ns(14757187500), calc.get_next_timestamp(96000));

  calc.add_timestamp(20000000000);
  calc.add_timestamp(21000000000);
  calc.add_timestamp(22000000000);
  ASSERT_EQ(timestamp_c::s(20), calc.get_next_timestamp(96000));
  ASSERT_EQ(timestamp_c::s(21), calc.get_next_timestamp(96000));
  ASSERT_EQ(timestamp_c::s(22), calc.get_next_timestamp(96000));
  ASSERT_EQ(timestamp_c::s(24), calc.get_next_timestamp(96000));
}

TEST(TimestampCalculator, WithTimestampsProvidedByPacket) {
  auto calc = timestamp_calculator_c{48000ll};

  auto packet = std::make_shared<packet_t>();
  packet->timestamp = 10000000000;
  calc.add_timestamp(packet);
  ASSERT_EQ(timestamp_c::s(10), calc.get_next_timestamp(0));
  ASSERT_EQ(timestamp_c::s(10), calc.get_next_timestamp(48000));
  ASSERT_EQ(timestamp_c::ms(11000), calc.get_next_timestamp(48000));
  ASSERT_EQ(timestamp_c::ms(12000), calc.get_next_timestamp(24000));
  ASSERT_EQ(timestamp_c::ms(12500), calc.get_next_timestamp(12345));
  ASSERT_EQ(timestamp_c::ns(12757187500), calc.get_next_timestamp(96000));
  ASSERT_EQ(timestamp_c::ns(14757187500), calc.get_next_timestamp(96000));

  packet->timestamp = 20000000000;
  calc.add_timestamp(packet);
  packet->timestamp = 21000000000;
  calc.add_timestamp(packet);
  packet->timestamp = 22000000000;
  calc.add_timestamp(packet);
  ASSERT_EQ(timestamp_c::s(20), calc.get_next_timestamp(96000));
  ASSERT_EQ(timestamp_c::s(21), calc.get_next_timestamp(96000));
  ASSERT_EQ(timestamp_c::s(22), calc.get_next_timestamp(96000));
  ASSERT_EQ(timestamp_c::s(24), calc.get_next_timestamp(96000));
}

TEST(TimestampCalculator, WithInvalidTimestampsProvided) {
  auto calc = timestamp_calculator_c{48000ll};

  auto packet = std::make_shared<packet_t>();
  packet->timestamp = -1;
  calc.add_timestamp(timestamp_c{});
  calc.add_timestamp(-1);
  calc.add_timestamp(packet);

  calc.add_timestamp(timestamp_c::s(5));

  ASSERT_EQ(timestamp_c::ms(5000), calc.get_next_timestamp(72000));
  ASSERT_EQ(timestamp_c::ms(6500), calc.get_next_timestamp(72000));
}

TEST(TimestampCalculator, GetDuration) {
  auto calc = timestamp_calculator_c{48000ll};

  EXPECT_EQ(timestamp_c::s(1),                 calc.get_duration(48000));
  EXPECT_EQ(timestamp_c::ns(2572016437500ll),  calc.get_duration(123456789));
  EXPECT_EQ(timestamp_c::ns(36000000000000ll), calc.get_duration(1728000000));
}

TEST(TimestampCalculator, InvalidSamplesPerSecond) {
  auto calc = timestamp_calculator_c{0ll};
  ASSERT_THROW(calc.get_next_timestamp(0), std::invalid_argument);
  ASSERT_THROW(calc.get_duration(0), std::invalid_argument);
}

TEST(TimestampCalculator, SetSamplesPerSecond) {
  auto calc = timestamp_calculator_c{48000ll};

  calc.add_timestamp(timestamp_c::s(1));
  ASSERT_EQ(timestamp_c::s(1), calc.get_next_timestamp(12345));
  ASSERT_EQ(timestamp_c::ns(1257187500), calc.get_next_timestamp(12345));

  calc.set_samples_per_second(98765);

  ASSERT_EQ(timestamp_c::ns(1514375000), calc.get_next_timestamp(192837465));
  ASSERT_EQ(timestamp_c::ns(1954002250259), calc.get_next_timestamp(0));
}

TEST(TimestampCalculator, AddingSameAndSmallerTimestamps) {
  auto calc = timestamp_calculator_c{48000ll};

  calc.add_timestamp(timestamp_c::s(10));
  calc.add_timestamp(timestamp_c::s(10));
  calc.add_timestamp(timestamp_c::s(5));
  calc.add_timestamp(timestamp_c::s(20));
  ASSERT_EQ(timestamp_c::s(10), calc.get_next_timestamp(48000));
  ASSERT_EQ(timestamp_c::s(20), calc.get_next_timestamp(48000));
  ASSERT_EQ(timestamp_c::s(21), calc.get_next_timestamp(48000));

  calc.add_timestamp(timestamp_c::s(20));
  calc.add_timestamp(timestamp_c::s(30));
  calc.add_timestamp(timestamp_c::s(40));
  ASSERT_EQ(timestamp_c::s(30), calc.get_next_timestamp(48000));
  ASSERT_EQ(timestamp_c::s(40), calc.get_next_timestamp(48000));
  ASSERT_EQ(timestamp_c::s(41), calc.get_next_timestamp(48000));
}

}
