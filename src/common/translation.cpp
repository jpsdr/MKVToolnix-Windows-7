/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   translation, locale handling

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include <clocale>
#include <codecvt>
#include <cstdlib>
#if HAVE_NL_LANGINFO
# include <langinfo.h>
#elif HAVE_LOCALE_CHARSET
# include <libcharset.h>
#endif
#include <locale>

#include "common/fs_sys_helpers.h"
#include "common/iso639.h"
#include "common/locale_string.h"
#include "common/qt.h"
#include "common/strings/editing.h"
#include "common/strings/formatting.h"
#include "common/strings/utf8.h"
#include "common/translation.h"

#if defined(SYS_WINDOWS)
# include <windows.h>
# include <winnls.h>

# include "common/memory.h"
#endif

std::vector<translation_c> translation_c::ms_available_translations;
int translation_c::ms_active_translation_idx = 0;
std::string translation_c::ms_default_iso639_ui_language;

translation_c::translation_c(std::string iso639_alpha_3_code,
                             std::string unix_locale,
                             std::string windows_locale,
                             std::string windows_locale_sysname,
                             std::string english_name,
                             std::string translated_name,
                             bool line_breaks_anywhere,
                             int language_id,
                             int sub_language_id)
  : m_iso639_alpha_3_code{std::move(iso639_alpha_3_code)}
  , m_unix_locale{std::move(unix_locale)}
  , m_windows_locale{std::move(windows_locale)}
  , m_windows_locale_sysname{std::move(windows_locale_sysname)}
  , m_english_name{std::move(english_name)}
  , m_translated_name{std::move(translated_name)}
  , m_line_breaks_anywhere{line_breaks_anywhere}
  , m_language_id{language_id}
  , m_sub_language_id{sub_language_id}
{
}

// See http://docs.translatehouse.org/projects/localization-guide/en/latest/guide/win_lang_ids.html for the (sub) language IDs.

