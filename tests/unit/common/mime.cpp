#include "common/common_pch.h"

#include "common/mime.h"

#include "tests/unit/init.h"

namespace {

TEST(FontMimeTypes, normalize_legacy_to_current) {
  using namespace mtx::mime;

  EXPECT_EQ("font/otf", get_font_mime_type_to_use("application/vnd.ms-opentype", font_mime_type_type_e::current));
  EXPECT_EQ("font/otf", get_font_mime_type_to_use("application/x-font-otf",      font_mime_type_type_e::current));
  EXPECT_EQ("font/ttf", get_font_mime_type_to_use("application/x-font-ttf",      font_mime_type_type_e::current));
  EXPECT_EQ("font/ttf", get_font_mime_type_to_use("application/x-truetype-font", font_mime_type_type_e::current));
}

TEST(FontMimeTypes, normalize_current_to_legacy) {
  using namespace mtx::mime;

  EXPECT_EQ("application/vnd.ms-opentype", get_font_mime_type_to_use("font/otf",        font_mime_type_type_e::legacy));
  EXPECT_EQ("application/x-truetype-font", get_font_mime_type_to_use("font/ttf",        font_mime_type_type_e::legacy));
  EXPECT_EQ("application/x-truetype-font", get_font_mime_type_to_use("font/collection", font_mime_type_type_e::legacy));
}

}
