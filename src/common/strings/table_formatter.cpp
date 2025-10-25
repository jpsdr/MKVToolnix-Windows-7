/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   formatting tabular data as a string

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/strings/table_formatter.h"
#include "common/strings/utf8.h"

namespace mtx::string {

table_formatter_c &
table_formatter_c::set_header(std::vector<std::string> const &header) {
  m_header = header;
  return *this;
}

table_formatter_c &
table_formatter_c::set_alignment(std::vector<alignment_e> const &alignment) {
  m_alignment = alignment;
  return *this;
}


table_formatter_c &
table_formatter_c::add_row(std::vector<std::string> const &row) {
  m_rows.emplace_back(row);
  return *this;
}

std::string
table_formatter_c::format()
  const {
  if (m_rows.empty() && m_header.empty())
    return {};

  auto num_columns = m_header.size();
  auto alignment   = m_alignment;

  while (alignment.size() < num_columns)
    alignment.push_back(align_left);

  // 1. Calculate column widths.
  std::vector<uint64_t> column_widths(num_columns, 0ull);

  for (auto column = 0u; column < num_columns; ++column)
    column_widths[column] = get_width_in_em(to_wide(m_header[column]));

  for (auto const &row : m_rows)
    for (auto column = 0u; column < num_columns; ++column)
      column_widths[column] = std::max<uint64_t>(column_widths[column], get_width_in_em(to_wide(row[column])));

  // 2. Generate column format strings for fmt::format.
  std::vector<std::string> column_formats;
  column_formats.reserve(num_columns);

  auto row_size = 1u + (num_columns - 1) * 3;
  auto idx      = 0;

  for (auto width : column_widths) {
    auto fmt_alignment = alignment[idx] == align_left  ? "<"
                       : alignment[idx] == align_right ? ">"
                       :                                 "^";

    column_formats.emplace_back(fmt::format("{{0:{0}{1}s}}", fmt_alignment, width));
    row_size += width;
    ++idx;
  }

  // 3. Reserve space for full output & create row formatting lambda.
  auto output_size = (m_rows.size() + 2) * row_size;

  std::string output;
  output.reserve(output_size);

  auto format_row = [&output, &column_formats, num_columns](std::vector<std::string> const &row, std::string const &separator) {
    for (auto column = 0u; column < num_columns; ++column) {
      if (column >= 1)
        output += separator;

      output += fmt::format(fmt::runtime(column_formats[column]), row[column]);
    }

    output += "\n";
  };

  // 4. Create separtors for columns & the header row.
  auto column_separator = " | "s;

  std::vector<std::string> header_separator;
  for (auto column = 0u; column < num_columns; ++column)
    header_separator.emplace_back(column_widths[column], '-');

  // 5. Format header, header separator, all rows.
  format_row(m_header,         column_separator);
  format_row(header_separator, "-+-"s);

  for (auto const &row : m_rows)
    format_row(row, column_separator);

  return output;
}

}
