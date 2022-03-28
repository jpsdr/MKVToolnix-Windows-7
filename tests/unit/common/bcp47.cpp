#include "common/common_pch.h"

#include "common/bcp47.h"

#include "tests/unit/init.h"

namespace {

using namespace mtx::bcp47;
using norm_e = mtx::bcp47::normalization_mode_e;

TEST(BCP47LanguageTags, Construction) {
  language_c::set_normalization_mode(norm_e::none);
  EXPECT_FALSE(language_c{}.is_valid());
}

TEST(BCP47LanguageTags, ParsingValid) {
  language_c::set_normalization_mode(norm_e::none);
  EXPECT_TRUE(language_c::parse("de-Latn-CH-x-weeee").is_valid());
  EXPECT_TRUE(language_c::parse("deu-Latn-CH-x-weeee").is_valid());
  EXPECT_TRUE(language_c::parse("ger-Latn-076-x-weeee").is_valid());
  EXPECT_TRUE(language_c::parse("x-muh-to-the-kuh").is_valid());
  EXPECT_TRUE(language_c::parse("ar-aao-Latn-DZ").is_valid());
  EXPECT_TRUE(language_c::parse("sr-ekavsk").is_valid());
  EXPECT_TRUE(language_c::parse("sr-Latn-ekavsk").is_valid());
  EXPECT_TRUE(language_c::parse("sr-Cyrl-ekavsk").is_valid());
}

TEST(BCP47LanguageTags, ParsingValidUNM49) {
  language_c::set_normalization_mode(norm_e::none);
  EXPECT_EQ("es-MX"s,  language_c::parse("es-484").format());
  EXPECT_EQ("es-419"s, language_c::parse("es-419").format());
}

TEST(BCP47LanguageTags, ParsingValidInRegistryButNotISOLists) {
  language_c::set_normalization_mode(norm_e::none);
  EXPECT_TRUE(language_c::parse("en-003").is_valid());
  EXPECT_TRUE(language_c::parse("en-BU").is_valid());
}

TEST(BCP47LanguageTags, ParsingInvalid) {
  language_c::set_normalization_mode(norm_e::none);
  EXPECT_FALSE(language_c::parse("zyx-Latn-CH-x-weeee").is_valid());  // invalid (zyx not ISO 639 code)
  EXPECT_FALSE(language_c::parse("ger-muku-CH-x-weeee").is_valid());  // invalid (muku not a script)
  EXPECT_FALSE(language_c::parse("ger-777").is_valid());              // invalid (777 not a region code)
  EXPECT_FALSE(language_c::parse("gonzo").is_valid());                // invalid
  EXPECT_FALSE(language_c::parse("de-aao-Latn-DZ").is_valid());       // invalid (aoo not valid with de)
  EXPECT_FALSE(language_c::parse("es-0").is_valid());                 // invalid (no such region)
}

TEST(BCP47LanguageTags, Formatting) {
  language_c::set_normalization_mode(norm_e::none);
  EXPECT_EQ(""s, language_c{}.format());

  language_c l;

  l.set_valid(true);
  l.set_language("de");
  l.set_script("latn");
  l.set_region("ch");
  l.set_private_use({ "weee"s });

  EXPECT_EQ("de-Latn-CH-x-weee"s, l.format());
  EXPECT_EQ("German (de-Latn-CH-x-weee)"s, l.format_long());

  l = language_c{};
  l.set_valid(true);
  l.set_language("DE");
  l.set_script("LATN");
  l.set_region("cH");
  l.set_private_use({ "WEEE"s });

  EXPECT_EQ("de-Latn-CH-x-weee"s, l.format());
  EXPECT_EQ("German (de-Latn-CH-x-weee)"s, l.format_long());

  l = language_c{};
  l.set_valid(true);
  l.set_private_use({ "weee"s, "wooo"s });

  EXPECT_EQ("x-weee-wooo"s, l.format());
  EXPECT_EQ("x-weee-wooo"s, l.format_long());

  l = language_c{};
  l.set_private_use({ "weee"s, "wooo"s });

  EXPECT_EQ(""s,            l.format());
  EXPECT_EQ(""s,            l.format(false));
  EXPECT_EQ("x-weee-wooo"s, l.format(true));

  EXPECT_EQ(""s,            l.format_long());
  EXPECT_EQ(""s,            l.format_long(false));
  EXPECT_EQ("x-weee-wooo"s, l.format_long(true));

  l = language_c{};
  l.set_language("ja");
  l.add_extension({ "t"s, { "test"s }});
  l.add_extension({ "u"s, { "attr"s, "co"s, "phonebk"s, "attr"s, "zz"s, "oooqqq"s }});
  l.set_valid(true);

  EXPECT_EQ("ja-t-test-u-attr-co-phonebk-attr-zz-oooqqq"s, l.format());

  l = language_c{};
  l.set_language("ja");
  l.add_extension({ "u"s, { "attr"s, "co"s, "phonebk"s, "attr"s, "zz"s, "oooqqq"s }});
  l.add_extension({ "t"s, { "test"s }});
  l.set_valid(true);

  EXPECT_EQ("ja-u-attr-co-phonebk-attr-zz-oooqqq-t-test"s, l.format());

  l.to_canonical_form();

  EXPECT_EQ("ja-t-test-u-attr-co-phonebk-attr-zz-oooqqq"s, l.format());
}

TEST(BCP47LanguageTags, FormattingInvalidWithoutLanguage) {
  language_c::set_normalization_mode(norm_e::none);
  auto l = language_c{};

  l.set_region("FR"s);
  l.set_private_use({ "moo"s });

  EXPECT_EQ(""s,          l.format());
  EXPECT_EQ("-FR-x-moo"s, l.format(true));
}

TEST(BCP47LanguageTags, CodeConversion) {
  language_c::set_normalization_mode(norm_e::none);
  EXPECT_EQ(""s,    language_c{}.get_iso639_alpha_3_code());
  EXPECT_EQ("ger"s, language_c::parse("de").get_iso639_alpha_3_code());
  EXPECT_EQ("ger"s, language_c::parse("deu").get_iso639_alpha_3_code());
  EXPECT_EQ("ger"s, language_c::parse("ger").get_iso639_alpha_3_code());

  language_c l;
  l.set_language("zyx");
  EXPECT_EQ(""s, l.get_iso639_alpha_3_code());

  EXPECT_FALSE(language_c{}.has_valid_iso639_code());
  EXPECT_FALSE(language_c::parse("zyx").has_valid_iso639_code());
  EXPECT_FALSE(language_c::parse("x-zyx").has_valid_iso639_code());

  EXPECT_TRUE(language_c::parse("de").has_valid_iso639_code());
  EXPECT_TRUE(language_c::parse("deu").has_valid_iso639_code());
  EXPECT_TRUE(language_c::parse("ger").has_valid_iso639_code());
}

TEST(BCP47LanguageTags, UnorderedMap) {
  language_c::set_normalization_mode(norm_e::none);
  std::unordered_map<language_c, int> m;

  m[language_c::parse("de-Latn")] = 42;
  EXPECT_EQ(42, m[language_c::parse("de-Latn")]);
  EXPECT_EQ(0,  m[language_c::parse("de")]);
}

TEST(BCP47LanguageTags, Clearing) {
  language_c::set_normalization_mode(norm_e::none);
  auto l = language_c::parse("eng");

  ASSERT_TRUE(l.has_valid_iso639_code());

  l.clear();

  EXPECT_FALSE(l.is_valid());
}

TEST(BCP47LanguageTags, EqualityOperators) {
  language_c::set_normalization_mode(norm_e::none);
  EXPECT_TRUE(language_c::parse("ger") == language_c::parse("ger"));
  EXPECT_TRUE(language_c::parse("ger") == language_c::parse("deu"));
  EXPECT_TRUE(language_c::parse("ger") == language_c::parse("de"));

  EXPECT_TRUE(language_c::parse("deu") == language_c::parse("ger"));
  EXPECT_TRUE(language_c::parse("de")  == language_c::parse("ger"));

  EXPECT_FALSE(language_c::parse("eng") == language_c::parse("ger"));
  EXPECT_FALSE(language_c::parse("eng") == language_c::parse("deu"));
  EXPECT_FALSE(language_c::parse("eng") == language_c::parse("de"));

  EXPECT_FALSE(language_c::parse("ger") == language_c::parse("eng"));
  EXPECT_FALSE(language_c::parse("deu") == language_c::parse("eng"));
  EXPECT_FALSE(language_c::parse("de")  == language_c::parse("eng"));

  EXPECT_FALSE(language_c::parse("ger") != language_c::parse("ger"));
  EXPECT_FALSE(language_c::parse("ger") != language_c::parse("deu"));
  EXPECT_FALSE(language_c::parse("ger") != language_c::parse("de"));

  EXPECT_FALSE(language_c::parse("deu") != language_c::parse("ger"));
  EXPECT_FALSE(language_c::parse("de")  != language_c::parse("ger"));

  EXPECT_TRUE(language_c::parse("eng") != language_c::parse("ger"));
  EXPECT_TRUE(language_c::parse("eng") != language_c::parse("deu"));
  EXPECT_TRUE(language_c::parse("eng") != language_c::parse("de"));

  EXPECT_TRUE(language_c::parse("ger") != language_c::parse("eng"));
  EXPECT_TRUE(language_c::parse("deu") != language_c::parse("eng"));
  EXPECT_TRUE(language_c::parse("de")  != language_c::parse("eng"));
}

TEST(BCP47LanguageTags, DifferenceBetweenISO639_2And639_3) {
  language_c::set_normalization_mode(norm_e::none);
  EXPECT_TRUE(language_c::parse("de").has_valid_iso639_code());
  EXPECT_TRUE(language_c::parse("de").has_valid_iso639_2_code());

  EXPECT_TRUE(language_c::parse("cmn").has_valid_iso639_code());
  EXPECT_FALSE(language_c::parse("cmn").has_valid_iso639_2_code());
}

TEST(BCP47LanguageTags, PrefixValidation) {
  language_c::set_normalization_mode(norm_e::none);
  EXPECT_TRUE(language_c::parse("de-CH-1996").is_valid());
  EXPECT_TRUE(language_c::parse("sr-Cyrl-ekavsk").is_valid());
  EXPECT_TRUE(language_c::parse("sr-Cyrl-SR-ekavsk").is_valid());
  EXPECT_TRUE(language_c::parse("en-GB-scotland").is_valid());
  EXPECT_TRUE(language_c::parse("zh-Latn-CN-pinyin").is_valid());

  EXPECT_TRUE(language_c::parse("sl-rozaj").is_valid());
  EXPECT_TRUE(language_c::parse("sl-rozaj-biske").is_valid());
  EXPECT_TRUE(language_c::parse("sl-rozaj-1994").is_valid());
  EXPECT_TRUE(language_c::parse("sl-rozaj-biske-1994").is_valid());

  EXPECT_TRUE(language_c::parse("de-1901").is_valid());
  EXPECT_TRUE(language_c::parse("de-1996").is_valid());

  EXPECT_TRUE(language_c::parse("zh-cmn").is_valid());
  EXPECT_TRUE(language_c::parse("zh-yue").is_valid());
  EXPECT_FALSE(language_c::parse("zh-cmn-yue").is_valid());

  EXPECT_TRUE(language_c::parse("hy-arevela").is_valid());
  EXPECT_TRUE(language_c::parse("hy-arevmda").is_valid());

  EXPECT_TRUE(language_c::parse("ja-Latn-hepburn").is_valid());
  EXPECT_TRUE(language_c::parse("ja-Latn-hepburn-heploc").is_valid());

  EXPECT_FALSE(language_c::parse("de-1996-1996").is_valid());
  EXPECT_TRUE(language_c::parse("cmn-Latn-pinyin").is_valid());
  EXPECT_TRUE(language_c::parse("zh-cmn-Latn-tongyong").is_valid());
  EXPECT_TRUE(language_c::parse("zh-yue-jyutping").is_valid());
}

TEST(BCP47LanguageTags, RFC4646AssortedValid) {
  language_c::set_normalization_mode(norm_e::none);
  EXPECT_TRUE(language_c::parse("de-1996").is_valid()); // section 3.1
  EXPECT_TRUE(language_c::parse("de-Latg-1996").is_valid()); // section 3.1
  EXPECT_TRUE(language_c::parse("de-CH-1996").is_valid()); // section 3.1
}

TEST(BCP47LanguageTags, RFC4646AppendixBValid) {
  language_c::set_normalization_mode(norm_e::none);
  // Simple language subtag:
  EXPECT_TRUE(language_c::parse("de").is_valid()); // (German)
  EXPECT_TRUE(language_c::parse("fr").is_valid()); // (French)
  EXPECT_TRUE(language_c::parse("ja").is_valid()); // (Japanese)
  // [MKVToolNix: deactivated as grandfathered entries are currently not supported]
  // EXPECT_TRUE(language_c::parse("i-enochian").is_valid()); // (example of a grandfathered tag)

  // Language subtag plus Script subtag:
  EXPECT_TRUE(language_c::parse("zh-Hant").is_valid()); // (Chinese written using the Traditional Chinese script)
  EXPECT_TRUE(language_c::parse("zh-Hans").is_valid()); // (Chinese written using the Simplified Chinese script)
  EXPECT_TRUE(language_c::parse("sr-Cyrl").is_valid()); // (Serbian written using the Cyrillic script)
  EXPECT_TRUE(language_c::parse("sr-Latn").is_valid()); // (Serbian written using the Latin script)

  // Language-Script-Region:
  EXPECT_TRUE(language_c::parse("zh-Hans-CN").is_valid()); // (Chinese written using the Simplified script as used in mainland China)
  EXPECT_TRUE(language_c::parse("sr-Latn-SR").is_valid()); // (Serbian written using the Latin script as used in Serbia) [MKVToolNix: original was CS = Serbian & Montenegro, but that country doesn't exist anymore, and neither does the code.]

  // Language-Variant:
  EXPECT_TRUE(language_c::parse("sl-rozaj").is_valid()); // (Resian dialect of Slovenian
  EXPECT_TRUE(language_c::parse("sl-nedis").is_valid()); // (Nadiza dialect of Slovenian)

  // Language-Region-Variant:
  EXPECT_TRUE(language_c::parse("de-CH-1901").is_valid()); // (German as used in Switzerland using the 1901 variant [orthography])
  EXPECT_TRUE(language_c::parse("sl-IT-nedis").is_valid()); // (Slovenian as used in Italy, Nadiza dialect)

  // Language-Script-Region-Variant:
  EXPECT_TRUE(language_c::parse("sl-Latn-IT-nedis").is_valid()); // (Nadiza dialect of Slovenian written using the Latin script as used in Italy.  Note that this tag is NOT RECOMMENDED because subtag 'sl' has a Suppress-Script value of 'Latn')

  // Language-Region:
  EXPECT_TRUE(language_c::parse("de-DE").is_valid()); // (German for Germany)
  EXPECT_TRUE(language_c::parse("en-US").is_valid()); // (English as used in the United States)
  EXPECT_TRUE(language_c::parse("es-419").is_valid()); // (Spanish appropriate for the Latin America and Caribbean region using the UN region code)

  // Private use subtags:
  EXPECT_TRUE(language_c::parse("de-CH-x-phonebk").is_valid());
  EXPECT_TRUE(language_c::parse("az-Arab-x-AZE-derbend").is_valid());

  // Extended language subtags (examples ONLY: extended languages MUST be defined by revision or update to this document):
  EXPECT_TRUE(language_c::parse("zh-mnp").is_valid());
  EXPECT_FALSE(language_c::parse("zh-mnp-nan-Hant-CN").is_valid()); // invalid as 'nan' must only be used with prefix 'zh'

  // Private use registry values:
  EXPECT_TRUE(language_c::parse("x-whatever").is_valid()); // (private use using the singleton 'x')
  EXPECT_TRUE(language_c::parse("qaa-Qaaa-QM-x-southern").is_valid()); // (all private tags)
  EXPECT_TRUE(language_c::parse("de-Qaaa").is_valid()); // (German, with a private script)
  EXPECT_TRUE(language_c::parse("sr-Latn-QM").is_valid()); // (Serbian, Latin-script, private region)
  EXPECT_TRUE(language_c::parse("sr-Qaaa-SR").is_valid()); // (Serbian, private script, for Serbia) [MKVToolNix: original used CS = Sebia & Montenegro, a country that doesn't exist anymore]

  // Tags that use extensions (examples ONLY: extensions MUST be defined by revision or update to this document or by RFC):
  // EXPECT_TRUE(language_c::parse("en-US-u-islamCal").is_valid());
  // EXPECT_TRUE(language_c::parse("zh-CN-a-myExt-x-private").is_valid());
  // EXPECT_TRUE(language_c::parse("en-a-myExt-b-another").is_valid());
}

TEST(BCP47LanguageTags, RFC4646AppendixBInvalid) {
  language_c::set_normalization_mode(norm_e::none);
  EXPECT_FALSE(language_c::parse("de-419-DE").is_valid()); // (two region tags)
  EXPECT_FALSE(language_c::parse("a-DE").is_valid()); // (use of a single-character subtag in primary position; note that there are a few grandfathered tags that start with "i-" that are valid)
  EXPECT_FALSE(language_c::parse("ar-a-aaa-b-bbb-a-ccc").is_valid()); // (two extensions with same single-letter prefix)
}

TEST(BCP47LanguageTags, OnlyCertainScriptsAllowedOrNoScriptAtAll) {
  language_c::set_normalization_mode(norm_e::none);
  EXPECT_TRUE(language_c::parse("sr-Bali").is_valid());
  EXPECT_TRUE(language_c::parse("sr-Cyrl").is_valid());
  EXPECT_TRUE(language_c::parse("sr-Latn").is_valid());

  EXPECT_TRUE(language_c::parse("sr-ekavsk").is_valid());
  EXPECT_TRUE(language_c::parse("sr-Cyrl-ekavsk").is_valid());
  EXPECT_TRUE(language_c::parse("sr-Latn-ekavsk").is_valid());

  EXPECT_TRUE(language_c::parse("sr-Latn-RS-ekavsk").is_valid());
}

TEST(BCP47LanguageTags, ExtensionsBasics) {
  language_c::set_normalization_mode(norm_e::none);
  EXPECT_TRUE(language_c::parse("ja-t-test").is_valid());
  EXPECT_TRUE(language_c::parse("ja-t-abcdefgh").is_valid());
  EXPECT_TRUE(language_c::parse("ja-t-test-u-attr-co-phonebk").is_valid());
  EXPECT_TRUE(language_c::parse("ja-t-test-u-attr-co-phonebk-attr-zz-oooqqq").is_valid());
  EXPECT_TRUE(language_c::parse("ja-u-attr-co-phonebk-t-test").is_valid());

  EXPECT_FALSE(language_c::parse("ja-t").is_valid());                                  // Nothing following the singleton
  EXPECT_FALSE(language_c::parse("ja-t-u-attr-co-phonebk").is_valid());                // No extension subtag within the t extension
  EXPECT_FALSE(language_c::parse("ja-t-u-attr-co-phonebk").is_valid());                // No extension subtag within the t extension
  EXPECT_FALSE(language_c::parse("ja-u-attr-co-phonebk-u-attr-zz-oooqqq").is_valid()); // Singleton occurring multiple times
  EXPECT_FALSE(language_c::parse("ja-t-z").is_valid());                                // Extension subtag too short
  EXPECT_FALSE(language_c::parse("ja-t-abcdefghi").is_valid());                        // Extension subtag too long

  EXPECT_FALSE(language_c::parse("ja-a-moo-cow").is_valid());                          // Singleton a is not registered at the moment
}

TEST(BCP47LanguageTags, ExtensionsRFC6067) {
  language_c::set_normalization_mode(norm_e::none);
  EXPECT_TRUE(language_c::parse("de-DE-u-attr-co-phonebk").is_valid());
}

TEST(BCP47LanguageTags, ExtensionsRFC6497) {
  language_c::set_normalization_mode(norm_e::none);
  EXPECT_TRUE(language_c::parse("und-Cyrl-t-und-latn-m0-ungegn-2007").is_valid());
  EXPECT_TRUE(language_c::parse("und-Hebr-t-und-latn-m0-ungegn-1972").is_valid());
  EXPECT_TRUE(language_c::parse("ja-t-it-m0-xxx-v21a-2007").is_valid());
}

TEST(BCP47LanguageTags, ExtensionsFormatting) {
  language_c::set_normalization_mode(norm_e::none);
  EXPECT_EQ("ja-t-test-u-attr-co-phonebk"s, language_c::parse("ja-T-Test-U-AttR-CO-phoNEbk").format());
  EXPECT_EQ("ja-u-attr-co-phonebk-t-test"s, language_c::parse("ja-U-AttR-CO-phoNEbk-T-Test").format());
}

TEST(BCP47LanguageTags, Matching) {
  language_c::set_normalization_mode(norm_e::none);
  EXPECT_FALSE(language_c{}                  .matches(language_c{}));
  EXPECT_FALSE(language_c{}                  .matches(language_c::parse("es")));
  EXPECT_FALSE(language_c::parse("es")       .matches(language_c{}));

  EXPECT_TRUE(language_c::parse("es")        .matches(language_c::parse("es")));
  EXPECT_TRUE(language_c::parse("es-MX")     .matches(language_c::parse("es")));
  EXPECT_TRUE(language_c::parse("es-Latn-MX").matches(language_c::parse("es")));

  EXPECT_TRUE(language_c::parse("es-MX")     .matches(language_c::parse("es-MX")));
  EXPECT_TRUE(language_c::parse("es-Latn-MX").matches(language_c::parse("es-Latn-MX")));

  EXPECT_TRUE(language_c::parse("es-Latn-MX").matches(language_c::parse("es-MX")));

  EXPECT_FALSE(language_c::parse("es")       .matches(language_c::parse("es-MX")));
  EXPECT_FALSE(language_c::parse("es")       .matches(language_c::parse("es-Latn-MX")));
}

TEST(BCP47LanguageTags, FindBestMatch) {
  language_c::set_normalization_mode(norm_e::none);
  using V = std::vector<language_c>;

  EXPECT_FALSE(language_c{}           .find_best_match({}).is_valid());
  EXPECT_FALSE(language_c::parse("es").find_best_match({}).is_valid());
  EXPECT_FALSE(language_c{}           .find_best_match(V{ language_c{} }).is_valid() );
  EXPECT_FALSE(language_c{}           .find_best_match(V{ language_c::parse("es") }).is_valid() );

  EXPECT_FALSE(language_c::parse("es")   .find_best_match(V{ language_c::parse("de"),    language_c::parse("fr") }).is_valid());
  EXPECT_FALSE(language_c::parse("es")   .find_best_match(V{ language_c::parse("es-US"), language_c::parse("es-ES") }).is_valid());
  EXPECT_FALSE(language_c::parse("es-MX").find_best_match(V{ language_c::parse("es-US"), language_c::parse("es-ES") }).is_valid());

  EXPECT_EQ(language_c::parse("es"),      language_c::parse("es")        .find_best_match(V{ language_c::parse("es") }));
  EXPECT_EQ(language_c::parse("es"),      language_c::parse("es")        .find_best_match(V{ language_c::parse("de"),      language_c::parse("es") }));
  EXPECT_EQ(language_c::parse("es"),      language_c::parse("es-Latn-MX").find_best_match(V{ language_c::parse("de"),      language_c::parse("es"), language_c::parse("fr") }));
  EXPECT_EQ(language_c::parse("es"),      language_c::parse("es-Latn-MX").find_best_match(V{ language_c::parse("es-US"),   language_c::parse("es"), language_c::parse("es-ES") }));
  EXPECT_EQ(language_c::parse("es-MX"),   language_c::parse("es-Latn-MX").find_best_match(V{ language_c::parse("es-US"),   language_c::parse("es"), language_c::parse("es-MX") }));
  EXPECT_EQ(language_c::parse("es-Latn"), language_c::parse("es-Latn-MX").find_best_match(V{ language_c::parse("es-Latn"), language_c::parse("es"), language_c::parse("es-MX") }));
}

TEST(BCP47LanguageTags, ISO3166_1_Alpha2Codes) {
  language_c::set_normalization_mode(norm_e::none);
  EXPECT_FALSE(language_c::parse("es").has_valid_iso3166_1_alpha_2_or_top_level_domain_country_code());
  EXPECT_FALSE(language_c::parse("es-029").has_valid_iso3166_1_alpha_2_or_top_level_domain_country_code());
  EXPECT_FALSE(language_c::parse("es-AA").has_valid_iso3166_1_alpha_2_or_top_level_domain_country_code());
  EXPECT_FALSE(language_c::parse("es-QT").has_valid_iso3166_1_alpha_2_or_top_level_domain_country_code());
  EXPECT_FALSE(language_c::parse("es-XS").has_valid_iso3166_1_alpha_2_or_top_level_domain_country_code());
  EXPECT_TRUE(language_c::parse("es-ES").has_valid_iso3166_1_alpha_2_or_top_level_domain_country_code());
  EXPECT_TRUE(language_c::parse("es-724").has_valid_iso3166_1_alpha_2_or_top_level_domain_country_code());

  EXPECT_EQ(""s, language_c::parse("es").get_iso3166_1_alpha_2_code());
  EXPECT_EQ(""s, language_c::parse("es-029").get_iso3166_1_alpha_2_code());
  EXPECT_EQ("ES"s, language_c::parse("es-ES").get_iso3166_1_alpha_2_code());
  EXPECT_EQ("ES"s, language_c::parse("es-724").get_iso3166_1_alpha_2_code());
  EXPECT_EQ("es"s, language_c::parse("es-ES").get_top_level_domain_country_code());
  EXPECT_EQ("es"s, language_c::parse("es-724").get_top_level_domain_country_code());

  EXPECT_EQ("GB"s, language_c::parse("en-GB").get_iso3166_1_alpha_2_code());
  EXPECT_EQ("uk"s, language_c::parse("en-GB").get_top_level_domain_country_code());
}

TEST(BCP47LanguageTags, ClosestISO639_2_Alpha3Code) {
  language_c::set_normalization_mode(norm_e::none);
  // default value returned in different cases
  EXPECT_EQ("und"s, language_c{}.get_closest_iso639_2_alpha_3_code());                          // empty entry
  EXPECT_EQ("und"s, language_c::parse("moocow").get_closest_iso639_2_alpha_3_code());           // invalid entry
  EXPECT_EQ("und"s, language_c::parse("x-muh-to-the-kuh").get_closest_iso639_2_alpha_3_code()); // valid but no ISO 639 code
  EXPECT_EQ("und"s, language_c::parse("aiw").get_closest_iso639_2_alpha_3_code());              // valid but no 639-2 code & not an extlang

  // "Valid, is extlang, prefix is ISO 639 code but not ISO 639-2"
  // would be another case when "und" should be returned, but as of
  // 2022-03-23 there's no such entry. All current prefixes for
  // extlangs do have ISO 639-2 codes.

  // Now some valid cases.
  EXPECT_EQ("fre"s, language_c::parse("fr-FR").get_closest_iso639_2_alpha_3_code());
  EXPECT_EQ("ger"s, language_c::parse("de").get_closest_iso639_2_alpha_3_code());
  EXPECT_EQ("ger"s, language_c::parse("deu").get_closest_iso639_2_alpha_3_code());
  EXPECT_EQ("ger"s, language_c::parse("ger").get_closest_iso639_2_alpha_3_code());

  // Last the interesting cases: `yue` = "Yue Chinese" doesn't have an
  // ISO 639-2 code, but it is an extlang with a prefix of `zh` =
  // "Chinese" for which there is an ISO 639-2 code: `chi`. Similarly
  // for the other two examples (`bsi` = "British Sign Language" → `sgn` = "Sign
  // Languages"; `zsm` = "Standard Malay" → `may` = "Malay (macrolanguage)")
  EXPECT_EQ("chi"s, language_c::parse("yue").get_closest_iso639_2_alpha_3_code());
  EXPECT_EQ("sgn"s, language_c::parse("bfi").get_closest_iso639_2_alpha_3_code());
  EXPECT_EQ("may"s, language_c::parse("zsm").get_closest_iso639_2_alpha_3_code());
}

TEST(BCP47LanguageTags, Grandfathered) {
  language_c::set_normalization_mode(norm_e::none);
  EXPECT_TRUE(language_c::parse("i-klingon").is_valid());
  EXPECT_TRUE(language_c::parse("i-KLiNGoN").is_valid());
  EXPECT_TRUE(language_c::parse("no-NyN").is_valid());
  EXPECT_TRUE(language_c::parse("sGn-be-FR").is_valid());

  EXPECT_EQ("i-klingon"s, language_c::parse("i-klingon").format());
  EXPECT_EQ("i-klingon"s, language_c::parse("i-KLiNGoN").format());
  EXPECT_EQ("no-nyn"s, language_c::parse("no-NyN").format());
  EXPECT_EQ("sgn-BE-FR"s, language_c::parse("sGn-be-FR").format());

  auto l = language_c{};
  l.set_valid(true);
  l.set_language("DE");
  l.set_grandfathered("i-KLINGON");

  EXPECT_EQ("i-klingon"s, l.format());
  EXPECT_EQ("i-KLINGON"s, l.get_grandfathered());
}

TEST(BCP47LanguageTags, ToCanonicalForm) {
  language_c::set_normalization_mode(norm_e::none);
  // No changes as they're already normalized.
  EXPECT_EQ("sgn"s, language_c::parse("sgn"s).to_canonical_form().format());
  EXPECT_EQ("nsi"s, language_c::parse("nsi"s).to_canonical_form().format());

  // No changes as even though they're listed as redundant, they don't have preferred values.
  EXPECT_EQ("az-Arab"s, language_c::parse("az-Arab"s).to_canonical_form().format());

  // For the following there are changes.
  EXPECT_EQ("nsi"s,             language_c::parse("sgn-nsi"s).to_canonical_form().format());
  EXPECT_EQ("ja-Latn-alalc97"s, language_c::parse("ja-Latn-hepburn-heploc"s).to_canonical_form().format());
  EXPECT_EQ("jbo"s,             language_c::parse("art-lojban"s).to_canonical_form().format());
  EXPECT_EQ("jsl"s,             language_c::parse("sgn-JP"s).to_canonical_form().format());
  EXPECT_EQ("cmn"s,             language_c::parse("zh-cmn"s).to_canonical_form().format());
  EXPECT_EQ("cmn-CN"s,          language_c::parse("zh-cmn-CN"s).to_canonical_form().format());
  EXPECT_EQ("cmn-Hans"s,        language_c::parse("zh-cmn-Hans"s).to_canonical_form().format());
  EXPECT_EQ("cmn"s,             language_c::parse("zh-guoyu"s).to_canonical_form().format());
  EXPECT_EQ("hak"s,             language_c::parse("zh-hakka"s).to_canonical_form().format());
  EXPECT_EQ("hak"s,             language_c::parse("i-hak"s).to_canonical_form().format());
  EXPECT_EQ("yue-jyutping"s,    language_c::parse("zh-yue-jyutping"s).to_canonical_form().format());
  EXPECT_EQ("zh-MM"s,           language_c::parse("zh-BU"s).to_canonical_form().format());
  EXPECT_EQ("cmn-MM"s,          language_c::parse("zh-cmn-BU"s).to_canonical_form().format());
}

TEST(BCP47LanguageTags, ToExtlangForm) {
  language_c::set_normalization_mode(norm_e::none);
  // No changes as they're already normalized.
  EXPECT_EQ("sgn"s, language_c::parse("sgn"s).to_extlang_form().format());

  // No changes as even though they're listed as redundant, they don't have preferred values.
  EXPECT_EQ("az-Arab"s, language_c::parse("az-Arab"s).to_extlang_form().format());

  // For the following there are changes.
  EXPECT_EQ("sgn-nsi"s,         language_c::parse("nsi"s).to_extlang_form().format());
  EXPECT_EQ("jbo"s,             language_c::parse("jbo"s).to_extlang_form().format());
  EXPECT_EQ("zh-yue-jyutping"s, language_c::parse("yue-jyutping"s).to_extlang_form().format());
}

TEST(BCP47LanguageTags, Cloning) {
  language_c::set_normalization_mode(norm_e::none);
  auto l  = language_c::parse("de-DE-1996");
  auto l2 = l.clone();

  EXPECT_TRUE(l.is_valid());
  EXPECT_TRUE(l2.is_valid());
  EXPECT_TRUE(l == l2);
  EXPECT_TRUE(l.format() == l2.format());

  l.set_region("CH");

  EXPECT_TRUE(l.is_valid());
  EXPECT_TRUE(l2.is_valid());
  EXPECT_FALSE(l == l2);
  EXPECT_FALSE(l.format() == l2.format());
}

TEST(BCP47LanguageTags, NormalizationDuringParsing) {
  language_c::set_normalization_mode(norm_e::none);

  EXPECT_EQ("nsi"s,             language_c::parse("nsi"s,                    norm_e::canonical).format());
  EXPECT_EQ("jbo"s,             language_c::parse("jbo"s,                    norm_e::canonical).format());
  EXPECT_EQ("yue-jyutping"s,    language_c::parse("yue-jyutping"s,           norm_e::canonical).format());

  EXPECT_EQ("nsi"s,             language_c::parse("nsi"s,                    norm_e::none).format());
  EXPECT_EQ("jbo"s,             language_c::parse("jbo"s,                    norm_e::none).format());
  EXPECT_EQ("yue-jyutping"s,    language_c::parse("yue-jyutping"s,           norm_e::none).format());

  EXPECT_EQ("sgn-nsi"s,         language_c::parse("nsi"s,                    norm_e::extlang).format());
  EXPECT_EQ("jbo"s,             language_c::parse("jbo"s,                    norm_e::extlang).format());
  EXPECT_EQ("zh-yue-jyutping"s, language_c::parse("yue-jyutping"s,           norm_e::extlang).format());

  EXPECT_EQ("sgn-nsi"s,         language_c::parse("nsi"s,                    norm_e::extlang).format());
  EXPECT_EQ("jbo"s,             language_c::parse("jbo"s,                    norm_e::extlang).format());
  EXPECT_EQ("zh-yue-jyutping"s, language_c::parse("yue-jyutping"s,           norm_e::extlang).format());

  EXPECT_EQ("nsi"s,             language_c::parse("sgn-nsi"s,                norm_e::canonical).format());
  EXPECT_EQ("ja-Latn-alalc97"s, language_c::parse("ja-Latn-hepburn-heploc"s, norm_e::canonical).format());

  EXPECT_EQ("nsi"s,             language_c::parse("nsi"s).format());
  EXPECT_EQ("jbo"s,             language_c::parse("jbo"s).format());
  EXPECT_EQ("yue-jyutping"s,    language_c::parse("yue-jyutping"s).format());

  language_c::set_normalization_mode(norm_e::extlang);

  EXPECT_EQ("sgn-nsi"s,         language_c::parse("nsi"s).format());
  EXPECT_EQ("jbo"s,             language_c::parse("jbo"s).format());
  EXPECT_EQ("zh-yue-jyutping"s, language_c::parse("yue-jyutping"s).format());

  language_c::set_normalization_mode(norm_e::none);
}

TEST(BCP47LanguageTags, NormalizationForDCNCTags) {
  language_c::set_normalization_mode(norm_e::canonical);

  EXPECT_EQ("cmn-Hans"s,    language_c::parse("QMS"s).format());
  EXPECT_EQ("cmn-Hant-CN"s, language_c::parse("QMT-CN"s).format());
  EXPECT_EQ("pt-BR"s,       language_c::parse("QBP"s).format());
}

}
