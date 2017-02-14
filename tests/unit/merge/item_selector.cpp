#include "common/common_pch.h"

#include "merge/item_selector.h"

#include "gtest/gtest.h"

namespace {

TEST(ItemSelector, NoneEmpty) {
  auto is = item_selector_c<bool>{};
  is.set_none();

  EXPECT_FALSE(is.selected(42));
  EXPECT_FALSE(is.selected(42, std::string{"eng"}));
}

TEST(ItemSelector, NoneIDsOnly) {
  auto is = item_selector_c<bool>{};
  is.set_none();

  is.add(23);
  is.add(42);

  EXPECT_FALSE(is.selected(42));
  EXPECT_FALSE(is.selected(42, std::string{"eng"}));
}

TEST(ItemSelector, NoneLanguagesOnly) {
  auto is = item_selector_c<bool>{};
  is.set_none();

  is.add("ger");
  is.add("eng");

  EXPECT_FALSE(is.selected(42));
  EXPECT_FALSE(is.selected(42, std::string{"eng"}));
}

TEST(ItemSelector, NoneIDsAndLanguages) {
  auto is = item_selector_c<bool>{};
  is.set_none();

  is.add(23);
  is.add(42);
  is.add("ger");
  is.add("eng");

  EXPECT_FALSE(is.selected(42));
  EXPECT_FALSE(is.selected(42, std::string{"eng"}));
}

TEST(ItemSelector, Empty) {
  auto is = item_selector_c<bool>{};

  EXPECT_TRUE(is.selected(42));
  EXPECT_TRUE(is.selected(42, std::string{"eng"}));

  EXPECT_TRUE(is.selected(54));
  EXPECT_TRUE(is.selected(54, std::string{"eng"}));
}

TEST(ItemSelector, IDsOnly) {
  auto is = item_selector_c<bool>{};

  is.add(23);
  is.add(42);

  EXPECT_TRUE(is.selected(42));
  EXPECT_TRUE(is.selected(42, std::string{"eng"}));

  EXPECT_FALSE(is.selected(54));
  EXPECT_FALSE(is.selected(54, std::string{"eng"}));
}

TEST(ItemSelector, LanguagesOnly) {
  auto is = item_selector_c<bool>{};

  is.add("ger");
  is.add("eng");

  EXPECT_FALSE(is.selected(42));
  EXPECT_TRUE(is.selected(42, std::string{"eng"}));

  EXPECT_FALSE(is.selected(54));
  EXPECT_TRUE(is.selected(54, std::string{"eng"}));
}

TEST(ItemSelector, IDsAndLanguages) {
  auto is = item_selector_c<bool>{};

  is.add(23);
  is.add(42);
  is.add("ger");
  is.add("eng");

  EXPECT_TRUE(is.selected(42));
  EXPECT_TRUE(is.selected(42, std::string{"fre"}));
  EXPECT_TRUE(is.selected(42, std::string{"eng"}));

  EXPECT_FALSE(is.selected(54));
  EXPECT_FALSE(is.selected(54, std::string{"fre"}));
  EXPECT_TRUE(is.selected(54, std::string{"eng"}));
}

TEST(ItemSelector, ReversedEmpty) {
  auto is = item_selector_c<bool>{};
  is.set_reversed();

  EXPECT_FALSE(is.selected(42));
  EXPECT_FALSE(is.selected(42, std::string{"fre"}));
  EXPECT_FALSE(is.selected(42, std::string{"eng"}));

  EXPECT_FALSE(is.selected(54));
  EXPECT_FALSE(is.selected(54, std::string{"fre"}));
  EXPECT_FALSE(is.selected(54, std::string{"eng"}));
}

TEST(ItemSelector, ReversedIDsOnly) {
  auto is = item_selector_c<bool>{};
  is.set_reversed();

  is.add(23);
  is.add(42);

  EXPECT_FALSE(is.selected(42));
  EXPECT_FALSE(is.selected(42, std::string{"fre"}));
  EXPECT_FALSE(is.selected(42, std::string{"eng"}));

  EXPECT_TRUE(is.selected(54));
  EXPECT_TRUE(is.selected(54, std::string{"fre"}));
  EXPECT_TRUE(is.selected(54, std::string{"eng"}));
}

TEST(ItemSelector, ReversedLanguagesOnly) {
  auto is = item_selector_c<bool>{};
  is.set_reversed();

  is.add("ger");
  is.add("eng");

  EXPECT_TRUE(is.selected(42));
  EXPECT_TRUE(is.selected(42, std::string{"fre"}));
  EXPECT_FALSE(is.selected(42, std::string{"eng"}));

  EXPECT_TRUE(is.selected(54));
  EXPECT_TRUE(is.selected(54, std::string{"fre"}));
  EXPECT_FALSE(is.selected(54, std::string{"eng"}));
}

TEST(ItemSelector, ReversedIDsAndLanguages) {
  auto is = item_selector_c<bool>{};
  is.set_reversed();

  is.add(23);
  is.add(42);
  is.add("ger");
  is.add("eng");

  EXPECT_FALSE(is.selected(42));
  EXPECT_FALSE(is.selected(42, std::string{"fre"}));
  EXPECT_FALSE(is.selected(42, std::string{"eng"}));

  EXPECT_TRUE(is.selected(54));
  EXPECT_TRUE(is.selected(54, std::string{"fre"}));
  EXPECT_FALSE(is.selected(54, std::string{"eng"}));
}

}
