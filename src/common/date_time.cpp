/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include <iomanip>
#include <sstream>
#include <time.h>
#if defined(SYS_WINDOWS)
# include <windows.h>
#endif

#include "common/date_time.h"

namespace mtx::date_time {

std::optional<std::tm>
time_t_to_tm(std::time_t time,
             epoch_timezone_e timezone) {
  static std::mutex s_mutex;
  std::lock_guard<std::mutex> lock{s_mutex};

  auto tm = timezone == epoch_timezone_e::UTC ? std::gmtime(&time) : std::localtime(&time);
  if (tm)
    return *tm;

  return {};
}

std::time_t
tm_to_time_t(std::tm time_info,
             epoch_timezone_e timezone) {
  if (timezone == epoch_timezone_e::local)
    return std::mktime(&time_info);

#if defined(SYS_WINDOWS)
  return ::_mkgmtime(&time_info);
#else
  return ::timegm(&time_info);
#endif
}


std::string
format_epoch_time(time_t const epoch_time,
                  std::string format_string,
                  epoch_timezone_e timezone) {
  auto time_info = time_t_to_tm(epoch_time, timezone);
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
    auto len = std::strftime(&buffer[0], buffer.size(), format_string.c_str(), &*time_info);
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

std::string
format_time_point(point_t const &time_point,
                  std::string const &format_string,
                  epoch_timezone_e timezone) {
  auto epoch_time = std::chrono::system_clock::to_time_t(time_point);
  auto time_info  = time_t_to_tm(epoch_time, timezone);
  if (!time_info)
    return {};

  auto ms           = std::chrono::duration_cast<std::chrono::milliseconds>(time_point.time_since_epoch()).count() % 1'000;
  auto fixed_format = format_string;

  boost::replace_all(fixed_format, "%f", fmt::format("{0:03d}", ms));

  return format_epoch_time(epoch_time, fixed_format, timezone);
}

}
