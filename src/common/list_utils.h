/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   list utility functions

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#ifndef MTX_COMMON_LIST_UTILS_H
#define MTX_COMMON_LIST_UTILS_H

#include <boost/optional.hpp>

namespace mtx {

template<typename T>
bool
included_in(T const &needle,
         T const &val) {
  return needle == val;
}

template<typename T,
         typename... Trest>
bool
included_in(T const &needle,
            T const &val,
            Trest... rest) {
  return (needle == val) || included_in(needle, rest...);
}

template<typename T>
bool
any_of(std::function<bool(T const &)> pred,
       T const &val) {
  return pred(val);
}

template<typename T,
         typename... Trest>
bool
any_of(std::function<bool(T const &)> pred,
       T const &val,
       Trest... rest) {
  return pred(val) || any_of(pred, rest...);
}

template<typename T>
bool
all_of(std::function<bool(T const &)> pred,
       T const &val) {
  return pred(val);
}

template<typename T,
         typename... Trest>
bool
all_of(std::function<bool(T const &)> pred,
       T const &val,
       Trest... rest) {
  return pred(val) && all_of(pred, rest...);
}

template<typename T,
         typename... Trest>
bool
none_of(std::function<bool(T const &)> pred,
        T const &val,
        Trest... rest) {
  return !any_of<T, Trest...>(pred, val, rest...);
}

template<typename T>
boost::optional<T>
first_of(std::function<bool(T const &)> pred,
       T const &val) {
  return pred(val) ? val : boost::optional<T>{};
}

template<typename T,
         typename... Trest>
boost::optional<T>
first_of(std::function<bool(T const &)> pred,
       T const &val,
       Trest... rest) {
  return pred(val) ? val : first_of(pred, rest...);
}

}

#endif  // MTX_COMMON_LIST_UTILS_H
