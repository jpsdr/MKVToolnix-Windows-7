/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   Data for ComboBoxes in mmg and other occurences

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <sstream>

#include "common/date_time.h"

namespace mtx { namespace date_time {

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

}}