void
translation_c::initialize_available_translations() {
  ms_available_translations.clear();
  ms_available_translations.emplace_back("eng", "en_US",       "en",          "english",    "English",                        "English",             false, 0x0009, 0x00);
#if defined(HAVE_LIBINTL_H)
//ms_available_translations.emplace_back("are", "ar_AE",       "ar",          "arabic",     "Arabic",                         "اَلْعَرَبِيَّةُ",             false, 0x0001, 0x0e);
  ms_available_translations.emplace_back("baq", "eu_ES",       "eu",          "basque",     "Basque",                         "Euskara",             false, 0x002d, 0x00);
  ms_available_translations.emplace_back("bel", "be_BY",       "be",          "belarusian", "Belarusian",                     "беларуская",          false, 0x0023, 0x00);
  ms_available_translations.emplace_back("bul", "bg_BG",       "bg",          "bulgarian",  "Bulgarian",                      "Български",           false, 0x0002, 0x01);
  ms_available_translations.emplace_back("cat", "ca_ES",       "ca",          "catalan",    "Catalan",                        "Català",              false, 0x0003, 0x00);
  ms_available_translations.emplace_back("chi", "zh_CN",       "zh_CN",       "chinese",    "Chinese (Simplified)",           "中文 (简体)",         true,  0x0004, 0x02);
  ms_available_translations.emplace_back("chi", "zh_SG",       "zh_SG",       "chinese",    "Chinese (Singapore & Malaysia)", "中文 (新马简体)",     true,  0x1004, 0x04);
  ms_available_translations.emplace_back("chi", "zh_TW",       "zh_TW",       "chinese",    "Chinese (Traditional)",          "中文 (繁體)",         true,  0x7C04, 0x01);
  ms_available_translations.emplace_back("cze", "cs_CZ",       "cs",          "czech",      "Czech",                          "Čeština",             false, 0x0005, 0x00);
  ms_available_translations.emplace_back("dut", "nl_NL",       "nl",          "dutch",      "Dutch",                          "Nederlands",          false, 0x0013, 0x00);
  ms_available_translations.emplace_back("fre", "fr_FR",       "fr",          "french",     "French",                         "Français",            false, 0x000c, 0x00);
  ms_available_translations.emplace_back("ger", "de_DE",       "de",          "german",     "German",                         "Deutsch",             false, 0x0007, 0x00);
//ms_available_translations.emplace_back("hin", "hi_IN",       "hi_IN",       "hindi",      "Hindi",                          "हिन्दी",               false, 0x0039, 0x00);
//ms_available_translations.emplace_back("hrv", "hr_HR",       "hr",          "croatian",   "Croatian",                       "Hrvatski",            false, 0x001a, 0x00);
  ms_available_translations.emplace_back("hun", "hu_HU",       "hu",          "hungarian",  "Hungarian",                      "Magyar",              false, 0x000e, 0x00);
  ms_available_translations.emplace_back("ita", "it_IT",       "it",          "italian",    "Italian",                        "Italiano",            false, 0x0010, 0x00);
  ms_available_translations.emplace_back("jpn", "ja_JP",       "ja",          "japanese",   "Japanese",                       "日本語",              true,  0x0011, 0x00);
  ms_available_translations.emplace_back("kor", "ko_KR",       "ko",          "korean",     "Korean",                         "한국어/조선말",       true,  0x0012, 0x01);
  ms_available_translations.emplace_back("lit", "lt_LT",       "lt",          "lithuanian", "Lithuanian",                     "Lietuvių",            false, 0x0027, 0x00);
  ms_available_translations.emplace_back("nor", "nb_NO",       "nb",          "norwegian",  "Norwegian",                      "Norsk Bokmål",        false, 0x0027, 0x00);
//ms_available_translations.emplace_back("per", "fa_IR",       "fa",          "persian",    "Persian",                        "فارسی",               false, 0x0029, 0x00);
  ms_available_translations.emplace_back("pol", "pl_PL",       "pl",          "polish",     "Polish",                         "Polski",              false, 0x0015, 0x00);
  ms_available_translations.emplace_back("por", "pt_BR",       "pt_BR",       "portuguese", "Portuguese (Brazil)",            "Português do Brasil", false, 0x0016, 0x01);
  ms_available_translations.emplace_back("por", "pt_PT",       "pt",          "portuguese", "Portuguese",                     "Português",           false, 0x0016, 0x02);
  ms_available_translations.emplace_back("rum", "ro_RO",       "ro",          "romanian",   "Romanian",                       "Română",              false, 0x0018, 0x00);
  ms_available_translations.emplace_back("rus", "ru_RU",       "ru",          "russian",    "Russian",                        "Русский",             false, 0x0019, 0x00);
  ms_available_translations.emplace_back("spa", "es_ES",       "es",          "spanish",    "Spanish",                        "Español",             false, 0x000a, 0x00);
  ms_available_translations.emplace_back("srp", "sr_RS",       "sr_RS",       "serbian",    "Serbian Cyrillic",               "Српски",              false, 0x001a, 0x03);
  ms_available_translations.emplace_back("srp", "sr_RS@latin", "sr_RS@latin", "serbian",    "Serbian Latin",                  "Srpski",              false, 0x001a, 0x02);
  ms_available_translations.emplace_back("swe", "sv_SE",       "sv",          "swedish",    "Swedish",                        "Svenska",             false, 0x001d, 0x01);
  ms_available_translations.emplace_back("tur", "tr_TR",       "tr",          "turkish",    "Turkish",                        "Türkçe",              false, 0x001f, 0x00);
  ms_available_translations.emplace_back("ukr", "uk_UA",       "uk",          "ukrainian",  "Ukrainian",                      "Українська",          false, 0x0022, 0x00);
//ms_available_translations.emplace_back("vie", "vi_VN",       "vi",          "vietnamese", "Vietnamese",                     "Tiếng Việt",          false, 0x002a, 0x00);
#endif

  ms_active_translation_idx = 0;
}

