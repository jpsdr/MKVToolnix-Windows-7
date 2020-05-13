/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   definitions used in all programs, helper functions

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include "common/memory.h"
#include "common/strings/editing.h"
#include "common/strings/regex.h"

namespace mtx::string {

std::vector<std::string>
split(std::string const &text,
      std::regex const &pattern,
      std::size_t max) {
  std::vector<std::string> results;

  auto match = std::sregex_iterator(text.begin(), text.end(), pattern);
  std::sregex_iterator end;
  std::size_t previous_match_end = 0;

  while (   (match != end)
         && (   (0 == max)
             || ((results.size() + 1) < max))) {
    results.push_back(text.substr(previous_match_end, match->position(0) - previous_match_end));
    previous_match_end = match->position(0) + match->length(0);
    ++match;
  }

  if (previous_match_end <= text.size())
    results.push_back(text.substr(std::min(previous_match_end, std::max<std::size_t>(text.size(), 1) - 1), text.size() - previous_match_end));

  return results;
}

std::vector<std::string>
split(std::string const &text,
      std::string const &pattern,
      std::size_t max) {
  return split(text, std::regex{mtx::regex::escape(pattern)}, max);
}

void
strip_back(std::string &s,
           bool newlines) {
  auto c  = s.c_str();
  int idx = 0, len = s.length();

  while ((idx < len) && (!c[len - idx - 1] || isblanktab(c[len - idx - 1]) || (newlines && iscr(c[len - idx - 1]))))
    ++idx;

  if (idx > 0)
    s.erase(len - idx, idx);
}

void
strip(std::string &s,
      bool newlines) {
  auto c  = s.c_str();
  int idx = 0, len = s.length();

  while ((idx < len) && (!c[idx] || isblanktab(c[idx]) || (newlines && iscr(c[idx]))))
    ++idx;

  if (idx > 0)
    s.erase(0, idx);

  strip_back(s, newlines);
}

void
strip(std::vector<std::string> &v,
      bool newlines) {
  size_t i;

  for (i = 0; i < v.size(); i++)
    strip(v[i], newlines);
}

std::string
strip_copy(std::string const &s,
           bool newlines) {
  auto new_s = s;
  strip(new_s, newlines);
  return new_s;
}

std::string &
shrink_whitespace(std::string &s) {
  size_t i                     = 0;
  bool previous_was_whitespace = false;
  while (s.length() > i) {
    if (!isblanktab(s[i])) {
      previous_was_whitespace = false;
      ++i;
      continue;
    }
    if (previous_was_whitespace)
      s.erase(i, 1);
    else {
      previous_was_whitespace = true;
      ++i;
    }
  }

  return s;
}

std::string
get_displayable(const char *src,
                int max_len) {
  std::string result;
  int len = (-1 == max_len) ? strlen(src) : max_len;

  for (int i = 0; i < len; ++i)
    result += (' ' > src[i]) ? '?' : src[i];

  return result;
}

std::string
get_displayable(std::string const &src) {
  return get_displayable(src.c_str(), src.length());
}

std::string
normalize_line_endings(std::string const &str,
                       line_ending_style_e line_ending_style) {
  static std::optional<std::regex> s_cr_lf_re, s_cr_re, s_lf_re;

  if (!s_cr_lf_re) {
    s_cr_lf_re = std::regex{"\r\n"};
    s_cr_re    = std::regex{"\r"};
    s_lf_re    = std::regex{"\n"};
  }

  auto result = std::regex_replace(str,    *s_cr_lf_re, "\n");
  result      = std::regex_replace(result, *s_cr_re,    "\n");

  if (line_ending_style_e::lf == line_ending_style)
    return result;

  return std::regex_replace(result, *s_lf_re, "\r\n");
}

std::string
chomp(std::string const &str) {
  static std::optional<std::regex> s_trailing_lf_re;

  if (!s_trailing_lf_re)
    s_trailing_lf_re = std::regex{"[\r\n]+$"};

  return std::regex_replace(str, *s_trailing_lf_re, "");
}

} // mtx::string
