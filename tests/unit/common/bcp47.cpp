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
  EXPECT_FALSE(mtx::bcp47::language_c::parse("zyx-Latn-CH-x-weeee").is_valid());  // invalid (zyx not ISO 639 code)
  EXPECT_FALSE(mtx::bcp47::language_c::parse("ger-muku-CH-x-weeee").is_valid());  // invalid (muku not a script)
  EXPECT_FALSE(mtx::bcp47::language_c::parse("ger-777").is_valid());              // invalid (777 not a region code)
  EXPECT_FALSE(mtx::bcp47::language_c::parse("zh-min").is_valid());               // invalid (min not allowed with zh)
  EXPECT_FALSE(mtx::bcp47::language_c::parse("gonzo").is_valid());                // invalid
  EXPECT_FALSE(mtx::bcp47::language_c::parse("de-aao-Latn-DZ").is_valid());       // invalid (aoo not valid with de)
  EXPECT_FALSE(mtx::bcp47::language_c::parse("de-ekavsk").is_valid());            // invalid (ekavsk not valid with de)
  EXPECT_FALSE(mtx::bcp47::language_c::parse("es-0").is_valid());                 // invalid (no such region)
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

  l = mtx::bcp47::language_c{};
  l.set_language("ja");
  l.add_extension({ "t"s, { "test"s }});
  l.add_extension({ "u"s, { "attr"s, "co"s, "phonebk"s, "attr"s, "zz"s, "oooqqq"s }});
  l.set_valid(true);

  EXPECT_EQ("ja-t-test-u-attr-co-phonebk-attr-zz-oooqqq"s, l.format());
}

TEST(BCP47LanguageTags, FormattingInvalidWithoutLanguage) {
  auto l = mtx::bcp47::language_c{};

  l.set_region("FR"s);
  l.set_private_use({ "moo"s });

  EXPECT_EQ(""s,          l.format());
  EXPECT_EQ("-FR-x-moo"s, l.format(true));
}

