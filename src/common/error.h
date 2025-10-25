/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definitions for the error exception class

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include <ostream>

namespace mtx {

class exception: public std::exception {
public:
  virtual const char *what() const throw() {
    return "unspecified MKVToolNix error";
  }

  virtual std::string error() const throw() {
    return what();
  }
};

class invalid_parameter_x: public exception {
public:
  virtual const char *what() const throw() {
    return "invalid parameter in function call";
  }
};

inline std::ostream &
operator <<(std::ostream &out,
            exception const &ex) {
  out << ex.what();
  return out;
}

}

#if FMT_VERSION >= 90000
template <> struct fmt::formatter<mtx::exception> : ostream_formatter {};
#endif  // FMT_VERSION >= 90000
