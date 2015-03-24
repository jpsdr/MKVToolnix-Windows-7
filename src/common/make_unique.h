/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   A definition for std::make_unique for compilers that don't have it
   yet (C++14).

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#ifndef MTX_COMMON_MAKE_UNIQUE_H
#define MTX_COMMON_MAKE_UNIQUE_H

#include "config.h"

#if !defined(HAVE_STD_MAKE_UNIQUE)

# include <memory>
# include <utility>

namespace std {

template<typename Ttype,
         typename... Targs>
std::unique_ptr<Ttype> make_unique(Targs &&... args) {
  return std::unique_ptr<Ttype>(new Ttype(std::forward<Targs>(args)...));
}

}

#endif  // !HAVE_STD_MAKE_UNIQUE

#endif  // MTX_COMMON_MAKE_UNIQUE_H