TEST(BCP47LanguageTags, CodeConversion) {
  EXPECT_EQ(""s,    mtx::bcp47::language_c{}.get_iso639_alpha_3_code());
  EXPECT_EQ("ger"s, mtx::bcp47::language_c::parse("de").get_iso639_alpha_3_code());
  EXPECT_EQ("ger"s, mtx::bcp47::language_c::parse("deu").get_iso639_alpha_3_code());
  EXPECT_EQ("ger"s, mtx::bcp47::language_c::parse("ger").get_iso639_alpha_3_code());

  mtx::bcp47::language_c l;
  l.set_language("zyx");
  EXPECT_EQ(""s, l.get_iso639_alpha_3_code());

  EXPECT_FALSE(mtx::bcp47::language_c{}.has_valid_iso639_code());
  EXPECT_FALSE(mtx::bcp47::language_c::parse("zyx").has_valid_iso639_code());
  EXPECT_FALSE(mtx::bcp47::language_c::parse("x-zyx").has_valid_iso639_code());

  EXPECT_TRUE(mtx::bcp47::language_c::parse("de").has_valid_iso639_code());
  EXPECT_TRUE(mtx::bcp47::language_c::parse("deu").has_valid_iso639_code());
  EXPECT_TRUE(mtx::bcp47::language_c::parse("ger").has_valid_iso639_code());

  EXPECT_EQ("ger"s, mtx::bcp47::language_c::parse("de").get_iso639_2_alpha_3_code_or("eng"s));
  EXPECT_EQ("ger"s, mtx::bcp47::language_c::parse("deu").get_iso639_2_alpha_3_code_or("eng"s));
  EXPECT_EQ("ger"s, mtx::bcp47::language_c::parse("ger").get_iso639_2_alpha_3_code_or("eng"s));

  EXPECT_EQ("eng"s, mtx::bcp47::language_c::parse("").get_iso639_2_alpha_3_code_or("eng"s));
  EXPECT_EQ("eng"s, mtx::bcp47::language_c::parse("x-moo").get_iso639_2_alpha_3_code_or("eng"s));
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

TEST(BCP47LanguageTags, DifferenceBetweenISO639_2And639_3) {
  EXPECT_TRUE(mtx::bcp47::language_c::parse("de").has_valid_iso639_code());
  EXPECT_TRUE(mtx::bcp47::language_c::parse("de").has_valid_iso639_2_code());

  EXPECT_TRUE(mtx::bcp47::language_c::parse("cmn").has_valid_iso639_code());
  EXPECT_FALSE(mtx::bcp47::language_c::parse("cmn").has_valid_iso639_2_code());
}

TEST(BCP47LanguageTags, PrefixValidation) {
  EXPECT_TRUE(mtx::bcp47::language_c::parse("de-CH-1996").is_valid());
  EXPECT_TRUE(mtx::bcp47::language_c::parse("sr-Cyrl-ekavsk").is_valid());
  EXPECT_TRUE(mtx::bcp47::language_c::parse("sr-Cyrl-SR-ekavsk").is_valid());
  EXPECT_TRUE(mtx::bcp47::language_c::parse("en-GB-scotland").is_valid());
  EXPECT_TRUE(mtx::bcp47::language_c::parse("zh-Latn-CN-pinyin").is_valid());

  EXPECT_FALSE(mtx::bcp47::language_c::parse("sr-biske").is_valid());
  EXPECT_FALSE(mtx::bcp47::language_c::parse("tr-rozaj").is_valid());

  EXPECT_TRUE(mtx::bcp47::language_c::parse("sl-rozaj").is_valid());
  EXPECT_TRUE(mtx::bcp47::language_c::parse("sl-rozaj-biske").is_valid());
  EXPECT_TRUE(mtx::bcp47::language_c::parse("sl-rozaj-1994").is_valid());
  EXPECT_TRUE(mtx::bcp47::language_c::parse("sl-rozaj-biske-1994").is_valid());
  EXPECT_FALSE(mtx::bcp47::language_c::parse("sl-1994").is_valid());
  EXPECT_FALSE(mtx::bcp47::language_c::parse("sl-biske-rozaj").is_valid());

  EXPECT_TRUE(mtx::bcp47::language_c::parse("de-1901").is_valid());
  EXPECT_TRUE(mtx::bcp47::language_c::parse("de-1996").is_valid());
  EXPECT_FALSE(mtx::bcp47::language_c::parse("de-1901-1996").is_valid());

  EXPECT_TRUE(mtx::bcp47::language_c::parse("zh-cmn").is_valid());
  EXPECT_TRUE(mtx::bcp47::language_c::parse("zh-yue").is_valid());
  EXPECT_FALSE(mtx::bcp47::language_c::parse("zh-cmn-yue").is_valid());

  EXPECT_TRUE(mtx::bcp47::language_c::parse("hy-arevela").is_valid());
  EXPECT_TRUE(mtx::bcp47::language_c::parse("hy-arevmda").is_valid());
  EXPECT_FALSE(mtx::bcp47::language_c::parse("hy-arevela-arevmda").is_valid());

  EXPECT_TRUE(mtx::bcp47::language_c::parse("ja-Latn-hepburn").is_valid());
  EXPECT_TRUE(mtx::bcp47::language_c::parse("ja-Latn-hepburn-heploc").is_valid());
  EXPECT_FALSE(mtx::bcp47::language_c::parse("ja-Latn-heploc").is_valid());

}

TEST(BCP47LanguageTags, RFC4646AssortedValid) {
  EXPECT_TRUE(mtx::bcp47::language_c::parse("de-1996").is_valid()); // section 3.1
  EXPECT_TRUE(mtx::bcp47::language_c::parse("de-Latg-1996").is_valid()); // section 3.1
  EXPECT_TRUE(mtx::bcp47::language_c::parse("de-CH-1996").is_valid()); // section 3.1
}

TEST(BCP47LanguageTags, RFC4646AssortedInvalid) {
  EXPECT_FALSE(mtx::bcp47::language_c::parse("fr-1996").is_valid()); // section 3.1
}

TEST(BCP47LanguageTags, RFC4646AppendixBValid) {
  // Simple language subtag:
  EXPECT_TRUE(mtx::bcp47::language_c::parse("de").is_valid()); // (German)
  EXPECT_TRUE(mtx::bcp47::language_c::parse("fr").is_valid()); // (French)
  EXPECT_TRUE(mtx::bcp47::language_c::parse("ja").is_valid()); // (Japanese)
  // [MKVToolNix: deactivated as grandfathered entries are currently not supported]
  // EXPECT_TRUE(mtx::bcp47::language_c::parse("i-enochian").is_valid()); // (example of a grandfathered tag)

  // Language subtag plus Script subtag:
  EXPECT_TRUE(mtx::bcp47::language_c::parse("zh-Hant").is_valid()); // (Chinese written using the Traditional Chinese script)
  EXPECT_TRUE(mtx::bcp47::language_c::parse("zh-Hans").is_valid()); // (Chinese written using the Simplified Chinese script)
  EXPECT_TRUE(mtx::bcp47::language_c::parse("sr-Cyrl").is_valid()); // (Serbian written using the Cyrillic script)
  EXPECT_TRUE(mtx::bcp47::language_c::parse("sr-Latn").is_valid()); // (Serbian written using the Latin script)

  // Language-Script-Region:
  EXPECT_TRUE(mtx::bcp47::language_c::parse("zh-Hans-CN").is_valid()); // (Chinese written using the Simplified script as used in mainland China)
  EXPECT_TRUE(mtx::bcp47::language_c::parse("sr-Latn-SR").is_valid()); // (Serbian written using the Latin script as used in Serbia) [MKVToolNix: original was CS = Serbian & Montenegro, but that country doesn't exist anymore, and neither does the code.]

  // Language-Variant:
  EXPECT_TRUE(mtx::bcp47::language_c::parse("sl-rozaj").is_valid()); // (Resian dialect of Slovenian
  EXPECT_TRUE(mtx::bcp47::language_c::parse("sl-nedis").is_valid()); // (Nadiza dialect of Slovenian)

  // Language-Region-Variant:
  EXPECT_TRUE(mtx::bcp47::language_c::parse("de-CH-1901").is_valid()); // (German as used in Switzerland using the 1901 variant [orthography])
  EXPECT_TRUE(mtx::bcp47::language_c::parse("sl-IT-nedis").is_valid()); // (Slovenian as used in Italy, Nadiza dialect)

  // Language-Script-Region-Variant:
  EXPECT_TRUE(mtx::bcp47::language_c::parse("sl-Latn-IT-nedis").is_valid()); // (Nadiza dialect of Slovenian written using the Latin script as used in Italy.  Note that this tag is NOT RECOMMENDED because subtag 'sl' has a Suppress-Script value of 'Latn')

  // Language-Region:
  EXPECT_TRUE(mtx::bcp47::language_c::parse("de-DE").is_valid()); // (German for Germany)
  EXPECT_TRUE(mtx::bcp47::language_c::parse("en-US").is_valid()); // (English as used in the United States)
  EXPECT_TRUE(mtx::bcp47::language_c::parse("es-419").is_valid()); // (Spanish appropriate for the Latin America and Caribbean region using the UN region code)

  // Private use subtags:
  EXPECT_TRUE(mtx::bcp47::language_c::parse("de-CH-x-phonebk").is_valid());
  EXPECT_TRUE(mtx::bcp47::language_c::parse("az-Arab-x-AZE-derbend").is_valid());

  // Extended language subtags (examples ONLY: extended languages MUST be defined by revision or update to this document):
  EXPECT_TRUE(mtx::bcp47::language_c::parse("zh-mnp").is_valid());
  EXPECT_FALSE(mtx::bcp47::language_c::parse("zh-mnp-nan-Hant-CN").is_valid()); // invalid as 'nan' must only be used with prefix 'zh'

  // Private use registry values:
  EXPECT_TRUE(mtx::bcp47::language_c::parse("x-whatever").is_valid()); // (private use using the singleton 'x')
  EXPECT_TRUE(mtx::bcp47::language_c::parse("qaa-Qaaa-QM-x-southern").is_valid()); // (all private tags)
  EXPECT_TRUE(mtx::bcp47::language_c::parse("de-Qaaa").is_valid()); // (German, with a private script)
  EXPECT_TRUE(mtx::bcp47::language_c::parse("sr-Latn-QM").is_valid()); // (Serbian, Latin-script, private region)
  EXPECT_TRUE(mtx::bcp47::language_c::parse("sr-Qaaa-SR").is_valid()); // (Serbian, private script, for Serbia) [MKVToolNix: original used CS = Sebia & Montenegro, a country that doesn't exist anymore]

  // Tags that use extensions (examples ONLY: extensions MUST be defined by revision or update to this document or by RFC):
  // EXPECT_TRUE(mtx::bcp47::language_c::parse("en-US-u-islamCal").is_valid());
  // EXPECT_TRUE(mtx::bcp47::language_c::parse("zh-CN-a-myExt-x-private").is_valid());
  // EXPECT_TRUE(mtx::bcp47::language_c::parse("en-a-myExt-b-another").is_valid());
}

TEST(BCP47LanguageTags, RFC4646AppendixBInvalid) {
  EXPECT_FALSE(mtx::bcp47::language_c::parse("de-419-DE").is_valid()); // (two region tags)
  EXPECT_FALSE(mtx::bcp47::language_c::parse("a-DE").is_valid()); // (use of a single-character subtag in primary position; note that there are a few grandfathered tags that start with "i-" that are valid)
  EXPECT_FALSE(mtx::bcp47::language_c::parse("ar-a-aaa-b-bbb-a-ccc").is_valid()); // (two extensions with same single-letter prefix)
}

TEST(BCP47LanguageTags, OnlyCertainScriptsAllowedOrNoScriptAtAll) {
  EXPECT_TRUE(mtx::bcp47::language_c::parse("sr-Bali").is_valid());
  EXPECT_TRUE(mtx::bcp47::language_c::parse("sr-Cyrl").is_valid());
  EXPECT_TRUE(mtx::bcp47::language_c::parse("sr-Latn").is_valid());

  EXPECT_TRUE(mtx::bcp47::language_c::parse("sr-ekavsk").is_valid());
  EXPECT_TRUE(mtx::bcp47::language_c::parse("sr-Cyrl-ekavsk").is_valid());
  EXPECT_TRUE(mtx::bcp47::language_c::parse("sr-Latn-ekavsk").is_valid());

  EXPECT_TRUE(mtx::bcp47::language_c::parse("sr-Latn-RS-ekavsk").is_valid());

  EXPECT_FALSE(mtx::bcp47::language_c::parse("sr-Bali-ekavsk").is_valid());
}

TEST(BCP47LanguageTags, ExtensionsBasics) {
  EXPECT_TRUE(mtx::bcp47::language_c::parse("ja-t-test").is_valid());
  EXPECT_TRUE(mtx::bcp47::language_c::parse("ja-t-abcdefgh").is_valid());
  EXPECT_TRUE(mtx::bcp47::language_c::parse("ja-t-test-u-attr-co-phonebk").is_valid());
  EXPECT_TRUE(mtx::bcp47::language_c::parse("ja-t-test-u-attr-co-phonebk-attr-zz-oooqqq").is_valid());
  EXPECT_TRUE(mtx::bcp47::language_c::parse("ja-u-attr-co-phonebk-t-test").is_valid());

  EXPECT_FALSE(mtx::bcp47::language_c::parse("ja-t").is_valid());                                  // Nothing following the singleton
  EXPECT_FALSE(mtx::bcp47::language_c::parse("ja-t-u-attr-co-phonebk").is_valid());                // No extension subtag within the t extension
  EXPECT_FALSE(mtx::bcp47::language_c::parse("ja-t-u-attr-co-phonebk").is_valid());                // No extension subtag within the t extension
  EXPECT_FALSE(mtx::bcp47::language_c::parse("ja-u-attr-co-phonebk-u-attr-zz-oooqqq").is_valid()); // Singleton occurring multiple times
  EXPECT_FALSE(mtx::bcp47::language_c::parse("ja-t-z").is_valid());                                // Extension subtag too short
  EXPECT_FALSE(mtx::bcp47::language_c::parse("ja-t-abcdefghi").is_valid());                        // Extension subtag too long

  EXPECT_FALSE(mtx::bcp47::language_c::parse("ja-a-moo-cow").is_valid());                          // Singleton a is not registered at the moment
}

TEST(BCP47LanguageTags, ExtensionsRFC6067) {
  EXPECT_TRUE(mtx::bcp47::language_c::parse("de-DE-u-attr-co-phonebk").is_valid());
}

TEST(BCP47LanguageTags, ExtensionsRFC6497) {
  EXPECT_TRUE(mtx::bcp47::language_c::parse("und-Cyrl-t-und-latn-m0-ungegn-2007").is_valid());
  EXPECT_TRUE(mtx::bcp47::language_c::parse("und-Hebr-t-und-latn-m0-ungegn-1972").is_valid());
  EXPECT_TRUE(mtx::bcp47::language_c::parse("ja-t-it-m0-xxx-v21a-2007").is_valid());
}

}
