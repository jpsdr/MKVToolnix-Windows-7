#include "common/common_pch.h"

#include "common/strings/editing.h"

#include "gtest/gtest.h"

namespace {

TEST(StringsEditing, NormalizeLineEndings) {
  EXPECT_EQ("this\nis\n\na kind\n\nof\nmagic",             mtx::string::normalize_line_endings("this\ris\r\ra kind\r\n\r\nof\nmagic"));
  EXPECT_EQ("this\nis\n\na kind\n\nof\nmagic",             mtx::string::normalize_line_endings("this\ris\r\ra kind\r\n\r\nof\nmagic", mtx::string::line_ending_style_e::lf));
  EXPECT_EQ("this\r\nis\r\n\r\na kind\r\n\r\nof\r\nmagic", mtx::string::normalize_line_endings("this\ris\r\ra kind\r\n\r\nof\nmagic", mtx::string::line_ending_style_e::cr_lf));
  EXPECT_EQ("",                                            mtx::string::normalize_line_endings("",                                    mtx::string::line_ending_style_e::lf));
  EXPECT_EQ("",                                            mtx::string::normalize_line_endings("",                                    mtx::string::line_ending_style_e::cr_lf));
}

TEST(StringsEditing, Chomp) {
  EXPECT_EQ("this\ris\r\ra kind\r\n\r\nof\nmagic",  mtx::string::chomp("this\ris\r\ra kind\r\n\r\nof\nmagic\r\n\r\n"));
  EXPECT_EQ("this\ris\r\ra kind\r\n\r\nof\nmagic",  mtx::string::chomp("this\ris\r\ra kind\r\n\r\nof\nmagic\n\n"));
  EXPECT_EQ("this\ris\r\ra kind\r\n\r\nof\nmagic",  mtx::string::chomp("this\ris\r\ra kind\r\n\r\nof\nmagic\r\r"));
  EXPECT_EQ("this\ris\r\ra kind\r\n\r\nof\nmagic ", mtx::string::chomp("this\ris\r\ra kind\r\n\r\nof\nmagic "));
}

TEST(StringsEditing, SplittingByStringEmptyPattern) {
  auto r = mtx::string::split("This, and that, is stuff."s, ""s);

  ASSERT_EQ(1,                            r.size());
  EXPECT_EQ("This, and that, is stuff."s, r[0]);
}

TEST(StringsEditing, SplittingByStringEmptyText) {
  auto r = mtx::string::split(""s, ","s);

  ASSERT_EQ(1,   r.size());
  EXPECT_EQ(""s, r[0]);
}

TEST(StringsEditing, SplittingByStringOneCharPattern) {
  auto r = mtx::string::split("This, and that, is stuff."s, ","s);

  ASSERT_EQ(3,             r.size());
  EXPECT_EQ("This"s,       r[0]);
  EXPECT_EQ(" and that"s,  r[1]);
  EXPECT_EQ(" is stuff."s, r[2]);
}

TEST(StringsEditing, SplittingByStringTwoCharsPattern) {
  auto r = mtx::string::split("This, and that, is stuff."s, ", "s);

  ASSERT_EQ(3,            r.size());
  EXPECT_EQ("This"s,      r[0]);
  EXPECT_EQ("and that"s,  r[1]);
  EXPECT_EQ("is stuff."s, r[2]);
}

TEST(StringsEditing, SplittingByStringLimit2) {
  auto r = mtx::string::split("This, and that, is stuff."s, ", "s, 2);

  ASSERT_EQ(2,                      r.size());
  EXPECT_EQ("This"s,                r[0]);
  EXPECT_EQ("and that, is stuff."s, r[1]);
}

TEST(StringsEditing, SplittingByStringLimit1) {
  auto r = mtx::string::split("This, and that, is stuff."s, ", "s, 1);

  ASSERT_EQ(1,                            r.size());
  EXPECT_EQ("This, and that, is stuff."s, r[0]);
}

TEST(StringsEditing, SplittingByStringPatternAtStart) {
  auto r = mtx::string::split(",This, and that, is stuff."s, ","s);

  ASSERT_EQ(4,             r.size());
  EXPECT_EQ(""s,           r[0]);
  EXPECT_EQ("This"s,       r[1]);
  EXPECT_EQ(" and that"s,  r[2]);
  EXPECT_EQ(" is stuff."s, r[3]);
}

TEST(StringsEditing, SplittingByStringPatternAtEnd) {
  auto r = mtx::string::split("This, and that, is stuff.,"s, ","s);

  ASSERT_EQ(4,             r.size());
  EXPECT_EQ("This"s,       r[0]);
  EXPECT_EQ(" and that"s,  r[1]);
  EXPECT_EQ(" is stuff."s, r[2]);
  EXPECT_EQ(""s,           r[3]);
}

}
