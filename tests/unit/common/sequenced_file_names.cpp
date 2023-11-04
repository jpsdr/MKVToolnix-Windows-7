#include "common/common_pch.h"

#include "common/qt.h"
#include "common/sequenced_file_names.h"

#include "tests/unit/init.h"

using namespace mtx::sequenced_file_names;

namespace {

TEST(SequencedFileName, NoMatch) {
  EXPECT_FALSE(analyzeFileNameForSequenceData(Q("/home/test/asd.mkv")).has_value());
  EXPECT_FALSE(analyzeFileNameForSequenceData(Q("/home/test/4711/moocow.mkv")).has_value());
  EXPECT_FALSE(analyzeFileNameForSequenceData(Q("/home/test/4711asd/moocow.mkv")).has_value());
  EXPECT_FALSE(analyzeFileNameForSequenceData(Q("/home/test/asd4711/moocow.mkv")).has_value());
}

TEST(SequencedFileName, PrefixNumberSuffixRecognition) {
  auto s = analyzeFileNameForSequenceData(Q("/home/test/4711.mkv"));
  ASSERT_TRUE(s.has_value());
  EXPECT_EQ(s->mode, SequencedFileNameData::Mode::PrefixNumberSuffix);
  EXPECT_EQ(s->number, 4711);
  EXPECT_TRUE(s->prefix.isEmpty());
  EXPECT_EQ(s->suffix, Q(".mkv"));

  s = analyzeFileNameForSequenceData(Q("/home/test/4711asd.mkv"));
  ASSERT_TRUE(s.has_value());
  EXPECT_EQ(s->mode, SequencedFileNameData::Mode::PrefixNumberSuffix);
  EXPECT_EQ(s->number, 4711);
  EXPECT_TRUE(s->prefix.isEmpty());
  EXPECT_EQ(s->suffix, Q("asd.mkv"));

  s = analyzeFileNameForSequenceData(Q("/home/test/asd4711.mkv"));
  ASSERT_TRUE(s.has_value());
  EXPECT_EQ(s->mode, SequencedFileNameData::Mode::PrefixNumberSuffix);
  EXPECT_EQ(s->number, 4711);
  EXPECT_EQ(s->prefix, Q("asd"));
  EXPECT_EQ(s->suffix, Q(".mkv"));
}

TEST(SequencedFileName, PrefixNumberSuffixSequencing) {
  auto s1 = analyzeFileNameForSequenceData(Q("/home/test/4711asd0001.mkv"));
  ASSERT_TRUE(s1.has_value());
  EXPECT_EQ(s1->mode, SequencedFileNameData::Mode::PrefixNumberSuffix);
  EXPECT_EQ(s1->number, 1);
  EXPECT_EQ(s1->prefix, Q("4711asd"));
  EXPECT_EQ(s1->suffix, Q(".mkv"));

  auto s2 = analyzeFileNameForSequenceData(Q("/home/test/4711asd0002.mkv"));
  ASSERT_TRUE(s2.has_value());
  EXPECT_EQ(s2->mode, SequencedFileNameData::Mode::PrefixNumberSuffix);
  EXPECT_EQ(s2->number, 2);
  EXPECT_EQ(s2->prefix, Q("4711asd"));
  EXPECT_EQ(s2->suffix, Q(".mkv"));

  auto s4 = analyzeFileNameForSequenceData(Q("/home/test/4711asd0004.mkv"));
  ASSERT_TRUE(s4.has_value());
  EXPECT_EQ(s4->mode, SequencedFileNameData::Mode::PrefixNumberSuffix);
  EXPECT_EQ(s4->number, 4);
  EXPECT_EQ(s4->prefix, Q("4711asd"));
  EXPECT_EQ(s4->suffix, Q(".mkv"));

  EXPECT_TRUE(s2->follows(*s1));
  EXPECT_FALSE(s4->follows(*s2));
  EXPECT_FALSE(s1->follows(*s2));
}

TEST(SequencedFileName, GoPro2Recognition) {
  auto s = analyzeFileNameForSequenceData(Q("/home/test/GOPR4711.mp4"));
  ASSERT_TRUE(s.has_value());
  EXPECT_EQ(s->mode, SequencedFileNameData::Mode::GoPro);
  EXPECT_EQ(s->number, 0);
  EXPECT_EQ(s->prefix, Q("GP"));
  EXPECT_EQ(s->suffix, Q("4711.mp4"));

  s = analyzeFileNameForSequenceData(Q("/home/test/GP014711.mp4"));
  ASSERT_TRUE(s.has_value());
  EXPECT_EQ(s->mode, SequencedFileNameData::Mode::GoPro);
  EXPECT_EQ(s->number, 1);
  EXPECT_EQ(s->prefix, Q("GP"));
  EXPECT_EQ(s->suffix, Q("4711.mp4"));

  s = analyzeFileNameForSequenceData(Q("/home/test/GP424711.mp4"));
  ASSERT_TRUE(s.has_value());
  EXPECT_EQ(s->mode, SequencedFileNameData::Mode::GoPro);
  EXPECT_EQ(s->number, 42);
  EXPECT_EQ(s->prefix, Q("GP"));
  EXPECT_EQ(s->suffix, Q("4711.mp4"));
}

TEST(SequencedFileName, GoPro2Sequencing) {
  auto s0 = analyzeFileNameForSequenceData(Q("/home/test/GOPR4711.mp4"));
  ASSERT_TRUE(s0.has_value());
  EXPECT_EQ(s0->mode, SequencedFileNameData::Mode::GoPro);
  EXPECT_EQ(s0->number, 0);
  EXPECT_EQ(s0->prefix, Q("GP"));
  EXPECT_EQ(s0->suffix, Q("4711.mp4"));

  auto s1 = analyzeFileNameForSequenceData(Q("/home/test/GP014711.mp4"));
  ASSERT_TRUE(s1.has_value());
  EXPECT_EQ(s1->mode, SequencedFileNameData::Mode::GoPro);
  EXPECT_EQ(s1->number, 1);
  EXPECT_EQ(s1->prefix, Q("GP"));
  EXPECT_EQ(s1->suffix, Q("4711.mp4"));

  auto s42 = analyzeFileNameForSequenceData(Q("/home/test/GP424711.mp4"));
  ASSERT_TRUE(s42.has_value());
  EXPECT_EQ(s42->mode, SequencedFileNameData::Mode::GoPro);
  EXPECT_EQ(s42->number, 42);
  EXPECT_EQ(s42->prefix, Q("GP"));
  EXPECT_EQ(s42->suffix, Q("4711.mp4"));

  EXPECT_TRUE(s1->follows(*s0));
  EXPECT_FALSE(s42->follows(*s1));
  EXPECT_FALSE(s0->follows(*s1));
}

TEST(SequencedFileName, GoPro6Recognition) {
  auto s = analyzeFileNameForSequenceData(Q("/home/test/GH014711.mp4"));
  ASSERT_TRUE(s.has_value());
  EXPECT_EQ(s->mode, SequencedFileNameData::Mode::GoPro);
  EXPECT_EQ(s->number, 1);
  EXPECT_EQ(s->prefix, Q("GH"));
  EXPECT_EQ(s->suffix, Q("4711.mp4"));

  s = analyzeFileNameForSequenceData(Q("/home/test/GX424711.mp4"));
  ASSERT_TRUE(s.has_value());
  EXPECT_EQ(s->mode, SequencedFileNameData::Mode::GoPro);
  EXPECT_EQ(s->number, 42);
  EXPECT_EQ(s->prefix, Q("GX"));
  EXPECT_EQ(s->suffix, Q("4711.mp4"));
}

TEST(SequencedFileName, GoPro6Sequencing) {
  auto s1 = analyzeFileNameForSequenceData(Q("/home/test/GH014711.mp4"));
  ASSERT_TRUE(s1.has_value());
  EXPECT_EQ(s1->mode, SequencedFileNameData::Mode::GoPro);
  EXPECT_EQ(s1->number, 1);
  EXPECT_EQ(s1->prefix, Q("GH"));
  EXPECT_EQ(s1->suffix, Q("4711.mp4"));

  auto s2 = analyzeFileNameForSequenceData(Q("/home/test/GH024711.mp4"));
  ASSERT_TRUE(s2.has_value());
  EXPECT_EQ(s2->mode, SequencedFileNameData::Mode::GoPro);
  EXPECT_EQ(s2->number, 2);
  EXPECT_EQ(s2->prefix, Q("GH"));
  EXPECT_EQ(s2->suffix, Q("4711.mp4"));

  auto s42 = analyzeFileNameForSequenceData(Q("/home/test/GH424711.mp4"));
  ASSERT_TRUE(s42.has_value());
  EXPECT_EQ(s42->mode, SequencedFileNameData::Mode::GoPro);
  EXPECT_EQ(s42->number, 42);
  EXPECT_EQ(s42->prefix, Q("GH"));
  EXPECT_EQ(s42->suffix, Q("4711.mp4"));

  EXPECT_TRUE(s2->follows(*s1));
  EXPECT_FALSE(s42->follows(*s2));
  EXPECT_FALSE(s1->follows(*s2));
}

TEST(SequencedFileName, GoProNotSequencingDiffrentVersions) {
  auto s0 = analyzeFileNameForSequenceData(Q("/home/test/GOPR4711.mp4"));
  ASSERT_TRUE(s0.has_value());
  EXPECT_EQ(s0->mode, SequencedFileNameData::Mode::GoPro);
  EXPECT_EQ(s0->number, 0);
  EXPECT_EQ(s0->prefix, Q("GP"));
  EXPECT_EQ(s0->suffix, Q("4711.mp4"));

  auto s1 = analyzeFileNameForSequenceData(Q("/home/test/GH024711.mp4"));
  ASSERT_TRUE(s1.has_value());
  EXPECT_EQ(s1->mode, SequencedFileNameData::Mode::GoPro);
  EXPECT_EQ(s1->number, 2);
  EXPECT_EQ(s1->prefix, Q("GH"));
  EXPECT_EQ(s1->suffix, Q("4711.mp4"));

  EXPECT_FALSE(s1->follows(*s0));
  EXPECT_FALSE(s0->follows(*s1));
}

TEST(SequencedFileName, GoProNotSequencingDifferentCodecs) {
  auto s1 = analyzeFileNameForSequenceData(Q("/home/test/GH014711.mp4"));
  ASSERT_TRUE(s1.has_value());
  EXPECT_EQ(s1->mode, SequencedFileNameData::Mode::GoPro);
  EXPECT_EQ(s1->number, 1);
  EXPECT_EQ(s1->prefix, Q("GH"));
  EXPECT_EQ(s1->suffix, Q("4711.mp4"));

  auto s2 = analyzeFileNameForSequenceData(Q("/home/test/GX024711.mp4"));
  ASSERT_TRUE(s2.has_value());
  EXPECT_EQ(s2->mode, SequencedFileNameData::Mode::GoPro);
  EXPECT_EQ(s2->number, 2);
  EXPECT_EQ(s2->prefix, Q("GX"));
  EXPECT_EQ(s2->suffix, Q("4711.mp4"));

  EXPECT_FALSE(s2->follows(*s1));
  EXPECT_FALSE(s1->follows(*s2));
}

}
