#include "common/common_pch.h"

#include "merge/packet.h"
#include "merge/timecode_calculator.h"

#include "gtest/gtest.h"

namespace {

TEST(TimecodeCalculator, NoTimecodesProvided) {
  auto calc = timecode_calculator_c{48000ll};

  ASSERT_EQ(timecode_c::s(0), calc.get_next_timecode(0));
  ASSERT_EQ(timecode_c::s(0), calc.get_next_timecode(48000));
  ASSERT_EQ(timecode_c::ms(1000), calc.get_next_timecode(48000));
  ASSERT_EQ(timecode_c::ms(2000), calc.get_next_timecode(24000));
  ASSERT_EQ(timecode_c::ms(2500), calc.get_next_timecode(12345));
  ASSERT_EQ(timecode_c::ns(2757187500), calc.get_next_timecode(96000));
  ASSERT_EQ(timecode_c::ns(4757187500), calc.get_next_timecode(96000));
}

TEST(TimecodeCalculator, WithTimecodesProvidedByTimecodeC) {
  auto calc = timecode_calculator_c{48000ll};

  calc.add_timecode(timecode_c::s(10));
  ASSERT_EQ(timecode_c::s(10), calc.get_next_timecode(0));
  ASSERT_EQ(timecode_c::s(10), calc.get_next_timecode(48000));
  ASSERT_EQ(timecode_c::ms(11000), calc.get_next_timecode(48000));
  ASSERT_EQ(timecode_c::ms(12000), calc.get_next_timecode(24000));
  ASSERT_EQ(timecode_c::ms(12500), calc.get_next_timecode(12345));
  ASSERT_EQ(timecode_c::ns(12757187500), calc.get_next_timecode(96000));
  ASSERT_EQ(timecode_c::ns(14757187500), calc.get_next_timecode(96000));

  calc.add_timecode(timecode_c::s(20));
  calc.add_timecode(timecode_c::s(21));
  calc.add_timecode(timecode_c::s(22));
  ASSERT_EQ(timecode_c::s(20), calc.get_next_timecode(96000));
  ASSERT_EQ(timecode_c::s(21), calc.get_next_timecode(96000));
  ASSERT_EQ(timecode_c::s(22), calc.get_next_timecode(96000));
  ASSERT_EQ(timecode_c::s(24), calc.get_next_timecode(96000));
}

TEST(TimecodeCalculator, WithTimecodesProvidedByUInt) {
  auto calc = timecode_calculator_c{48000ll};

  calc.add_timecode(10000000000);
  ASSERT_EQ(timecode_c::s(10), calc.get_next_timecode(0));
  ASSERT_EQ(timecode_c::s(10), calc.get_next_timecode(48000));
  ASSERT_EQ(timecode_c::ms(11000), calc.get_next_timecode(48000));
  ASSERT_EQ(timecode_c::ms(12000), calc.get_next_timecode(24000));
  ASSERT_EQ(timecode_c::ms(12500), calc.get_next_timecode(12345));
  ASSERT_EQ(timecode_c::ns(12757187500), calc.get_next_timecode(96000));
  ASSERT_EQ(timecode_c::ns(14757187500), calc.get_next_timecode(96000));

  calc.add_timecode(20000000000);
  calc.add_timecode(21000000000);
  calc.add_timecode(22000000000);
  ASSERT_EQ(timecode_c::s(20), calc.get_next_timecode(96000));
  ASSERT_EQ(timecode_c::s(21), calc.get_next_timecode(96000));
  ASSERT_EQ(timecode_c::s(22), calc.get_next_timecode(96000));
  ASSERT_EQ(timecode_c::s(24), calc.get_next_timecode(96000));
}

TEST(TimecodeCalculator, WithTimecodesProvidedByPacket) {
  auto calc = timecode_calculator_c{48000ll};

  auto packet = std::make_shared<packet_t>();
  packet->timecode = 10000000000;
  calc.add_timecode(packet);
  ASSERT_EQ(timecode_c::s(10), calc.get_next_timecode(0));
  ASSERT_EQ(timecode_c::s(10), calc.get_next_timecode(48000));
  ASSERT_EQ(timecode_c::ms(11000), calc.get_next_timecode(48000));
  ASSERT_EQ(timecode_c::ms(12000), calc.get_next_timecode(24000));
  ASSERT_EQ(timecode_c::ms(12500), calc.get_next_timecode(12345));
  ASSERT_EQ(timecode_c::ns(12757187500), calc.get_next_timecode(96000));
  ASSERT_EQ(timecode_c::ns(14757187500), calc.get_next_timecode(96000));

  packet->timecode = 20000000000;
  calc.add_timecode(packet);
  packet->timecode = 21000000000;
  calc.add_timecode(packet);
  packet->timecode = 22000000000;
  calc.add_timecode(packet);
  ASSERT_EQ(timecode_c::s(20), calc.get_next_timecode(96000));
  ASSERT_EQ(timecode_c::s(21), calc.get_next_timecode(96000));
  ASSERT_EQ(timecode_c::s(22), calc.get_next_timecode(96000));
  ASSERT_EQ(timecode_c::s(24), calc.get_next_timecode(96000));
}

TEST(TimecodeCalculator, WithInvalidTimecodesProvided) {
  auto calc = timecode_calculator_c{48000ll};

  auto packet = std::make_shared<packet_t>();
  packet->timecode = -1;
  calc.add_timecode(timecode_c{});
  calc.add_timecode(-1);
  calc.add_timecode(packet);

  calc.add_timecode(timecode_c::s(5));

  ASSERT_EQ(timecode_c::ms(5000), calc.get_next_timecode(72000));
  ASSERT_EQ(timecode_c::ms(6500), calc.get_next_timecode(72000));
}

TEST(TimecodeCalculator, GetDuration) {
  auto calc = timecode_calculator_c{48000ll};

  EXPECT_EQ(timecode_c::s(1),                  calc.get_duration(48000));
  EXPECT_EQ(timecode_c::ns(2572016437500ll),  calc.get_duration(123456789));
  EXPECT_EQ(timecode_c::ns(36000000000000ll), calc.get_duration(1728000000));
}

TEST(TimecodeCalculator, InvalidSamplesPerSecond) {
  auto calc = timecode_calculator_c{0ll};
  ASSERT_THROW(calc.get_next_timecode(0), std::invalid_argument);
  ASSERT_THROW(calc.get_duration(0), std::invalid_argument);
}

TEST(TimecodeCalculator, SetSamplesPerSecond) {
  auto calc = timecode_calculator_c{48000ll};

  calc.add_timecode(timecode_c::s(1));
  ASSERT_EQ(timecode_c::s(1), calc.get_next_timecode(12345));
  ASSERT_EQ(timecode_c::ns(1257187500), calc.get_next_timecode(12345));

  calc.set_samples_per_second(98765);

  ASSERT_EQ(timecode_c::ns(1514375000), calc.get_next_timecode(192837465));
  ASSERT_EQ(timecode_c::ns(1954002250259), calc.get_next_timecode(0));
}

}
