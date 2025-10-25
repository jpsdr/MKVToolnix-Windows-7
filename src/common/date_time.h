/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   declarations date/time helper functions

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

class QDateTime;

namespace mtx::date_time {

std::string format(QDateTime const &timestamp, std::string const &format_string);
std::string format_iso_8601(QDateTime const &timestamp);

}