int
translation_c::look_up_translation(const std::string &locale) {
  try {
    auto hits = std::vector< std::pair<int, int> >{};
    auto full = locale_string_c(locale).str(locale_string_c::full);
    auto half = locale_string_c(locale).str(locale_string_c::half);

    for (auto beg = ms_available_translations.begin(), itr = ms_available_translations.begin(), end = ms_available_translations.end(); itr != end; ++itr) {
      auto score = itr->matches(full) ? 2
                 : itr->matches(half) ? 1
                 :                      0;

      if (score)
        hits.emplace_back(std::make_pair(score, static_cast<int>(std::distance(beg, itr))));
    }

    if (!hits.empty()) {
      std::sort(hits.begin(), hits.end());
      return hits.back().second;
    }

  } catch (mtx::locale_string_format_x &) {
  }

  return -1;
}

int
translation_c::look_up_translation(int language_id, int sub_language_id) {
  auto ptr = std::find_if(ms_available_translations.begin(), ms_available_translations.end(), [language_id,sub_language_id](translation_c const &tr) {
      return (tr.m_language_id == language_id) && (!tr.m_sub_language_id || (tr.m_sub_language_id == sub_language_id));
    });

  int idx = ptr == ms_available_translations.end() ? -1 : std::distance(ms_available_translations.begin(), ptr);
  mxdebug_if(debugging_c::requested("locale"), fmt::format("look_up_translation for 0x{0:04x}/0x{1:02x}: {2}\n", language_id, sub_language_id, idx));

  return idx;
}

void
translation_c::determine_default_iso639_ui_language() {
  std::string language_code;

#if defined(SYS_WINDOWS)
  auto language_id = GetUserDefaultUILanguage();
  auto length      = GetLocaleInfoW(language_id, LOCALE_SISO639LANGNAME2, NULL, 0);

  if (length <= 0)
    return;

  std::wstring language_code_wide(length, L' ');
  GetLocaleInfoW(language_id, LOCALE_SISO639LANGNAME2, &language_code_wide[0], length);
  language_code_wide.resize(length - 1);

  language_code = to_utf8(language_code_wide);

#else   // SYS_WINDOWS

  auto data = setlocale(LC_MESSAGES, nullptr);
  std::string previous_locale;

  if (data)
    previous_locale = data;

  setlocale(LC_MESSAGES, "");
  data = setlocale(LC_MESSAGES, nullptr);

  if (data)
    language_code = to_utf8(Q(data).replace(QRegularExpression{"_.*"}, ""));

  setlocale(LC_MESSAGES, previous_locale.c_str());

#endif  // SYS_WINDOWS

  auto language = mtx::iso639::look_up(language_code);
  if (language)
    ms_default_iso639_ui_language = language->alpha_3_code;
}

std::string
translation_c::get_default_ui_locale() {
  std::string locale;

#if defined(HAVE_LIBINTL_H)
  bool debug = debugging_c::requested("locale");

# if defined(SYS_WINDOWS)
  std::string env_var = mtx::sys::get_environment_variable("LC_MESSAGES");
  if (!env_var.empty() && (-1 != look_up_translation(env_var)))
    return env_var;

  env_var = mtx::sys::get_environment_variable("LANG");
  if (!env_var.empty() && (-1 != look_up_translation(env_var)))
    return env_var;

  auto lang_id = GetUserDefaultUILanguage();
  int idx      = translation_c::look_up_translation(lang_id & 0x3ff, (lang_id >> 10) & 0x3f);
  if (-1 != idx)
    locale = ms_available_translations[idx].get_locale();

  mxdebug_if(debug, fmt::format("[lang_id {0:04x} idx {1} locale {2}]\n", lang_id, idx, locale));

# else  // SYS_WINDOWS

  char *data = setlocale(LC_MESSAGES, nullptr);
  if (data) {
    std::string previous_locale = data;
    mxdebug_if(debug, fmt::format("[get_default_ui_locale previous {0}]\n", previous_locale));
    setlocale(LC_MESSAGES, "");
    data = setlocale(LC_MESSAGES, nullptr);

    if (data)
      locale = data;

    mxdebug_if(debug, fmt::format("[get_default_ui_locale new {0}]\n", locale));

    setlocale(LC_MESSAGES, previous_locale.c_str());
  } else
    mxdebug_if(debug, fmt::format("[get_default_ui_locale get previous failed]\n"));

# endif // SYS_WINDOWS
#endif  // HAVE_LIBINTL_H

  return locale;
}

