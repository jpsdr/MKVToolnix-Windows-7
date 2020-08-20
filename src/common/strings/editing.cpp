/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   definitions used in all programs, helper functions

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include "common/memory.h"
#include "common/strings/editing.h"

namespace mtx::string {

std::vector<std::string>
split(std::string const &text,
      mtx::regex::jp::Regex const &pattern,
      std::size_t max) {
  if (text.empty())
    return { text };

  std::vector<std::string> results;
  jpcre2::VecOff match_start, match_end;

  if (!mtx::regex::match(text, match_start, match_end, pattern, "g"))
    return {};

  if (match_start.size() != match_end.size())
    return {};

  std::size_t previous_match_end = 0, idx = 0;
  auto const num_matches = match_start.size();

  while (   (idx < num_matches)
         && ((results.size() + 1) < max)) {
    results.emplace_back(text.substr(previous_match_end, match_start[idx] - previous_match_end));
    previous_match_end = match_end[idx];
    ++idx;
  }

  if (previous_match_end <= text.size())
    results.emplace_back(text.substr(std::min(previous_match_end, text.size() - 1), text.size() - previous_match_end));

  return results;
}

std::vector<std::string>
split(std::string const &text,
      std::string const &pattern,
      std::size_t max) {
  if (text.empty())
    return { ""s };

  if (pattern.empty())
    return { text };

  std::vector<std::string> results;

  auto pos = text.find(pattern);
  std::size_t consumed_up_to = 0;

  while ((pos != std::string::npos) && ((results.size() + 1) < max)) {
    results.emplace_back(text.substr(consumed_up_to, pos - consumed_up_to));

    consumed_up_to = pos + pattern.size();
    pos            = text.find(pattern, consumed_up_to);
  }

  if (consumed_up_to < text.size())
    results.emplace_back(text.substr(consumed_up_to));

  else if (consumed_up_to == text.size())
    results.emplace_back(""s);

  return results;
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
  static std::optional<mtx::regex::jp::Regex> s_cr_lf_re, s_cr_re, s_lf_re;

  if (!s_cr_lf_re) {
    s_cr_lf_re = mtx::regex::jp::Regex{"\r\n", "S"};
    s_cr_re    = mtx::regex::jp::Regex{"\r",   "S"};
    s_lf_re    = mtx::regex::jp::Regex{"\n",   "S"};
  }

  auto result = mtx::regex::replace(str,    *s_cr_lf_re, "g", "\n");
  result      = mtx::regex::replace(result, *s_cr_re,    "g", "\n");

  if (line_ending_style_e::lf == line_ending_style)
    return result;

  return mtx::regex::replace(result, *s_lf_re, "g", "\r\n");
}

std::string
chomp(std::string const &str) {
  static std::optional<mtx::regex::jp::Regex> s_trailing_lf_re;

  if (!s_trailing_lf_re)
    s_trailing_lf_re = mtx::regex::jp::Regex{"[\r\n]+$", "S"};

  return mtx::regex::replace(str, *s_trailing_lf_re, "g", "");
}

} // mtx::string
