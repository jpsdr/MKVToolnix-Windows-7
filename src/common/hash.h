/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   Hash/unordered map helper functions

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#ifndef MTX_COMMON_HASH_H
#define MTX_COMMON_HASH_H

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

}

#endif  // MTX_COMMON_HASH_H
