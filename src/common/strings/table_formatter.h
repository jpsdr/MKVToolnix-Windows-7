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

namespace mtx::string {

class table_formatter_c {
public:
  enum alignment_e {
    align_left,
    align_right,
    align_center,
  };

private:
  std::vector<std::string> m_header;
  std::vector<alignment_e> m_alignment;
  std::vector<std::vector<std::string>> m_rows;

public:
  table_formatter_c &set_header(std::vector<std::string> const &header);
  table_formatter_c &set_alignment(std::vector<alignment_e> const &alignment);
  table_formatter_c &add_row(std::vector<std::string> const &row);
  std::string format() const;
};

}
