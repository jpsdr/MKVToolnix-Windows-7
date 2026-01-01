/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Helper functions for containers

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include <functional>

#include "common/concepts.h"

namespace mtx {

// Support for hashing all scoped and unscoped enums via their
// underlying type.
template<typename Tkey>
struct hash {
  std::size_t operator()(Tkey const &key) const {
    using T = typename std::underlying_type<Tkey>::type;
    return std::hash<T>()(static_cast<T>(key));
  }
};

template<is_associative_container_cc Tcontainer,
         typename Tkey>
bool
includes(Tcontainer const &container,
         Tkey const &key) {
  return container.find(key) != container.end();
}

template<is_not_associative_container_cc Tcontainer,
         typename Tkey>
bool
includes(Tcontainer const &container,
         Tkey const &key) {
  return std::find(container.begin(), container.end(), key) != container.end();
}

template<is_associative_container_cc Tcontainer>
std::vector< typename Tcontainer::key_type >
keys(Tcontainer const &container) {
  std::vector< typename Tcontainer::key_type > the_keys;

  for (auto const &pair : container)
    the_keys.emplace_back(pair.first);

  return the_keys;
}

}
