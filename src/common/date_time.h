/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   declarations date/time helper functions

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#ifndef MTX_COMMON_DATE_TIME_H
#define MTX_COMMON_DATE_TIME_H

#include "common/common_pch.h"

#include <boost/date_time/posix_time/ptime.hpp>

namespace mtx { namespace date_time {

int64_t to_time_t(boost::posix_time::ptime const &pt);
std::string to_string(boost::posix_time::ptime const &pt, char const *format);

inline std::string
to_string(boost::posix_time::ptime const &pt,
          std::string const &format) {
  return to_string(pt, format.c_str());
}

}}

#endif // MTX_COMMON_DATE_TIME_H
