/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definition for item selector class

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/bcp47.h"
#include "common/container.h"

// pre-declare operator << so that the compiler knows it's a template
// function when it sees the friend declaration inside the class &
// generates the proper code for the linker.
template<typename T> class item_selector_c;
template<typename T> std::ostream & operator <<(std::ostream &out, item_selector_c<T> const &selector);

template<typename T>
class item_selector_c {
protected:
  T m_default_value;
  std::unordered_map<int64_t, T> m_items;
  std::unordered_map<mtx::bcp47::language_c, T> m_language_items;
  bool m_none{}, m_reversed{};

public:
  item_selector_c(T default_value = T{})
    : m_default_value(default_value)
  {
  }

  mtx::bcp47::language_c
  best_language_match(mtx::bcp47::language_c const &language)
    const {
    std::vector<mtx::bcp47::language_c> potential_matches;

    for (auto const &pair : m_language_items)
      potential_matches.emplace_back(pair.first);

    return language.find_best_match(potential_matches);
  }

  bool
  selected(int64_t item,
           mtx::bcp47::language_c const &language_item = {})
    const {
    if (m_none)
      return false;

    if (m_items.empty() && m_language_items.empty())
      return !m_reversed;

    auto matched_language = best_language_match(language_item);
    auto included         = (                            !m_items.empty()             && mtx::includes(m_items, item))
                         || (language_item.is_valid() &&  matched_language.is_valid() && mtx::includes(m_language_items, matched_language));
    return m_reversed ? !included : included;
  }

  T
  get(int64_t item,
      mtx::bcp47::language_c const &language_item = {})
    const {
    if (!selected(item, language_item))
      return m_default_value;

    if (!m_language_items.empty()) {
      auto matched_language = best_language_match(language_item);
      if (matched_language.is_valid() && mtx::includes(m_language_items, matched_language))
        return m_language_items.at(matched_language);

      return m_default_value;
    }

    return mtx::includes(m_items, item) ? m_items.at(item) : m_default_value;
  }

  bool none() const {
    return m_none;
  }

  void set_none() {
    m_none = true;
  }

  bool reversed() const {
    return m_reversed;
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

  std::vector<int64_t> item_ids() const {
    return mtx::keys(m_items);
  }

  friend std::ostream & operator << <>(std::ostream &out, item_selector_c<T> const &selector);
};

template<typename T>
std::ostream &
operator <<(std::ostream &out,
            item_selector_c<T> const &selector) {
  out << "<def:" << selector.m_default_value << " none:" << selector.m_none << " reversed:" << selector.m_reversed << " items:[";

  auto first = true;

  for (auto const &item : selector.m_items) {
    if (!first)
      out << " ";
    first = false;

    out << item.first << ":" << item.second;
  }

  out << "] lang_items:[";

  first = true;

  for (auto const &item : selector.m_language_items) {
    if (!first)
      out << " ";
    first = false;

    out << item.first << ":" << item.second;
  }

  out << "]>";

  return out;
}

#if FMT_VERSION >= 90000
template <typename T> struct fmt::formatter<item_selector_c<T>> : ostream_formatter {};
#endif  // FMT_VERSION >= 90000
