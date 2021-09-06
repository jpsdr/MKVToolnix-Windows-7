#include "common/common_pch.h"

#include "common/mm_io_x.h"
#include "common/mm_file_io.h"

#include "tests/unit/init.h"
#include "tests/unit/util.h"

namespace {

TEST(MmIo, Slurp) {
  memory_cptr m;

  ASSERT_NO_THROW(m = mm_file_io_c::slurp("tests/unit/data/text/chunky_bacon.txt"));
  EXPECT_EQ("Chunky Bacon\n"s, *m);

  ASSERT_THROW(mm_file_io_c::slurp("doesnotexist"), mtx::mm_io::exception);
}

}
