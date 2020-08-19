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

}
