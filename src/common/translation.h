/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   translation, locale handling

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include <ostream>

class translation_c {
public:
  static std::vector<translation_c> ms_available_translations;
  static int ms_active_translation_idx;
  static std::string ms_default_iso639_ui_language;

public:
  std::string m_iso639_alpha_3_code, m_unix_locale, m_windows_locale, m_windows_locale_sysname, m_english_name, m_translated_name;
  bool m_line_breaks_anywhere;
  int m_language_id, m_sub_language_id;

  translation_c(std::string iso639_alpha_3_code,
                std::string unix_locale,
                std::string windows_locale,
                std::string windows_locale_sysname,
                std::string english_name,
                std::string translated_name,
                bool line_breaks_anywhere,
                int language_id,
                int sub_language_id);

  std::string get_locale() const;
  bool matches(std::string const &locale) const;

  static void initialize_available_translations();
  static int look_up_translation(const std::string &locale);
  static int look_up_translation(int language_id, int sub_language_id);
  static std::string get_default_ui_locale();
  static void determine_default_iso639_ui_language();
  static translation_c &get_active_translation();
  static void set_active_translation(const std::string &locale);
};

class translatable_string_c {
protected:
  std::vector<std::string> m_untranslated_strings;
  std::optional<std::string> m_overridden_by;

public:
  translatable_string_c() = default;
  translatable_string_c(const std::string &untranslated_string);
  translatable_string_c(const char *untranslated_string);
  translatable_string_c(std::vector<translatable_string_c> const &untranslated_strings);

  std::string get_translated() const;
  std::string get_untranslated() const;

  translatable_string_c &override(std::string const &by);

protected:
  std::string join(std::vector<std::string> const &strings) const;
};

#define YT(s) translatable_string_c(s)

inline std::ostream &
operator <<(std::ostream &out,
            translatable_string_c const &s) {
  out << s.get_translated();
  return out;
}

#if FMT_VERSION >= 90000
template <> struct fmt::formatter<translatable_string_c> : ostream_formatter {};
#endif

void init_locales(std::string locale = "");
