/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   sorting functions

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/strings/parsing.h"
#include "common/strings/utf8.h"

namespace mtx::sort {

// --------------- sort by

template<  typename Titer
         , typename Tcriterion_maker
         , typename Tcriterion = typename std::result_of< Tcriterion_maker(typename std::iterator_traits<Titer>::value_type) >::type
         , typename Tcomparator = std::less<Tcriterion>
         >
void
by(Titer first,
   Titer last,
   Tcriterion_maker criterion_maker,
   Tcomparator comparator = Tcomparator{}) {
  using value_type     = typename std::iterator_traits<Titer>::value_type;
  using criterion_type = decltype( criterion_maker(*first) );
  using pair_type      = std::pair<value_type, criterion_type>;

  if (first == last)
    return;

  std::vector<pair_type> to_sort;
  to_sort.reserve(std::distance(first, last));

  for (auto idx = first; idx < last; ++idx)
    to_sort.push_back(std::make_pair(std::move(*idx), criterion_maker(*idx)));

  std::sort(to_sort.begin(), to_sort.end(), [&comparator](pair_type const &a, pair_type const &b) { return comparator(a.second, b.second); });

  std::transform(to_sort.begin(), to_sort.end(), first, [](pair_type &pair) -> value_type && { return std::move(pair.first); });
}

// --------------- sort naturally

template<typename StrT>
class natural_element_c {
private:
  StrT m_content;
  uint64_t m_number{};
  bool m_is_numeric{};

public:
  natural_element_c(StrT const &content)
    : m_content{content}
  {
    m_is_numeric = parse_number(m_content, m_number);
  }

  bool
  operator <(natural_element_c const &b)
    const {
    if ( m_is_numeric &&  b.m_is_numeric)
      return m_number < b.m_number;
    if (!m_is_numeric && !b.m_is_numeric)
      return m_content < b.m_content;
    return m_is_numeric;
  }

  bool
  operator ==(natural_element_c const &b)
    const {
    return m_content == b.m_content;
  }
};

template<typename StrT>
class natural_string_c {
private:
  StrT m_original;
  std::vector<natural_element_c<std::string> > m_parts;

public:
  natural_string_c(StrT const &original)
    : m_original{original}
  {
    static std::regex re("\\d+");

    auto utf8 = to_utf8(m_original);

    std::sregex_token_iterator it(utf8.begin(), utf8.end(), re, std::vector<int>{ -1, 0 });
    std::sregex_token_iterator end;
    while (it != end)
      m_parts.push_back(natural_element_c<std::string>{*it++});
  }

  StrT const &
  get_original()
    const {
    return m_original;
  }

  bool
  operator <(natural_string_c<StrT> const &b)
    const {
    auto min = std::min<int>(m_parts.size(), b.m_parts.size());
    for (int idx = 0; idx < min; ++idx)
      if (m_parts[idx] < b.m_parts[idx])
        return true;
      else if (!(m_parts[idx] == b.m_parts[idx]))
        return false;
    return m_parts.size() < b.m_parts.size();
  }
};

template<typename IterT>
void
naturally(IterT first,
          IterT last) {
  using StrT = typename std::iterator_traits<IterT>::value_type;
  by(first, last, [](StrT const &string) { return natural_string_c<StrT>{string}; });
}

}
