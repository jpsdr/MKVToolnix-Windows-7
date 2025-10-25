/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   string helper functions

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include <QRegularExpression>

namespace mtx::string {

constexpr auto is_blank_or_tab(char c) { return (c == ' ')   || (c == '\t'); }
constexpr auto is_newline(char c)      { return (c == '\n')  || (c == '\r'); }

enum class line_ending_style_e {
  cr_lf,
  lf,
};

std::string normalize_line_endings(std::string const &str, line_ending_style_e line_ending_style = line_ending_style_e::lf);
std::string chomp(std::string const &str);

std::vector<std::string> split(std::string const &text, QRegularExpression const &pattern, std::size_t max = std::numeric_limits<std::size_t>::max());
std::vector<std::string> split(std::string const &text, std::string const &pattern = ",", std::size_t max = std::numeric_limits<std::size_t>::max());

void strip(std::string &s, bool newlines = false);
std::string strip_copy(std::string const &s, bool newlines = false);
void strip(std::vector<std::string> &v, bool newlines = false);
void strip_back(std::string &s, bool newlines = false);

std::string &shrink_whitespace(std::string &s);

std::string get_displayable(const char *src, int max_len = -1);
std::string get_displayable(std::string const &src);

QString replace(QString const &original, QRegularExpression const &regex, std::function<QString(QRegularExpressionMatch const &)> replacement);
std::string replace(std::string const &original, QRegularExpression const &regex, std::function<QString(QRegularExpressionMatch const &)> replacement);
std::string replace(char const *original, QRegularExpression const &regex, std::function<QString(QRegularExpressionMatch const &)> replacement);

} // mtx::string
