/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   class definitions for the error exception class

   Written by Moritz Bunkus <moritz@bunkus.org>.
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
