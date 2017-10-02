/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   declarations date/time helper functions

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#include <ctime>

#include <boost/date_time/posix_time/ptime.hpp>

namespace mtx { namespace date_time {

int64_t to_time_t(boost::posix_time::ptime const &pt);
std::string to_string(boost::posix_time::ptime const &pt, char const *format);

inline std::string
to_string(boost::posix_time::ptime const &pt,
          std::string const &format) {
  return to_string(pt, format.c_str());
}

enum class epoch_timezone_e {
  UTC,
  local,
};

std::string format_epoch_time(std::time_t const epoch_time, std::string format_string, epoch_timezone_e timezone);
std::string format_epoch_time_iso_8601(std::time_t const epoch_time, epoch_timezone_e timezone);

}}
