/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   regular expression helper functions

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include "common/strings/regex.h"

namespace mtx::regex {

std::string
escape(std::string const &s) {
  std::regex re{R"(([\^$\\.*+?(){}\[\]|]))"};
  return std::regex_replace(s, re, R"(\$1)");
}

}
