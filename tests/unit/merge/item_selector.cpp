#include "common/common_pch.h"

#include "merge/item_selector.h"

#include "tests/unit/init.h"

namespace {

TEST(ItemSelector, NoneEmpty) {
  auto is = item_selector_c<bool>{};
  is.set_none();

  EXPECT_FALSE(is.selected(42));
  EXPECT_FALSE(is.selected(42, mtx::bcp47::language_c::parse("eng")));
}

TEST(ItemSelector, NoneIDsOnly) {
  auto is = item_selector_c<bool>{};
  is.set_none();

  is.add(23);
  is.add(42);

  EXPECT_FALSE(is.selected(42));
  EXPECT_FALSE(is.selected(42, mtx::bcp47::language_c::parse("eng")));
}

TEST(ItemSelector, NoneLanguagesOnly) {
  auto is = item_selector_c<bool>{};
  is.set_none();

  is.add(mtx::bcp47::language_c::parse("ger"));
  is.add(mtx::bcp47::language_c::parse("eng"));

  EXPECT_FALSE(is.selected(42));
  EXPECT_FALSE(is.selected(42, mtx::bcp47::language_c::parse("eng")));
}

TEST(ItemSelector, NoneIDsAndLanguages) {
  auto is = item_selector_c<bool>{};
  is.set_none();

  is.add(23);
  is.add(42);
  is.add(mtx::bcp47::language_c::parse("ger"));
  is.add(mtx::bcp47::language_c::parse("eng"));

  EXPECT_FALSE(is.selected(42));
  EXPECT_FALSE(is.selected(42, mtx::bcp47::language_c::parse("eng")));
}

TEST(ItemSelector, Empty) {
  auto is = item_selector_c<bool>{};

  EXPECT_TRUE(is.selected(42));
  EXPECT_TRUE(is.selected(42, mtx::bcp47::language_c::parse("eng")));

  EXPECT_TRUE(is.selected(54));
  EXPECT_TRUE(is.selected(54, mtx::bcp47::language_c::parse("eng")));
}

TEST(ItemSelector, IDsOnly) {
  auto is = item_selector_c<bool>{};

  is.add(23);
  is.add(42);

  EXPECT_TRUE(is.selected(42));
  EXPECT_TRUE(is.selected(42, mtx::bcp47::language_c::parse("eng")));

  EXPECT_FALSE(is.selected(54));
  EXPECT_FALSE(is.selected(54, mtx::bcp47::language_c::parse("eng")));
}

TEST(ItemSelector, LanguagesOnly) {
  auto is = item_selector_c<bool>{};

  is.add(mtx::bcp47::language_c::parse("ger"));
  is.add(mtx::bcp47::language_c::parse("eng"));

  EXPECT_FALSE(is.selected(42));
  EXPECT_TRUE(is.selected(42, mtx::bcp47::language_c::parse("eng")));

  EXPECT_FALSE(is.selected(54));
  EXPECT_TRUE(is.selected(54, mtx::bcp47::language_c::parse("eng")));
}

TEST(ItemSelector, IDsAndLanguages) {
  auto is = item_selector_c<bool>{};

  is.add(23);
  is.add(42);
  is.add(mtx::bcp47::language_c::parse("ger"));
  is.add(mtx::bcp47::language_c::parse("eng"));

  EXPECT_TRUE(is.selected(42));
  EXPECT_TRUE(is.selected(42, mtx::bcp47::language_c::parse("fre")));
  EXPECT_TRUE(is.selected(42, mtx::bcp47::language_c::parse("eng")));

  EXPECT_FALSE(is.selected(54));
  EXPECT_FALSE(is.selected(54, mtx::bcp47::language_c::parse("fre")));
  EXPECT_TRUE(is.selected(54, mtx::bcp47::language_c::parse("eng")));
}

TEST(ItemSelector, ReversedEmpty) {
  auto is = item_selector_c<bool>{};
  is.set_reversed();

  EXPECT_FALSE(is.selected(42));
  EXPECT_FALSE(is.selected(42, mtx::bcp47::language_c::parse("fre")));
  EXPECT_FALSE(is.selected(42, mtx::bcp47::language_c::parse("eng")));

  EXPECT_FALSE(is.selected(54));
  EXPECT_FALSE(is.selected(54, mtx::bcp47::language_c::parse("fre")));
  EXPECT_FALSE(is.selected(54, mtx::bcp47::language_c::parse("eng")));
}

TEST(ItemSelector, ReversedIDsOnly) {
  auto is = item_selector_c<bool>{};
  is.set_reversed();

  is.add(23);
  is.add(42);

  EXPECT_FALSE(is.selected(42));
  EXPECT_FALSE(is.selected(42, mtx::bcp47::language_c::parse("fre")));
  EXPECT_FALSE(is.selected(42, mtx::bcp47::language_c::parse("eng")));

  EXPECT_TRUE(is.selected(54));
  EXPECT_TRUE(is.selected(54, mtx::bcp47::language_c::parse("fre")));
  EXPECT_TRUE(is.selected(54, mtx::bcp47::language_c::parse("eng")));
}

TEST(ItemSelector, ReversedLanguagesOnly) {
  auto is = item_selector_c<bool>{};
  is.set_reversed();

  is.add(mtx::bcp47::language_c::parse("ger"));
  is.add(mtx::bcp47::language_c::parse("eng"));

  EXPECT_TRUE(is.selected(42));
  EXPECT_TRUE(is.selected(42, mtx::bcp47::language_c::parse("fre")));
  EXPECT_FALSE(is.selected(42, mtx::bcp47::language_c::parse("eng")));

  EXPECT_TRUE(is.selected(54));
  EXPECT_TRUE(is.selected(54, mtx::bcp47::language_c::parse("fre")));
  EXPECT_FALSE(is.selected(54, mtx::bcp47::language_c::parse("eng")));
}

TEST(ItemSelector, ReversedIDsAndLanguages) {
  auto is = item_selector_c<bool>{};
  is.set_reversed();

  is.add(23);
  is.add(42);
  is.add(mtx::bcp47::language_c::parse("ger"));
  is.add(mtx::bcp47::language_c::parse("eng"));

  EXPECT_FALSE(is.selected(42));
  EXPECT_FALSE(is.selected(42, mtx::bcp47::language_c::parse("fre")));
  EXPECT_FALSE(is.selected(42, mtx::bcp47::language_c::parse("eng")));

  EXPECT_TRUE(is.selected(54));
  EXPECT_TRUE(is.selected(54, mtx::bcp47::language_c::parse("fre")));
  EXPECT_FALSE(is.selected(54, mtx::bcp47::language_c::parse("eng")));
}

TEST(ItemSelector, LanguageMatching) {
  auto is = item_selector_c<int>{};

  is.add(mtx::bcp47::language_c::parse("en"));
  is.add(mtx::bcp47::language_c::parse("en-US"));
  is.add(mtx::bcp47::language_c::parse("en-AU"));
  is.add(mtx::bcp47::language_c::parse("es"),    1);
  is.add(mtx::bcp47::language_c::parse("es-ES"), 2);
  is.add(mtx::bcp47::language_c::parse("es-MX"), 3);

  EXPECT_FALSE(is.selected(42, mtx::bcp47::language_c::parse("de")));

  EXPECT_TRUE(is.selected(42, mtx::bcp47::language_c::parse("en")));
  EXPECT_TRUE(is.selected(42, mtx::bcp47::language_c::parse("en-US")));
  EXPECT_TRUE(is.selected(42, mtx::bcp47::language_c::parse("en-AU")));

  EXPECT_TRUE(is.selected(42, mtx::bcp47::language_c::parse("es")));
  EXPECT_TRUE(is.selected(42, mtx::bcp47::language_c::parse("es-ES")));
  EXPECT_TRUE(is.selected(42, mtx::bcp47::language_c::parse("es-MX")));
  EXPECT_TRUE(is.selected(42, mtx::bcp47::language_c::parse("es-US")));

  EXPECT_EQ(0, is.get(42, mtx::bcp47::language_c::parse("de")));

  EXPECT_EQ(1, is.get(42, mtx::bcp47::language_c::parse("es")));
  EXPECT_EQ(2, is.get(42, mtx::bcp47::language_c::parse("es-ES")));
  EXPECT_EQ(3, is.get(42, mtx::bcp47::language_c::parse("es-MX")));
  EXPECT_EQ(1, is.get(42, mtx::bcp47::language_c::parse("es-US")));
}

}
