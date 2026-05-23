#include "common/common_pch.h"

#include "common/qt.h"
#include "common/sequenced_file_names.h"

#include "tests/unit/init.h"

using namespace mtx::sequenced_file_names;

namespace {

TEST(SequencedFileName, NoMatch) {
  EXPECT_FALSE(analyzeFileNameForSequenceData(u"/home/test/asd.mkv"_s).has_value());
  EXPECT_FALSE(analyzeFileNameForSequenceData(u"/home/test/4711/moocow.mkv"_s).has_value());
  EXPECT_FALSE(analyzeFileNameForSequenceData(u"/home/test/4711asd/moocow.mkv"_s).has_value());
  EXPECT_FALSE(analyzeFileNameForSequenceData(u"/home/test/asd4711/moocow.mkv"_s).has_value());
}

TEST(SequencedFileName, PrefixNumberSuffixRecognition) {
  auto s = analyzeFileNameForSequenceData(u"/home/test/4711.mkv"_s);
  ASSERT_TRUE(s.has_value());
  EXPECT_EQ(s->mode, SequencedFileNameData::Mode::PrefixNumberSuffix);
  EXPECT_EQ(s->number, 4711);
  EXPECT_TRUE(s->prefix.isEmpty());
  EXPECT_EQ(s->suffix, u".mkv"_s);

  s = analyzeFileNameForSequenceData(u"/home/test/4711asd.mkv"_s);
  ASSERT_TRUE(s.has_value());
  EXPECT_EQ(s->mode, SequencedFileNameData::Mode::PrefixNumberSuffix);
  EXPECT_EQ(s->number, 4711);
  EXPECT_TRUE(s->prefix.isEmpty());
  EXPECT_EQ(s->suffix, u"asd.mkv"_s);

  s = analyzeFileNameForSequenceData(u"/home/test/asd4711.mkv"_s);
  ASSERT_TRUE(s.has_value());
  EXPECT_EQ(s->mode, SequencedFileNameData::Mode::PrefixNumberSuffix);
  EXPECT_EQ(s->number, 4711);
  EXPECT_EQ(s->prefix, u"asd"_s);
  EXPECT_EQ(s->suffix, u".mkv"_s);
}

TEST(SequencedFileName, PrefixNumberSuffixSequencing) {
  auto s1 = analyzeFileNameForSequenceData(u"/home/test/4711asd0001.mkv"_s);
  ASSERT_TRUE(s1.has_value());
  EXPECT_EQ(s1->mode, SequencedFileNameData::Mode::PrefixNumberSuffix);
  EXPECT_EQ(s1->number, 1);
  EXPECT_EQ(s1->prefix, u"4711asd"_s);
  EXPECT_EQ(s1->suffix, u".mkv"_s);

  auto s2 = analyzeFileNameForSequenceData(u"/home/test/4711asd0002.mkv"_s);
  ASSERT_TRUE(s2.has_value());
  EXPECT_EQ(s2->mode, SequencedFileNameData::Mode::PrefixNumberSuffix);
  EXPECT_EQ(s2->number, 2);
  EXPECT_EQ(s2->prefix, u"4711asd"_s);
  EXPECT_EQ(s2->suffix, u".mkv"_s);

  auto s4 = analyzeFileNameForSequenceData(u"/home/test/4711asd0004.mkv"_s);
  ASSERT_TRUE(s4.has_value());
  EXPECT_EQ(s4->mode, SequencedFileNameData::Mode::PrefixNumberSuffix);
  EXPECT_EQ(s4->number, 4);
  EXPECT_EQ(s4->prefix, u"4711asd"_s);
  EXPECT_EQ(s4->suffix, u".mkv"_s);

  EXPECT_TRUE(s2->follows(*s1));
  EXPECT_FALSE(s4->follows(*s2));
  EXPECT_FALSE(s1->follows(*s2));
}

TEST(SequencedFileName, GoPro2Recognition) {
  auto s = analyzeFileNameForSequenceData(u"/home/test/GOPR4711.mp4"_s);
  ASSERT_TRUE(s.has_value());
  EXPECT_EQ(s->mode, SequencedFileNameData::Mode::GoPro);
  EXPECT_EQ(s->number, 0);
  EXPECT_EQ(s->prefix, u"GP"_s);
  EXPECT_EQ(s->suffix, u"4711.mp4"_s);

  s = analyzeFileNameForSequenceData(u"/home/test/GP014711.mp4"_s);
  ASSERT_TRUE(s.has_value());
  EXPECT_EQ(s->mode, SequencedFileNameData::Mode::GoPro);
  EXPECT_EQ(s->number, 1);
  EXPECT_EQ(s->prefix, u"GP"_s);
  EXPECT_EQ(s->suffix, u"4711.mp4"_s);

  s = analyzeFileNameForSequenceData(u"/home/test/GP424711.mp4"_s);
  ASSERT_TRUE(s.has_value());
  EXPECT_EQ(s->mode, SequencedFileNameData::Mode::GoPro);
  EXPECT_EQ(s->number, 42);
  EXPECT_EQ(s->prefix, u"GP"_s);
  EXPECT_EQ(s->suffix, u"4711.mp4"_s);
}

TEST(SequencedFileName, GoPro2Sequencing) {
  auto s0 = analyzeFileNameForSequenceData(u"/home/test/GOPR4711.mp4"_s);
  ASSERT_TRUE(s0.has_value());
  EXPECT_EQ(s0->mode, SequencedFileNameData::Mode::GoPro);
  EXPECT_EQ(s0->number, 0);
  EXPECT_EQ(s0->prefix, u"GP"_s);
  EXPECT_EQ(s0->suffix, u"4711.mp4"_s);

  auto s1 = analyzeFileNameForSequenceData(u"/home/test/GP014711.mp4"_s);
  ASSERT_TRUE(s1.has_value());
  EXPECT_EQ(s1->mode, SequencedFileNameData::Mode::GoPro);
  EXPECT_EQ(s1->number, 1);
  EXPECT_EQ(s1->prefix, u"GP"_s);
  EXPECT_EQ(s1->suffix, u"4711.mp4"_s);

  auto s42 = analyzeFileNameForSequenceData(u"/home/test/GP424711.mp4"_s);
  ASSERT_TRUE(s42.has_value());
  EXPECT_EQ(s42->mode, SequencedFileNameData::Mode::GoPro);
  EXPECT_EQ(s42->number, 42);
  EXPECT_EQ(s42->prefix, u"GP"_s);
  EXPECT_EQ(s42->suffix, u"4711.mp4"_s);

  EXPECT_TRUE(s1->follows(*s0));
  EXPECT_FALSE(s42->follows(*s1));
  EXPECT_FALSE(s0->follows(*s1));
}

TEST(SequencedFileName, GoPro6Recognition) {
  auto s = analyzeFileNameForSequenceData(u"/home/test/GH014711.mp4"_s);
  ASSERT_TRUE(s.has_value());
  EXPECT_EQ(s->mode, SequencedFileNameData::Mode::GoPro);
  EXPECT_EQ(s->number, 1);
  EXPECT_EQ(s->prefix, u"GH"_s);
  EXPECT_EQ(s->suffix, u"4711.mp4"_s);

  s = analyzeFileNameForSequenceData(u"/home/test/GX424711.mp4"_s);
  ASSERT_TRUE(s.has_value());
  EXPECT_EQ(s->mode, SequencedFileNameData::Mode::GoPro);
  EXPECT_EQ(s->number, 42);
  EXPECT_EQ(s->prefix, u"GX"_s);
  EXPECT_EQ(s->suffix, u"4711.mp4"_s);
}

TEST(SequencedFileName, GoPro6Sequencing) {
  auto s1 = analyzeFileNameForSequenceData(u"/home/test/GH014711.mp4"_s);
  ASSERT_TRUE(s1.has_value());
  EXPECT_EQ(s1->mode, SequencedFileNameData::Mode::GoPro);
  EXPECT_EQ(s1->number, 1);
  EXPECT_EQ(s1->prefix, u"GH"_s);
  EXPECT_EQ(s1->suffix, u"4711.mp4"_s);

  auto s2 = analyzeFileNameForSequenceData(u"/home/test/GH024711.mp4"_s);
  ASSERT_TRUE(s2.has_value());
  EXPECT_EQ(s2->mode, SequencedFileNameData::Mode::GoPro);
  EXPECT_EQ(s2->number, 2);
  EXPECT_EQ(s2->prefix, u"GH"_s);
  EXPECT_EQ(s2->suffix, u"4711.mp4"_s);

  auto s42 = analyzeFileNameForSequenceData(u"/home/test/GH424711.mp4"_s);
  ASSERT_TRUE(s42.has_value());
  EXPECT_EQ(s42->mode, SequencedFileNameData::Mode::GoPro);
  EXPECT_EQ(s42->number, 42);
  EXPECT_EQ(s42->prefix, u"GH"_s);
  EXPECT_EQ(s42->suffix, u"4711.mp4"_s);

  EXPECT_TRUE(s2->follows(*s1));
  EXPECT_FALSE(s42->follows(*s2));
  EXPECT_FALSE(s1->follows(*s2));
}

TEST(SequencedFileName, GoProNotSequencingDiffrentVersions) {
  auto s0 = analyzeFileNameForSequenceData(u"/home/test/GOPR4711.mp4"_s);
  ASSERT_TRUE(s0.has_value());
  EXPECT_EQ(s0->mode, SequencedFileNameData::Mode::GoPro);
  EXPECT_EQ(s0->number, 0);
  EXPECT_EQ(s0->prefix, u"GP"_s);
  EXPECT_EQ(s0->suffix, u"4711.mp4"_s);

  auto s1 = analyzeFileNameForSequenceData(u"/home/test/GH024711.mp4"_s);
  ASSERT_TRUE(s1.has_value());
  EXPECT_EQ(s1->mode, SequencedFileNameData::Mode::GoPro);
  EXPECT_EQ(s1->number, 2);
  EXPECT_EQ(s1->prefix, u"GH"_s);
  EXPECT_EQ(s1->suffix, u"4711.mp4"_s);

  EXPECT_FALSE(s1->follows(*s0));
  EXPECT_FALSE(s0->follows(*s1));
}

TEST(SequencedFileName, GoProNotSequencingDifferentCodecs) {
  auto s1 = analyzeFileNameForSequenceData(u"/home/test/GH014711.mp4"_s);
  ASSERT_TRUE(s1.has_value());
  EXPECT_EQ(s1->mode, SequencedFileNameData::Mode::GoPro);
  EXPECT_EQ(s1->number, 1);
  EXPECT_EQ(s1->prefix, u"GH"_s);
  EXPECT_EQ(s1->suffix, u"4711.mp4"_s);

  auto s2 = analyzeFileNameForSequenceData(u"/home/test/GX024711.mp4"_s);
  ASSERT_TRUE(s2.has_value());
  EXPECT_EQ(s2->mode, SequencedFileNameData::Mode::GoPro);
  EXPECT_EQ(s2->number, 2);
  EXPECT_EQ(s2->prefix, u"GX"_s);
  EXPECT_EQ(s2->suffix, u"4711.mp4"_s);

  EXPECT_FALSE(s2->follows(*s1));
  EXPECT_FALSE(s1->follows(*s2));
}

}
