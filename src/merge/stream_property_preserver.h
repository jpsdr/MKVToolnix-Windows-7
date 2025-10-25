/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

template<typename T>
class stream_property_preserver_c {
private:
  std::deque<std::pair<T, std::optional<uint64_t>>> m_available_properties;

public:
  void
  add(T const &property,
      std::optional<uint64_t> position = std::nullopt) {
    m_available_properties.emplace_back(property, position);
  }

  void
  add_maybe(T const &property,
            std::optional<uint64_t> position = std::nullopt) {
    if (property != T{})
      add(property, position);
  }

  std::optional<T>
  get_next(std::optional<uint64_t> position = std::nullopt) {
    while (   !m_available_properties.empty()
           && position
           && m_available_properties.front().second
           && (*position < *m_available_properties.front().second))
      m_available_properties.pop_front();

    if (m_available_properties.empty())
      return {};

    auto property = m_available_properties.front().first;
    m_available_properties.pop_front();

    return property;
  }
};
