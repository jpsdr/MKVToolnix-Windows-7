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

#include <concepts>

namespace mtx {

template<typename T>
concept is_associative_container_cc = requires {
  sizeof(typename T::key_type) != 0;
};

template<typename T>
concept is_not_associative_container_cc = !is_associative_container_cc<T>;

} // mtx