std::string
translation_c::get_locale()
  const {
#if defined(SYS_WINDOWS)
  return m_windows_locale;
#else
  return m_unix_locale;
#endif
}

bool
translation_c::matches(std::string const &locale)
  const {
  if (balg::iequals(locale, get_locale()))
    return true;

#if defined(SYS_WINDOWS)
  if (balg::iequals(locale, m_windows_locale_sysname))
    return true;
#endif

  return false;
}

translation_c &
translation_c::get_active_translation() {
  return ms_available_translations[ms_active_translation_idx];
}

void
translation_c::set_active_translation(const std::string &locale) {
  int idx                   = look_up_translation(locale);
  ms_active_translation_idx = std::max(idx, 0);

  mxdebug_if(debugging_c::requested("locale"), fmt::format("[translation_c::set_active_translation() active_translation_idx {0} for locale {1}]\n", ms_active_translation_idx, locale));
}

// ------------------------------------------------------------

translatable_string_c::translatable_string_c(const std::string &untranslated_string)
  : m_untranslated_strings{untranslated_string}
{
}

translatable_string_c::translatable_string_c(const char *untranslated_string)
  : m_untranslated_strings{std::string{untranslated_string}}
{
}

translatable_string_c::translatable_string_c(std::vector<translatable_string_c> const &untranslated_strings)
{
  for (auto const &untranslated_string : untranslated_strings)
    m_untranslated_strings.emplace_back(untranslated_string.get_untranslated());
}

std::string
translatable_string_c::get_translated()
  const
{
  if (m_overridden_by)
    return *m_overridden_by;

  std::vector<std::string> translated_strings;
  for (auto const &untranslated_string : m_untranslated_strings)
    if (!untranslated_string.empty())
      translated_strings.emplace_back(gettext(untranslated_string.c_str()));

  return join(translated_strings);
}

std::string
translatable_string_c::get_untranslated()
  const
{
  return join(m_untranslated_strings);
}

translatable_string_c &
translatable_string_c::override(std::string const &by) {
  m_overridden_by = by;
  return *this;
}

std::string
translatable_string_c::join(std::vector<std::string> const &strings)
  const {
  auto separator = translation_c::get_active_translation().m_line_breaks_anywhere ? "" : " ";
  return mtx::string::join(strings, separator);
}

// ------------------------------------------------------------

#if defined(HAVE_LIBINTL_H)

