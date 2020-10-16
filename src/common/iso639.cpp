/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   ISO 639 language definitions, lookup functions

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include <boost/version.hpp>
#include <unordered_map>

#include "common/iso639.h"
#include "common/strings/editing.h"
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

#define FILL(s, idx) s + std::wstring(longest[idx] - get_width_in_em(s), L' ')

void
list_languages() {
  std::wstring w_col1 = to_wide(Y("English language name"));
  std::wstring w_col2 = to_wide(Y("ISO 639-2 code"));
  std::wstring w_col3 = to_wide(Y("ISO 639-1 code"));

  size_t longest[3]   = { get_width_in_em(w_col1), get_width_in_em(w_col2), get_width_in_em(w_col3) };

  for (auto &lang : g_languages) {
    longest[0] = std::max(longest[0], get_width_in_em(to_wide(gettext(lang.english_name.c_str()))));
    longest[1] = std::max(longest[1], get_width_in_em(to_wide(lang.alpha_3_code)));
    longest[2] = std::max(longest[2], get_width_in_em(to_wide(lang.alpha_2_code)));
  }

  mxinfo(FILL(w_col1, 0) + L" | " + FILL(w_col2, 1) + L" | " + FILL(w_col3, 2) + L"\n");
  mxinfo(std::wstring(longest[0] + 1, L'-') + L'+' + std::wstring(longest[1] + 2, L'-') + L'+' + std::wstring(longest[2] + 1, L'-') + L"\n");

  for (auto &lang : g_languages) {
    std::wstring english = to_wide(gettext(lang.english_name.c_str()));
    std::wstring code2   = to_wide(lang.alpha_3_code);
    std::wstring code1   = to_wide(lang.alpha_2_code);
    mxinfo(FILL(english, 0) + L" | " + FILL(code2, 1) + L" | " + FILL(code1, 2) + L"\n");
  }
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
        bool allow_short_english_name) {
  if (s.empty())
    return {};

  auto source          = s;
  auto deprecated_code = s_deprecated_1_and_2_codes.find(source);
  if (deprecated_code != s_deprecated_1_and_2_codes.end())
    source = deprecated_code->second;

  auto lang_code = std::find_if(g_languages.begin(), g_languages.end(), [&source](auto const &lang) { return (lang.alpha_3_code == source) || (lang.terminology_abbrev == source) || (lang.alpha_2_code == source); });
  if (lang_code != g_languages.end())
    return *lang_code;

  for (auto const &language : g_languages) {
    auto const &english_name = language.english_name;
    auto s_lower             = balg::to_lower_copy(s);
    auto names               = mtx::string::split(english_name, ";");

    mtx::string::strip(names);

    for (auto const &name : names)
      if (balg::to_lower_copy(name) == s_lower)
        return language;
  }

  if (!allow_short_english_name)
    return {};

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
