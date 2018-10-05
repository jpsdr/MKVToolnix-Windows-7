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

const std::string empty_string("");

std::vector<std::string>
split(std::string const &text,
      boost::regex const &pattern,
      size_t max,
      boost::match_flag_type match_flags) {
  std::vector<std::string> results;

  auto match = boost::make_regex_iterator(text, pattern, match_flags);
  boost::sregex_iterator end;
  size_t previous_match_end = 0;

  while (   (match != end)
         && (   (0 == max)
             || ((results.size() + 1) < max))) {
    results.push_back(text.substr(previous_match_end, match->position(static_cast<boost::sregex_iterator::value_type::size_type>(0)) - previous_match_end));
    previous_match_end = match->position(static_cast<boost::sregex_iterator::value_type::size_type>(0)) + match->length(0);
    ++match;
  }

  if (previous_match_end <= text.size())
    results.push_back(text.substr(std::min(previous_match_end, std::max<size_t>(text.size(), 1) - 1), text.size() - previous_match_end));

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
get_displayable_string(const char *src,
                       int max_len) {
  std::string result;
  int len = (-1 == max_len) ? strlen(src) : max_len;

  for (int i = 0; i < len; ++i)
    result += (' ' > src[i]) ? '?' : src[i];

  return result;
}

std::string
get_displayable_string(std::string const &src) {
  return get_displayable_string(src.c_str(), src.length());
}

std::string
normalize_line_endings(std::string const &str,
                       line_ending_style_e line_ending_style) {
  static boost::regex s_cr_lf_re, s_cr_re, s_lf_re;

  if (s_cr_lf_re.empty()) {
    s_cr_lf_re = boost::regex{"\r\n", boost::regex::perl};
    s_cr_re    = boost::regex{"\r",   boost::regex::perl};
    s_lf_re    = boost::regex{"\n",   boost::regex::perl};
  }

  auto result = boost::regex_replace(str,    s_cr_lf_re, "\n");
  result      = boost::regex_replace(result, s_cr_re,    "\n");

  if (line_ending_style_e::lf == line_ending_style)
    return result;

  return boost::regex_replace(result, s_lf_re, "\r\n");
}

std::string
chomp(std::string const &str) {
  static boost::regex s_trailing_lf_re;

  if (s_trailing_lf_re.empty())
    s_trailing_lf_re = boost::regex{"[\r\n]+\\z", boost::regex::perl};

  return boost::regex_replace(str, s_trailing_lf_re, "");
}
