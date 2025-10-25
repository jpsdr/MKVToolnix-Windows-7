/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   definitions used in all programs, helper functions

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include <QRegularExpression>

#include "common/memory.h"
#include "common/qt.h"
#include "common/strings/editing.h"

namespace mtx::string {

std::vector<std::string>
split(std::string const &text,
      QRegularExpression const &pattern,
      std::size_t max) {
  if (text.empty())
    return { text };

  std::vector<std::string> results;

  int copiedUpTo = 0;
  auto text_q    = Q(text);
  auto itr       = pattern.globalMatch(text_q);

  while (itr.hasNext() && ((results.size() + 1) < max)) {
    auto match = itr.next();
    auto start = match.capturedStart(0);
    auto end   = match.capturedEnd(0);

    results.push_back(to_utf8(text_q.mid(copiedUpTo, start - copiedUpTo)));
    copiedUpTo = end;
  }

  results.push_back(to_utf8(text_q.mid(copiedUpTo)));

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

  while ((idx < len) && (!c[len - idx - 1] || mtx::string::is_blank_or_tab(c[len - idx - 1]) || (newlines && mtx::string::is_newline(c[len - idx - 1]))))
    ++idx;

  if (idx > 0)
    s.erase(len - idx, idx);
}

void
strip(std::string &s,
      bool newlines) {
  auto c  = s.c_str();
  int idx = 0, len = s.length();

  while ((idx < len) && (!c[idx] || mtx::string::is_blank_or_tab(c[idx]) || (newlines && mtx::string::is_newline(c[idx]))))
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
    if (!mtx::string::is_blank_or_tab(s[i])) {
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
  static std::optional<QRegularExpression> s_cr_lf_re, s_cr_re, s_lf_re;

  if (!s_cr_lf_re) {
    s_cr_lf_re = QRegularExpression{"\r\n"};
    s_cr_re    = QRegularExpression{"\r"};
    s_lf_re    = QRegularExpression{"\n"};
  }

  auto result = Q(str).replace(*s_cr_lf_re, "\n");
  result      = result.replace(*s_cr_re,    "\n");

  if (line_ending_style_e::lf == line_ending_style)
    return to_utf8(result);

  return to_utf8(result.replace(*s_lf_re, "\r\n"));
}

std::string
chomp(std::string const &str) {
  static std::optional<QRegularExpression> s_trailing_lf_re;

  if (!s_trailing_lf_re)
    s_trailing_lf_re = QRegularExpression{"[\r\n]+$"};

  return to_utf8(Q(str).replace(*s_trailing_lf_re, {}));
}

QString
replace(QString const &original,
        QRegularExpression const &regex,
        std::function<QString(QRegularExpressionMatch const &)> replacement) {
  QString result;
  int copiedUpTo = 0;

  result.reserve(original.size());

  auto itr = regex.globalMatch(original);

  while (itr.hasNext()) {
    auto match = itr.next();
    auto start = match.capturedStart(0);
    auto end   = match.capturedEnd(0);

    if (start > copiedUpTo)
      result += original.mid(copiedUpTo, start - copiedUpTo);

    result     += replacement(match);
    copiedUpTo  = end;
  }

  if (copiedUpTo < original.size())
    result += original.mid(copiedUpTo);

  return result;
}

std::string
replace(std::string const &original,
        QRegularExpression const &regex,
        std::function<QString(QRegularExpressionMatch const &)> replacement) {
  return to_utf8(replace(Q(original), regex, replacement));
}

std::string
replace(char const *original,
        QRegularExpression const &regex,
        std::function<QString(QRegularExpressionMatch const &)> replacement) {
  return to_utf8(replace(Q(original), regex, replacement));
}

} // mtx::string
