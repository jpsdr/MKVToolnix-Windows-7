/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   helper functions for WebM

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include <QRegularExpression>

#include "common/qt.h"
#include "common/webm.h"

bool
is_webm_file_name(const std::string &file_name) {
  static QRegularExpression s_webm_file_name_re("\\.webm[av]?$");
  return Q(file_name).contains(s_webm_file_name_re);
}
