/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definition for item selector class

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/bcp47.h"
#include "common/container.h"

template<typename T>
class item_selector_c {
public:
  T m_default_value;
  std::unordered_map<int64_t, T> m_items;
  std::unordered_map<mtx::bcp47::language_c, T> m_language_items;
  bool m_none{}, m_reversed{};

public:
  item_selector_c(T default_value = T{})
    : m_default_value(default_value)
  {
  }

  bool
  selected(int64_t item,
           mtx::bcp47::language_c const &language_item = {})
    const {
    if (m_none)
      return false;

    if (m_items.empty() && m_language_items.empty())
      return !m_reversed;

    auto included = (                            !m_items.empty()          && mtx::includes(m_items, item))
                 || (language_item.is_valid() && !m_language_items.empty() && mtx::includes(m_language_items, language_item));
    return m_reversed ? !included : included;
  }

  T
  get(int64_t item,
      mtx::bcp47::language_c const &language_item = {})
    const {
    if (!selected(item, language_item))
      return m_default_value;

    if (!m_language_items.empty())
      return language_item.is_valid() && mtx::includes(m_language_items, language_item) ? m_language_items.at(language_item) : m_default_value;

    return mtx::includes(m_items, item) ? m_items.at(item) : m_default_value;
  }

  bool none() const {
    return m_none;
  }

  void set_none() {
    m_none = true;
  }

  void set_reversed() {
    m_reversed = true;
  }

  void add(int64_t item, T value = T{}) {
    m_items[item] = value;
  }

  void add(mtx::bcp47::language_c const &item, T value = T{}) {
    m_language_items[item] = value;
  }

  void clear() {
    m_items.clear();
    m_language_items.clear();
  }

  bool empty() const {
    return m_items.empty() && m_language_items.empty();
  }
};
