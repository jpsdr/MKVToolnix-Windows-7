#include "common/common_pch.h"

#include "common/bcp47.h"

#include "gtest/gtest.h"

namespace {

TEST(BCP47LanguageTags, Construction) {
  EXPECT_FALSE(mtx::bcp47::language_c{}.is_valid());
}

TEST(BCP47LanguageTags, ParsingValid) {
  EXPECT_TRUE(mtx::bcp47::language_c::parse("de-Latn-CH-x-weeee").is_valid());
  EXPECT_TRUE(mtx::bcp47::language_c::parse("deu-Latn-CH-x-weeee").is_valid());
  EXPECT_TRUE(mtx::bcp47::language_c::parse("ger-Latn-076-x-weeee").is_valid());
  EXPECT_TRUE(mtx::bcp47::language_c::parse("x-muh-to-the-kuh").is_valid());
  EXPECT_TRUE(mtx::bcp47::language_c::parse("ar-aao-Latn-DZ").is_valid());
  EXPECT_TRUE(mtx::bcp47::language_c::parse("sr-ekavsk").is_valid());
  EXPECT_TRUE(mtx::bcp47::language_c::parse("sr-Latn-ekavsk").is_valid());
  EXPECT_TRUE(mtx::bcp47::language_c::parse("sr-Cyrl-ekavsk").is_valid());
}

TEST(BCP47LanguageTags, ParsingValidUNM49) {
  EXPECT_EQ("es-MX"s,  mtx::bcp47::language_c::parse("es-484").format());
  EXPECT_EQ("es-419"s, mtx::bcp47::language_c::parse("es-419").format());
}

TEST(BCP47LanguageTags, ParsingInvalid) {
  EXPECT_FALSE(mtx::bcp47::language_c::parse("muh-Latn-CH-x-weeee").is_valid());  // invalid (muh not ISO 639 code).is_valid())
  EXPECT_FALSE(mtx::bcp47::language_c::parse("ger-muku-CH-x-weeee").is_valid());  // invalid (muku not a script).is_valid())
  EXPECT_FALSE(mtx::bcp47::language_c::parse("ger-777").is_valid());              // invalid (777 not a region code).is_valid())
  EXPECT_FALSE(mtx::bcp47::language_c::parse("zh-min").is_valid());               // invalid (min not allowed with zh).is_valid())
  EXPECT_FALSE(mtx::bcp47::language_c::parse("gonzo").is_valid());                // invalid
  EXPECT_FALSE(mtx::bcp47::language_c::parse("de-aao-Latn-DZ").is_valid());       // invalid (aoo not valid with de).is_valid())
  EXPECT_FALSE(mtx::bcp47::language_c::parse("de-ekavsk").is_valid());            // invalid (ekavsk not valid with de).is_valid())
}

TEST(BCP47LanguageTags, Formatting) {
  EXPECT_EQ(""s, mtx::bcp47::language_c{}.format());

  mtx::bcp47::language_c l;

  l.set_valid(true);
  l.set_language("de");
  l.set_script("latn");
  l.set_region("ch");
  l.set_private_use({ "weee"s });

  EXPECT_EQ("de-Latn-CH-x-weee"s, l.format());
  EXPECT_EQ("German (de-Latn-CH-x-weee)"s, l.format_long());

  l = mtx::bcp47::language_c{};
  l.set_valid(true);
  l.set_language("DE");
  l.set_script("LATN");
  l.set_region("cH");
  l.set_private_use({ "WEEE"s });

  EXPECT_EQ("de-Latn-CH-x-weee"s, l.format());
  EXPECT_EQ("German (de-Latn-CH-x-weee)"s, l.format_long());

  l = mtx::bcp47::language_c{};
  l.set_valid(true);
  l.set_private_use({ "weee"s, "wooo"s });

  EXPECT_EQ("x-weee-wooo"s, l.format());
  EXPECT_EQ("x-weee-wooo"s, l.format_long());

  l = mtx::bcp47::language_c{};
  l.set_private_use({ "weee"s, "wooo"s });

  EXPECT_EQ(""s,            l.format());
  EXPECT_EQ(""s,            l.format(false));
  EXPECT_EQ("x-weee-wooo"s, l.format(true));

  EXPECT_EQ(""s,            l.format_long());
  EXPECT_EQ(""s,            l.format_long(false));
  EXPECT_EQ("x-weee-wooo"s, l.format_long(true));
}

TEST(BCP47LanguageTags, FormattingInvalidWithoutLanguage) {
  auto l = mtx::bcp47::language_c{};

  l.set_region("FR"s);
  l.set_private_use({ "moo"s });

  EXPECT_EQ(""s,          l.format());
  EXPECT_EQ("-FR-x-moo"s, l.format(true));
}

TEST(BCP47LanguageTags, CodeConversion) {
  EXPECT_EQ(""s,    mtx::bcp47::language_c{}.get_iso639_2_code());
  EXPECT_EQ("ger"s, mtx::bcp47::language_c::parse("de").get_iso639_2_code());
  EXPECT_EQ("ger"s, mtx::bcp47::language_c::parse("deu").get_iso639_2_code());
  EXPECT_EQ("ger"s, mtx::bcp47::language_c::parse("ger").get_iso639_2_code());

  mtx::bcp47::language_c l;
  l.set_language("muh");
  EXPECT_EQ(""s, l.get_iso639_2_code());

  EXPECT_FALSE(mtx::bcp47::language_c{}.has_valid_iso639_code());
  EXPECT_FALSE(mtx::bcp47::language_c::parse("muh").has_valid_iso639_code());
  EXPECT_FALSE(mtx::bcp47::language_c::parse("x-muh").has_valid_iso639_code());

  EXPECT_TRUE(mtx::bcp47::language_c::parse("de").has_valid_iso639_code());
  EXPECT_TRUE(mtx::bcp47::language_c::parse("deu").has_valid_iso639_code());
  EXPECT_TRUE(mtx::bcp47::language_c::parse("ger").has_valid_iso639_code());

  EXPECT_EQ("ger"s, mtx::bcp47::language_c::parse("de").get_iso639_2_code_or("eng"s));
  EXPECT_EQ("ger"s, mtx::bcp47::language_c::parse("deu").get_iso639_2_code_or("eng"s));
  EXPECT_EQ("ger"s, mtx::bcp47::language_c::parse("ger").get_iso639_2_code_or("eng"s));

  EXPECT_EQ("eng"s, mtx::bcp47::language_c::parse("").get_iso639_2_code_or("eng"s));
  EXPECT_EQ("eng"s, mtx::bcp47::language_c::parse("x-moo").get_iso639_2_code_or("eng"s));
}

TEST(BCP47LanguageTags, UnorderedMap) {
  std::unordered_map<mtx::bcp47::language_c, int> m;

  m[mtx::bcp47::language_c::parse("de-Latn")] = 42;
  EXPECT_EQ(42, m[mtx::bcp47::language_c::parse("de-Latn")]);
  EXPECT_EQ(0,  m[mtx::bcp47::language_c::parse("de")]);
}

TEST(BCP47LanguageTags, Clearing) {
  auto l = mtx::bcp47::language_c::parse("eng");

  ASSERT_TRUE(l.has_valid_iso639_code());

  l.clear();

  EXPECT_FALSE(l.is_valid());
}

TEST(BCP47LanguageTags, EqualityOperators) {
  EXPECT_TRUE(mtx::bcp47::language_c::parse("ger") == mtx::bcp47::language_c::parse("ger"));
  EXPECT_TRUE(mtx::bcp47::language_c::parse("ger") == mtx::bcp47::language_c::parse("deu"));
  EXPECT_TRUE(mtx::bcp47::language_c::parse("ger") == mtx::bcp47::language_c::parse("de"));

  EXPECT_TRUE(mtx::bcp47::language_c::parse("deu") == mtx::bcp47::language_c::parse("ger"));
  EXPECT_TRUE(mtx::bcp47::language_c::parse("de")  == mtx::bcp47::language_c::parse("ger"));

  EXPECT_FALSE(mtx::bcp47::language_c::parse("eng") == mtx::bcp47::language_c::parse("ger"));
  EXPECT_FALSE(mtx::bcp47::language_c::parse("eng") == mtx::bcp47::language_c::parse("deu"));
  EXPECT_FALSE(mtx::bcp47::language_c::parse("eng") == mtx::bcp47::language_c::parse("de"));

  EXPECT_FALSE(mtx::bcp47::language_c::parse("ger") == mtx::bcp47::language_c::parse("eng"));
  EXPECT_FALSE(mtx::bcp47::language_c::parse("deu") == mtx::bcp47::language_c::parse("eng"));
  EXPECT_FALSE(mtx::bcp47::language_c::parse("de")  == mtx::bcp47::language_c::parse("eng"));

  EXPECT_FALSE(mtx::bcp47::language_c::parse("ger") != mtx::bcp47::language_c::parse("ger"));
  EXPECT_FALSE(mtx::bcp47::language_c::parse("ger") != mtx::bcp47::language_c::parse("deu"));
  EXPECT_FALSE(mtx::bcp47::language_c::parse("ger") != mtx::bcp47::language_c::parse("de"));

  EXPECT_FALSE(mtx::bcp47::language_c::parse("deu") != mtx::bcp47::language_c::parse("ger"));
  EXPECT_FALSE(mtx::bcp47::language_c::parse("de")  != mtx::bcp47::language_c::parse("ger"));

  EXPECT_TRUE(mtx::bcp47::language_c::parse("eng") != mtx::bcp47::language_c::parse("ger"));
  EXPECT_TRUE(mtx::bcp47::language_c::parse("eng") != mtx::bcp47::language_c::parse("deu"));
  EXPECT_TRUE(mtx::bcp47::language_c::parse("eng") != mtx::bcp47::language_c::parse("de"));

  EXPECT_TRUE(mtx::bcp47::language_c::parse("ger") != mtx::bcp47::language_c::parse("eng"));
  EXPECT_TRUE(mtx::bcp47::language_c::parse("deu") != mtx::bcp47::language_c::parse("eng"));
  EXPECT_TRUE(mtx::bcp47::language_c::parse("de")  != mtx::bcp47::language_c::parse("eng"));
}

}
