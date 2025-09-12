/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include <QDateTime>
#include <QRegularExpression>
#include <QTimeZone>

#include "common/date_time.h"
#include "common/qt.h"
#include "common/strings/parsing.h"

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

std::optional<int64_t>
mtx::date_time::parse_iso_8601_to_epoch(std::string const &s) {
  //                      1        2        3                4        5        6           7  8     9        10
  QRegularExpression re{"^(\\d{4})-(\\d{2})-(\\d{2})(?:T|\\s)(\\d{2}):(\\d{2}):(\\d{2})\\s*(Z|([+-])(\\d{2}):(\\d{2}))$", QRegularExpression::CaseInsensitiveOption};
  int64_t year{}, month{}, day{}, hours{}, minutes{}, seconds{};
  int64_t offset_hours{}, offset_minutes{}, offset_mult{1};

  auto matches = re.match(Q(s));
  if (!matches.hasMatch())
    return std::nullopt;

  if (   !mtx::string::parse_number(to_utf8(matches.captured(1)), year)
      || !mtx::string::parse_number(to_utf8(matches.captured(2)), month)
      || !mtx::string::parse_number(to_utf8(matches.captured(3)), day)
      || !mtx::string::parse_number(to_utf8(matches.captured(4)), hours)
      || !mtx::string::parse_number(to_utf8(matches.captured(5)), minutes)
      || !mtx::string::parse_number(to_utf8(matches.captured(6)), seconds))
    return std::nullopt;

  if (to_utf8(matches.captured(7)) != "Z") {
    if (   !mtx::string::parse_number(to_utf8(matches.captured(9)),  offset_hours)
        || !mtx::string::parse_number(to_utf8(matches.captured(10)), offset_minutes))
      return std::nullopt;

    if (to_utf8(matches.captured(8)) == "-")
      offset_mult = -1;
  }

  if (   (year           < 1900)
      || (month          <    1)
      || (month          >   12)
      || (day            <    1)
      || (day            >   31)
      || (hours          <    0)
      || (hours          >   23)
      || (minutes        <    0)
      || (minutes        >   59)
      || (seconds        <    0)
      || (seconds        >   59)
      || (offset_hours   <    0)
      || (offset_hours   >   23)
      || (offset_minutes <    0)
      || (offset_minutes >   59))
    return std::nullopt;

  QDate date(year, month, day);
  QTime time(hours, minutes, seconds);
  auto secs = QDateTime{date, time, QTimeZone::utc()}.toSecsSinceEpoch();
  auto tz_offset_minutes = (offset_hours * 60 + offset_minutes) * offset_mult;
  secs -= tz_offset_minutes * 60;
  return std::make_optional(secs);
}
