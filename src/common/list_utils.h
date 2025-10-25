/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   list utility functions

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

namespace mtx {

template<typename Tneedle,
         typename Tvalue>
bool
included_in(Tneedle const &needle,
            Tvalue const &val) {
  return needle == val;
}

template<typename Tneedle,
         typename Tvalue,
         typename... Trest>
bool
included_in(Tneedle const &needle,
            Tvalue const &val,
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
std::optional<T>
first_of(std::function<bool(T const &)> pred,
         T const &val) {
  return pred(val) ? val : std::optional<T>{};
}

// Versions for containers

template<typename T,
         typename... Trest>
std::optional<T>
first_of(std::function<bool(T const &)> pred,
         T const &val,
         Trest... rest) {
  return pred(val) ? val : first_of(pred, rest...);
}

template<typename Tcontainer,
         typename Tpredicate> bool
any(Tcontainer container,
    Tpredicate predicate) {
  return std::find_if(std::begin(container), std::end(container), predicate) != std::end(container);
}

template<typename Tcontainer,
         typename Tpredicate> bool
none(Tcontainer container,
     Tpredicate predicate) {
  return !any(container, predicate);
}

template<typename Tcontainer,
         typename Tpredicate> bool
all(Tcontainer container,
    Tpredicate predicate) {
  return std::find_if_not(std::begin(container), std::end(container), predicate) == std::end(container);
}

}
