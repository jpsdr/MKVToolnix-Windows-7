/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   ISO 639 language definitions, lookup functions

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include <boost/version.hpp>
#include <unordered_map>

#include "common/iso639.h"
#include "common/strings/editing.h"
#include "common/strings/utf8.h"

static std::unordered_map<std::string, std::string> s_deprecated_1_and_2_codes{
  // ISO 639-1
  { "iw", "he" },

  // ISO 639-2
  { "scr", "hrv" },
  { "scc", "srp" },
  { "mol", "rum" },
};

std::vector<std::string> const g_popular_language_codes{
  // Derived from the list of spoken languages sorted by native speakers available at
  // https://en.wikipedia.org/wiki/List_of_languages_by_number_of_native_speakers
  // A couple of languages not on that list have been added due to user request.
  "amh",
  "ara",
  "aze",
  "ben",
  "bho",
  "bul",
  "bur",
  "ceb",
  "dan",
  "fin",
  "khm",
  "chi",
  "cze",
  "dut",
  "eng",
  "fre",
  "ger",
  "gre",
  "guj",
  "hau",
  "heb",
  "hin",
  "hun",
  "ibo",
  "ind",
  "ita",
  "jpn",
  "jav",
  "kan",
  "kaz",
  "kin",
  "kor",
  "kur",
  "mag",
  "mai",
  "may",
  "mal",
  "mar",
  "nep",
  "ori",
  "pan",
  "per",
  "pol",
  "por",
  "pus",
  "rum",
  "run",
  "rus",
  "snd",
  "sin",
  "som",
  "spa",
  "srp",
  "sun",
  "swe",
  "tam",
  "tel",
  "tha",
  "tur",
  "ukr",
  "urd",
  "uzb",
  "vie",
  "yor",
  "zul",

  // "Undetermined" & "No linguistic content"
  "und", "zxx",
 };

bool
is_valid_iso639_2_code(std::string const &iso639_2_code) {
  return std::find_if(g_iso639_languages.begin(), g_iso639_languages.end(), [&iso639_2_code](auto const &lang) { return lang.iso639_2_code == iso639_2_code; }) != g_iso639_languages.end();
}

#define FILL(s, idx) s + std::wstring(longest[idx] - get_width_in_em(s), L' ')

void
list_iso639_languages() {
  std::wstring w_col1 = to_wide(Y("English language name"));
  std::wstring w_col2 = to_wide(Y("ISO 639-2 code"));
  std::wstring w_col3 = to_wide(Y("ISO 639-1 code"));

  size_t longest[3]   = { get_width_in_em(w_col1), get_width_in_em(w_col2), get_width_in_em(w_col3) };

  for (auto &lang : g_iso639_languages) {
    longest[0] = std::max(longest[0], get_width_in_em(to_wide(gettext(lang.english_name.c_str()))));
    longest[1] = std::max(longest[1], get_width_in_em(to_wide(lang.iso639_2_code)));
    longest[2] = std::max(longest[2], get_width_in_em(to_wide(lang.iso639_1_code)));
  }

  mxinfo(FILL(w_col1, 0) + L" | " + FILL(w_col2, 1) + L" | " + FILL(w_col3, 2) + L"\n");
  mxinfo(std::wstring(longest[0] + 1, L'-') + L'+' + std::wstring(longest[1] + 2, L'-') + L'+' + std::wstring(longest[2] + 1, L'-') + L"\n");

  for (auto &lang : g_iso639_languages) {
    std::wstring english = to_wide(gettext(lang.english_name.c_str()));
    std::wstring code2   = to_wide(lang.iso639_2_code);
    std::wstring code1   = to_wide(lang.iso639_1_code);
    mxinfo(FILL(english, 0) + L" | " + FILL(code2, 1) + L" | " + FILL(code1, 2) + L"\n");
  }
}

std::string
map_iso639_2_to_iso639_1(std::string const &iso639_2_code) {
  auto lang = std::find_if(g_iso639_languages.begin(), g_iso639_languages.end(), [&iso639_2_code](auto const &lang) { return lang.iso639_2_code == iso639_2_code; });
  return (lang != g_iso639_languages.end()) ? lang->iso639_1_code : std::string{};
}

/** \brief Map a string to a ISO 639-2 language code

   Searches the array of ISO 639 codes. If \c s is a valid ISO 639-2
   code, a valid ISO 639-1 code, a valid terminology abbreviation
   for an ISO 639-2 code or the English name for an ISO 639-2 code
   then it returns the index of that entry in the \c g_iso639_languages array.

   \param c The string to look for in the array of ISO 639 codes.
   \return The index into the \c g_iso639_languages array if found or
     \c -1 if no such entry was found.
*/
int
map_to_iso639_2_code(std::string const &s,
                     bool allow_short_english_name) {
  if (s.empty())
    return -1;

  auto source          = s;
  auto deprecated_code = s_deprecated_1_and_2_codes.find(source);
  if (deprecated_code != s_deprecated_1_and_2_codes.end())
    source = deprecated_code->second;

  auto lang_code = std::find_if(g_iso639_languages.begin(), g_iso639_languages.end(), [&source](auto const &lang) { return (lang.iso639_2_code == source) || (lang.terminology_abbrev == source) || (lang.iso639_1_code == source); });
  if (lang_code != g_iso639_languages.end())
    return std::distance(g_iso639_languages.begin(), lang_code);

  for (int index = 0, num_languages = g_iso639_languages.size(); index < num_languages; ++index) {
    auto const &english_name = g_iso639_languages[index].english_name;
    auto s_lower             = balg::to_lower_copy(s);
    auto names               = mtx::string::split(english_name, ";");

    mtx::string::strip(names);

    for (auto const &name : names)
      if (balg::to_lower_copy(name) == s_lower)
        return index;
  }

  if (!allow_short_english_name)
    return -1;

  for (int index = 0, num_languages = g_iso639_languages.size(); index < num_languages; ++index) {
    auto const &english_name = g_iso639_languages[index].english_name;
    auto names               = mtx::string::split(english_name, ";");

    mtx::string::strip(names);

    if (names.end() != std::find_if(names.begin(), names.end(), [&source](auto const &name) { return balg::istarts_with(name, source); }))
      return index;
  }

  return -1;
}

// Make the following two strings translatable:
#undef Y
#define Y(x)
Y("Undetermined")
Y("No linguistic content; Not applicable")
