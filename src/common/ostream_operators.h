/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   generic "operator <<()" for standard types

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

namespace std {

template<typename CharT, typename TraitsT, typename ContT>
std::basic_ostream<CharT, TraitsT> &
operator <<(std::basic_ostream<CharT, TraitsT> &out,
            std::vector<ContT> const &vec) {
  out << "<";
  bool first = true;
  for (auto const &element : vec) {
    if (first)
      first = false;
    else
      out << ",";
    out << element;
  }
  out << ">";

  return out;
}

template<typename CharT, typename TraitsT, typename PairT1, typename PairT2>
std::basic_ostream<CharT, TraitsT> &
operator <<(std::basic_ostream<CharT, TraitsT> &out,
            std::pair<PairT1, PairT2> const &p) {
  out << "(" << p.first << "/" << p.second << ")";
  return out;
}

}
