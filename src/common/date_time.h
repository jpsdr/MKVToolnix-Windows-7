/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   declarations date/time helper functions

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#include <ctime>

#include "common/date_time_fwd.h"

namespace mtx::date_time {

std::optional<std::tm> time_t_to_tm(std::time_t time, epoch_timezone_e timezone);
std::time_t tm_to_time_t(std::tm time_info, epoch_timezone_e timezone);

std::string format_epoch_time(std::time_t const epoch_time, std::string format_string, epoch_timezone_e timezone);
std::string format_epoch_time_iso_8601(std::time_t const epoch_time, epoch_timezone_e timezone);
std::string format_time_point(point_t const &time_point, std::string const &format_string, epoch_timezone_e timezone);

}
