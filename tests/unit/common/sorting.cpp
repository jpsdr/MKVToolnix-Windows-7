#include "common/common_pch.h"

#include "common/sorting.h"

#include "tests/unit/init.h"

namespace {

TEST(Sorting, Naturally) {
  std::vector<std::string> v;

  v.emplace_back("muh495.txt"s);
  v.emplace_back("muh0123.txt"s);
  v.emplace_back("muh12.txt"s);
  v.emplace_back("muh495a10.txt"s);
  v.emplace_back("muh495a5.txt"s);

  mtx::sort::naturally(v.begin(), v.end());

  EXPECT_EQ("muh12.txt"s,     v[0]);
  EXPECT_EQ("muh0123.txt"s,   v[1]);
  EXPECT_EQ("muh495.txt"s,    v[2]);
  EXPECT_EQ("muh495a5.txt"s,  v[3]);
  EXPECT_EQ("muh495a10.txt"s, v[4]);

}

}
