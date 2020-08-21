/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   helper functions for WebM

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include "common/regex.h"
#include "common/webm.h"

bool
is_webm_file_name(const std::string &file_name) {
  static mtx::regex::jp::Regex s_webm_file_name_re("\\.webm[av]?$");
  return mtx::regex::match(file_name, s_webm_file_name_re);
}
