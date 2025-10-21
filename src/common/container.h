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

template<typename,
         typename = void>
struct is_associative_container: std::false_type {
};

template<typename T>
struct is_associative_container<T, typename std::enable_if<sizeof(typename T::key_type) != 0>::type>: std::true_type {
};

template<typename Tcontainer,
         typename Tkey>
auto
includes(Tcontainer const &container,
         Tkey const &key)
  -> typename std::enable_if< is_associative_container<Tcontainer>::value , bool >::type
{
  return container.find(key) != container.end();
}

template<typename Tcontainer,
         typename Tkey>
auto
includes(Tcontainer const &container,
         Tkey const &key)
  -> typename std::enable_if< !is_associative_container<Tcontainer>::value , bool >::type
{
  return std::find(container.begin(), container.end(), key) != container.end();
}

template<typename Tcontainer>
auto
keys(Tcontainer const &container,
     typename std::enable_if< is_associative_container<Tcontainer>::value , bool >::type = {})
  -> std::vector< typename Tcontainer::key_type >
{
  std::vector< typename Tcontainer::key_type > the_keys;

  for (auto const &pair : container)
    the_keys.emplace_back(pair.first);

  return the_keys;
}

}
