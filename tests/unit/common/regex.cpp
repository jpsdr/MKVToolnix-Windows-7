#include "common/common_pch.h"

#include "common/regex.h"

#include "gtest/gtest.h"

namespace {

TEST(Regex, Replacing) {
  mtx::regex::jp::Regex re{"\\.0*$|(\\.[0-9]*[1-9])0*$"};
  auto repl = [](mtx::regex::jp::NumSub const &match) {
    return match.size() > 1 ? match[1] : ""s;
  };

  EXPECT_EQ("48000",          mtx::regex::replace("48000.",           re, "", repl));
  EXPECT_EQ("48000",          mtx::regex::replace("48000.0",          re, "", repl));
  EXPECT_EQ("48000",          mtx::regex::replace("48000.000",        re, "", repl));
  EXPECT_EQ("48000.0012",     mtx::regex::replace("48000.0012",       re, "", repl));
  EXPECT_EQ("48000.0012",     mtx::regex::replace("48000.001200",     re, "", repl));
  EXPECT_EQ("48000.00120034", mtx::regex::replace("48000.00120034",   re, "", repl));
  EXPECT_EQ("48000.00120034", mtx::regex::replace("48000.0012003400", re, "", repl));
}

}
