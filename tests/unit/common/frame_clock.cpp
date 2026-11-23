#include "common/common_pch.h"

#include "common/frame_clock.h"

#include "tests/unit/init.h"

namespace {

// The exact timestamp of `units` units of num / den seconds, rounded to the
// nearest ns (half away from zero).
int64_t
exact(int64_t units,
      int64_t num,
      int64_t den) {
  return mtx::to_int_rounded(mtx::rational(units * num, den) * 1'000'000'000);
}

TEST(FrameClock, StartsAtZero) {
  mtx::frame_clock_c clock;

  EXPECT_FALSE(clock.has_unit());
  EXPECT_EQ(0, clock.now());

  clock.set_unit(1001, 48000);
  EXPECT_TRUE(clock.has_unit());
  EXPECT_EQ(0, clock.now());
}

TEST(FrameClock, AdvanceReturnsTheStart) {
  mtx::frame_clock_c clock;
  clock.set_unit(1001, 48000);                   // one field at 23.976

  EXPECT_EQ(0,        clock.advance(2));
  EXPECT_EQ(41708333, clock.advance(2));         // 41,708,333.33
  EXPECT_EQ(83416667, clock.advance(3));         // 83,416,666.67
  EXPECT_EQ(exact(7, 1001, 48000), clock.now());
}

TEST(FrameClock, PeekDoesNotMove) {
  mtx::frame_clock_c clock;
  clock.set_unit(1001, 60000);                   // one field at 29.97

  clock.advance(2);
  EXPECT_EQ(exact(5, 1001, 60000), clock.peek(3));
  EXPECT_EQ(exact(2, 1001, 60000), clock.now());
}

TEST(FrameClock, NoDriftOverAMillionFrames) {
  mtx::frame_clock_c clock;
  clock.set_unit(1001, 48000);

  for (auto frame = 0; frame < 1'000'000; ++frame)
    clock.advance(2);

  // 41,708.33 s exactly; cut field lengths added up: 41,708,332,000,000
  EXPECT_EQ(41'708'333'333'333, clock.now());
}

TEST(FrameClock, EveryFrameOnTheExactTime) {
  struct rate_t { int64_t num, den; };
  // field lengths: 23.976, 24, 25, 29.97, 30, 50, 59.94, 60 frames per second
  std::vector<rate_t> const rates{ { 1001, 48000 }, { 1, 48 }, { 1, 50 }, { 1001, 60000 }, { 1, 60 }, { 1, 100 }, { 1001, 120000 }, { 1, 120 } };

  for (auto const &rate : rates) {
    mtx::frame_clock_c clock;
    clock.set_unit(rate.num, rate.den);

    auto units = int64_t{};
    for (auto frame = 0; frame < 200'000; ++frame) {
      auto fields = frame % 2 ? 3 : 2;           // 2:3 pulldown
      ASSERT_EQ(exact(units, rate.num, rate.den), clock.advance(fields)) << rate.num << "/" << rate.den << " frame " << frame;
      units += fields;
    }
  }
}

TEST(FrameClock, ResetRestartsTheCount) {
  mtx::frame_clock_c clock;
  clock.set_unit(1001, 60000);

  clock.advance(10);
  clock.reset(1'000'000'000);

  EXPECT_EQ(1'000'000'000,                         clock.advance(2));
  EXPECT_EQ(1'000'000'000 + exact(2, 1001, 60000), clock.now());
}

TEST(FrameClock, NewUnitRestartsAtTheCurrentTime) {
  mtx::frame_clock_c clock;
  clock.set_unit(1001, 48000);

  clock.advance(4);                              // 83,416,666.67 -> 83,416,667
  clock.set_unit(1, 50);

  EXPECT_EQ(83'416'667,              clock.advance(2));
  EXPECT_EQ(83'416'667 + 40'000'000, clock.now());

  clock.set_unit(1, 50);                         // the same length: the count goes on
  EXPECT_EQ(83'416'667 + 40'000'000, clock.now());
}

}