void
init_locales(std::string locale) {
  auto debug = debugging_c::requested("locale");

  translation_c::determine_default_iso639_ui_language();
  translation_c::initialize_available_translations();

  mxdebug_if(debug, fmt::format("[init_locales start: locale {0} default_iso639_ui_language {1}]\n", locale, translation_c::ms_default_iso639_ui_language));

  std::string locale_dir;
  std::string default_locale = translation_c::get_default_ui_locale();

  if (-1 == translation_c::look_up_translation(locale)) {
    mxdebug_if(debug, fmt::format("[init_locales lookup failed; clearing locale]\n"));
    locale = "";
  }

  if (locale.empty()) {
    locale = default_locale;
    mxdebug_if(debug, fmt::format("[init_locales setting to default locale {0}]\n", locale));
  }

# if defined(SYS_WINDOWS)
  mtx::sys::set_environment_variable("LANGUAGE", "");

  if (!locale.empty()) {
    // The Windows system headers define LC_MESSAGES but
    // Windows itself doesn't know it and treats "set_locale(LC_MESSAGES, ...)"
    // as an error. gettext uses the LANG and LC_MESSAGE environment variables instead.

    // Windows knows two environments: the system environment that's
    // modified by SetEnvironmentVariable() and the C library's cache
    // of said environment which is modified via _putenv().

    mtx::sys::set_environment_variable("LANG",        locale);
    mtx::sys::set_environment_variable("LC_MESSAGES", locale);

    translation_c::set_active_translation(locale);
  }

  locale_dir = g_cc_local_utf8->native((mtx::sys::get_installation_path() / "locale").string());

# else  // SYS_WINDOWS
  auto language_var = mtx::sys::get_environment_variable("LANGUAGE");
  if (!language_var.empty()) {
    mxdebug_if(debug, fmt::format("[init_locales LANGUAGE is set to {0}; un-setting it]\n", language_var));
    mtx::sys::unset_environment_variable("LANGUAGE");
  }

  std::string chosen_locale;

  try {
    locale_string_c loc_default(default_locale);
    std::string loc_req_with_default_codeset(locale_string_c(locale).set_codeset_and_modifier(loc_default).str());

    mxdebug_if(debug, fmt::format("[init_locales loc_default is {0}; trying locale {2} followed by loc_req_with_default_codeset {1}]\n", loc_default.str(), loc_req_with_default_codeset, locale));

    if (setlocale(LC_MESSAGES, locale.c_str()))
      chosen_locale = locale;

    else if (setlocale(LC_MESSAGES, loc_req_with_default_codeset.c_str()))
      chosen_locale = loc_req_with_default_codeset;

    else {
      std::string loc_req_with_utf8 = locale_string_c(locale).set_codeset_and_modifier(locale_string_c("dummy.UTF-8")).str();
      mxdebug_if(debug, fmt::format("[init_locales both failed; also trying {0}]\n", loc_req_with_utf8));
      if (setlocale(LC_MESSAGES, loc_req_with_utf8.c_str()))
        chosen_locale = loc_req_with_utf8;
    }

  } catch (mtx::locale_string_format_x &error) {
    mxdebug_if(debug, fmt::format("[init_locales format error in {0}]\n", error.error()));
  }

  mxdebug_if(debug, fmt::format("[init_locales chosen locale {0}]\n", chosen_locale));

  // Hard fallback to "C" locale if no suitable locale was
  // selected. This can happen if the system has no locales for
  // "en_US" or "en_US.UTF-8" compiled.
  if (chosen_locale.empty() && setlocale(LC_MESSAGES, "C"))
    chosen_locale = "C";

  if (chosen_locale.empty())
    mxerror(Y("The locale could not be set properly. Check the LANG, LC_ALL and LC_MESSAGES environment variables.\n"));

  translation_c::set_active_translation(chosen_locale);

#  if defined(SYS_APPLE)
  locale_dir = (mtx::sys::get_installation_path() / "locale").string();
#  else
  auto appimage_locale_dir = mtx::sys::get_installation_path() / ".." / "share" / "locale";
  locale_dir               = boost::filesystem::is_directory(appimage_locale_dir) ? appimage_locale_dir.string() : std::string{MTX_LOCALE_DIR};
#  endif  // SYS_APPLE

# endif  // SYS_WINDOWS

# if defined(SYS_APPLE)
  int result = setenv("LC_MESSAGES", chosen_locale.c_str(), 1);
  mxdebug_if(debug, fmt::format("[init_locales setenv() return code: {0}]\n", result));
# endif

  mxdebug_if(debug, fmt::format("[init_locales locale_dir: {0}]\n", locale_dir));

  bindtextdomain("mkvtoolnix", locale_dir.c_str());
  textdomain("mkvtoolnix");
  bind_textdomain_codeset("mkvtoolnix", "UTF-8");
}

#else  // HAVE_LIBINTL_H

void
init_locales(std::string) {
  translation_c::determine_default_iso639_ui_language();
  translation_c::initialize_available_translations();
}

#endif  // HAVE_LIBINTL_H
