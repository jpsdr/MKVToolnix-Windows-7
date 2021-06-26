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

TEST(StringsEditing, SplittingRegexByStringEmptyPattern) {
  auto r = mtx::string::split("This, and that, is stuff."s, QRegularExpression{""});

  ASSERT_EQ(27,  r.size());
  EXPECT_EQ("",  r[0]);
  EXPECT_EQ("T", r[1]);
  EXPECT_EQ("h", r[2]);
  EXPECT_EQ("i", r[3]);
  EXPECT_EQ("s", r[4]);
  EXPECT_EQ(",", r[5]);
  EXPECT_EQ(" ", r[6]);
  EXPECT_EQ("a", r[7]);
  EXPECT_EQ("n", r[8]);
  EXPECT_EQ("d", r[9]);
  EXPECT_EQ(" ", r[10]);
  EXPECT_EQ("t", r[11]);
  EXPECT_EQ("h", r[12]);
  EXPECT_EQ("a", r[13]);
  EXPECT_EQ("t", r[14]);
  EXPECT_EQ(",", r[15]);
  EXPECT_EQ(" ", r[16]);
  EXPECT_EQ("i", r[17]);
  EXPECT_EQ("s", r[18]);
  EXPECT_EQ(" ", r[19]);
  EXPECT_EQ("s", r[20]);
  EXPECT_EQ("t", r[21]);
  EXPECT_EQ("u", r[22]);
  EXPECT_EQ("f", r[23]);
  EXPECT_EQ("f", r[24]);
  EXPECT_EQ(".", r[25]);
  EXPECT_EQ("",  r[26]);
}

TEST(StringsEditing, SplittingRegexByStringEmptyText) {
  auto r = mtx::string::split(""s, QRegularExpression{","});

  ASSERT_EQ(1,   r.size());
  EXPECT_EQ(""s, r[0]);
}

TEST(StringsEditing, SplittingRegexByStringOneCharPattern) {
  auto r = mtx::string::split("This, and that, is stuff."s, QRegularExpression{","});

  ASSERT_EQ(3,             r.size());
  EXPECT_EQ("This"s,       r[0]);
  EXPECT_EQ(" and that"s,  r[1]);
  EXPECT_EQ(" is stuff."s, r[2]);
}

TEST(StringsEditing, SplittingRegexByStringTwoCharsPattern) {
  auto r = mtx::string::split("This,  and that,    is stuff."s, QRegularExpression{", *"});

  ASSERT_EQ(3,            r.size());
  EXPECT_EQ("This"s,      r[0]);
  EXPECT_EQ("and that"s,  r[1]);
  EXPECT_EQ("is stuff."s, r[2]);
}

TEST(StringsEditing, SplittingRegexByStringLimit2) {
  auto r = mtx::string::split("This,  and that,   is stuff."s, QRegularExpression{", *"}, 2);

  ASSERT_EQ(2,                        r.size());
  EXPECT_EQ("This"s,                  r[0]);
  EXPECT_EQ("and that,   is stuff."s, r[1]);
}

TEST(StringsEditing, SplittingRegexByStringLimit1) {
  auto r = mtx::string::split("This,  and that,   is stuff."s, QRegularExpression{", *"}, 1);

  ASSERT_EQ(1,                               r.size());
  EXPECT_EQ("This,  and that,   is stuff."s, r[0]);
}

TEST(StringsEditing, SplittingRegexByStringPatternAtStart) {
  auto r = mtx::string::split(",This, and that, is stuff."s, QRegularExpression{", *"});

  ASSERT_EQ(4,            r.size());
  EXPECT_EQ(""s,          r[0]);
  EXPECT_EQ("This"s,      r[1]);
  EXPECT_EQ("and that"s,  r[2]);
  EXPECT_EQ("is stuff."s, r[3]);
}

TEST(StringsEditing, SplittingRegexByStringPatternAtEnd) {
  auto r = mtx::string::split("This, and that, is stuff.,"s, QRegularExpression{", *"});

  ASSERT_EQ(4,            r.size());
  EXPECT_EQ("This"s,      r[0]);
  EXPECT_EQ("and that"s,  r[1]);
  EXPECT_EQ("is stuff."s, r[2]);
  EXPECT_EQ(""s,          r[3]);
}

TEST(StringsEditing, ReplacingRegex) {
  QRegularExpression re{"\\.0*$|(\\.[0-9]*[1-9])0*$"};
  auto repl = [](QRegularExpressionMatch const &match) {
    return match.captured(1);
  };

  EXPECT_EQ("48000",          mtx::string::replace("48000.",           re, repl));
  EXPECT_EQ("48000",          mtx::string::replace("48000.0",          re, repl));
  EXPECT_EQ("48000",          mtx::string::replace("48000.000",        re, repl));
  EXPECT_EQ("48000.0012",     mtx::string::replace("48000.0012",       re, repl));
  EXPECT_EQ("48000.0012",     mtx::string::replace("48000.001200",     re, repl));
  EXPECT_EQ("48000.00120034", mtx::string::replace("48000.00120034",   re, repl));
  EXPECT_EQ("48000.00120034", mtx::string::replace("48000.0012003400", re, repl));
}

}
