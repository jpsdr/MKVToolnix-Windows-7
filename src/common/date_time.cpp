/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <sstream>
#if defined(SYS_WINDOWS)
# include <windows.h>
#endif

#include "common/date_time.h"

namespace mtx::date_time {

int64_t
to_time_t(boost::posix_time::ptime const &pt) {
  auto diff = pt - boost::posix_time::ptime{ boost::gregorian::date(1970, 1, 1) };
  return diff.ticks() / diff.ticks_per_second();
}

std::string
to_string(boost::posix_time::ptime const &writing_date,
          char const *format) {
  std::stringstream ss;
  auto output_facet = new boost::posix_time::time_facet(format);

  ss.imbue(std::locale{ std::locale::classic(), output_facet });

  ss << writing_date;

  return ss.str();
}

std::string
format_epoch_time(time_t const epoch_time,
                  std::string format_string,
                  epoch_timezone_e timezone) {
  auto time_info = timezone == epoch_timezone_e::UTC ? std::gmtime(&epoch_time) : std::localtime(&epoch_time);
  if (!time_info)
    return {};

#if defined(SYS_WINDOWS)
  // std::strftime on MSVC does not conform to C++11/POSIX with
  // respect to the %z format modifier. Its output is usually the time
  // zone name, not its offset. See
  // https://msdn.microsoft.com/en-us/library/fe06s4ak.aspx
  TIME_ZONE_INFORMATION time_zone_info{};

  auto result = GetTimeZoneInformation(&time_zone_info);
  auto bias   = -1 * (time_zone_info.Bias + (result == TIME_ZONE_ID_DAYLIGHT ? time_zone_info.DaylightBias : 0));
  auto offset = fmt::format("{0}{1:02}{2:02}", bias >= 0 ? '+' : '-', std::abs(bias) / 60, std::abs(bias) % 60);

  boost::replace_all(format_string, "%z", offset);
#endif

  format_string += 'z';

  std::string buffer;
  buffer.resize(format_string.size());

  while (true) {
    auto len = std::strftime(&buffer[0], buffer.size(), format_string.c_str(), time_info);
    if (len) {
      buffer.resize(len - 1);
      break;
    }

    buffer.resize(buffer.size() * 2);
  }

  return buffer;
}

std::string
format_epoch_time_iso_8601(std::time_t const epoch_time,
                           epoch_timezone_e timezone) {
  if (timezone == epoch_timezone_e::UTC)
    return format_epoch_time(epoch_time, "%Y-%m-%dT%H:%M:%SZ", timezone);

  auto result = format_epoch_time(epoch_time, "%Y-%m-%dT%H:%M:%S%z", timezone);
  if (result.length() >= 2)
    result.insert(result.length() - 2, ":");

  return result;
}

}
