/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   ISO 639 language definitions, lookup functions

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include <boost/version.hpp>
#include <unordered_map>

#include "common/iso639.h"
#include "common/strings/editing.h"
#include "common/strings/table_formatter.h"
#include "common/strings/utf8.h"

namespace mtx::iso639 {

namespace {

std::unordered_map<std::string, std::string> s_deprecated_1_and_2_codes{
  // ISO 639-1
  { "iw", "he" },

  // ISO 639-2
  { "scr", "hrv" },
  { "scc", "srp" },
  { "mol", "rum" },
};

} // anonymous namespace

void
list_languages() {
  mtx::string::table_formatter_c formatter;
  formatter.set_header({ Y("English language name"), Y("ISO 639-3 code"), Y("ISO 639-2 code"), Y("ISO 639-1 code") });

  for (auto &lang : g_languages)
    formatter.add_row({ gettext(lang.english_name.c_str()), lang.alpha_3_code, lang.is_part_of_iso639_2 ? lang.alpha_3_code : ""s, lang.alpha_2_code });

  mxinfo(formatter.format());
}

/** \brief Map a string to a ISO 639-2 language code

   Searches the array of ISO 639 codes. If \c s is a valid ISO 639-2
   code, a valid ISO 639-1 code, a valid terminology abbreviation
   for an ISO 639-2 code or the English name for an ISO 639-2 code
   then it returns the index of that entry in the \c g_languages array.

   \param c The string to look for in the array of ISO 639 codes.
   \return The index into the \c g_languages array if found or
   an empty optional if no such entry was found.
*/
std::optional<language_t>
look_up(std::string const &s,
        bool also_look_up_by_name) {
  if (s.empty())
    return {};

  auto source          = s;
  auto deprecated_code = s_deprecated_1_and_2_codes.find(source);
  if (deprecated_code != s_deprecated_1_and_2_codes.end())
    source = deprecated_code->second;

  auto lang_code = std::find_if(g_languages.begin(), g_languages.end(), [&source](auto const &lang) { return (lang.alpha_3_code == source) || (lang.terminology_abbrev == source) || (lang.alpha_2_code == source); });
  if (lang_code != g_languages.end())
    return *lang_code;

  if (!also_look_up_by_name)
    return {};

  for (auto const &language : g_languages) {
    auto const &english_name = language.english_name;
    auto s_lower             = balg::to_lower_copy(s);
    auto names               = mtx::string::split(english_name, ";");

    mtx::string::strip(names);

    for (auto const &name : names)
      if (balg::to_lower_copy(name) == s_lower)
        return language;
  }

  for (auto const &language : g_languages) {
    auto names = mtx::string::split(language.english_name, ";");

    mtx::string::strip(names);

    if (names.end() != std::find_if(names.begin(), names.end(), [&source](auto const &name) { return balg::istarts_with(name, source); }))
      return language;
  }

  return {};
}

} // namespace mtx::iso639

// Make the following two strings translatable:
#undef Y
#define Y(x)
Y("Undetermined")
Y("No linguistic content; Not applicable")
