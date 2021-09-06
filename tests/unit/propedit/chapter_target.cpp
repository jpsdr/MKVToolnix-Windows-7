#include "common/common_pch.h"

#include "propedit/chapter_target.h"

#include "tests/unit/init.h"

namespace {

TEST(ChapterTarget, Basics) {
  chapter_target_c ct;

  EXPECT_TRUE(ct.has_changes());
}


}
