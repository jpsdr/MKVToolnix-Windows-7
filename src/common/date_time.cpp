/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include <QDateTime>
#include <QTimeZone>

#include "common/date_time.h"

namespace mtx::date_time {

std::string
format(QDateTime const &timestamp,
       std::string const &format_string) {
  if (!timestamp.isValid())
    return {};

  std::string output;
  auto is_format_char   = false;
  auto date             = timestamp.date();
  auto time             = timestamp.time();
  auto time_zone_offset = timestamp.timeZone().offsetFromUtc(timestamp.toLocalTime());

  for (auto const &c : format_string) {
    if (!is_format_char && (c == '%'))
      is_format_char = true;

    else if (!is_format_char)
      output += c;

    else {
      is_format_char = false;

      if (c == 'Y')
        output += fmt::format("{0:04d}", date.year());

      else if (c == 'm')
        output += fmt::format("{0:02d}", date.month());

      else if (c == 'd')
        output += fmt::format("{0:02d}", date.day());

      else if (c == 'H')
        output += fmt::format("{0:02d}", time.hour());

      else if (c == 'M')
        output += fmt::format("{0:02d}", time.minute());

      else if (c == 'S')
        output += fmt::format("{0:02d}", time.second());

      else if (c == 'f')
        output += fmt::format("{0:03d}", time.msec());

      else if (c == 'z')
        output += fmt::format("{0}{1:02d}:{2:02d}", time_zone_offset < 0 ? '-' : '+', std::abs(time_zone_offset) / 3600, (std::abs(time_zone_offset) / 60) % 60);

      else
        output += c;
    }

  }

  return output;
}

std::string
format_iso_8601(QDateTime const &timestamp) {
  auto zone_specifier = timestamp.timeZone() == QTimeZone::utc() ? "Z"s : "%z";
  return format(timestamp, "%Y-%m-%dT%H:%M:%S"s + zone_specifier);
}

}
