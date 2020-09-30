/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   regular expression helper functions

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#include <jpcre2.hpp>

namespace mtx::regex {

using jp = jpcre2::select<char>;

inline auto
match(std::string const &subject,
      jp::Regex const &regex,
      std::string const &modifier = {}) {
  return jp::RegexMatch()
    .setRegexObject(&regex)
    .setSubject(subject)
    .setModifier(modifier)
    .match();
}

inline auto
match(std::string const &subject,
      jp::VecNum &matches,
      jp::Regex const &regex,
      std::string const &modifier = {}) {
  return jp::RegexMatch()
    .setRegexObject(&regex)
    .setNumberedSubstringVector(&matches)
    .setSubject(subject)
    .setModifier(modifier)
    .match();
}

inline auto
match(std::string const &subject,
      jpcre2::VecOff &matches_start,
      jpcre2::VecOff &matches_end,
      jp::Regex const &regex,
      std::string const &modifier = {}) {
  return jp::RegexMatch()
    .setRegexObject(&regex)
    .setMatchStartOffsetVector(&matches_start)
    .setMatchEndOffsetVector(&matches_end)
    .setSubject(subject)
    .setModifier(modifier)
    .match();
}

inline auto
replace(std::string const &subject,
        jp::Regex const &regex,
        std::string const &modifier,
        std::string const &replacement) {
  return jp::RegexReplace{}
    .setRegexObject(&regex)
    .setSubject(subject)
    .setReplaceWith(replacement)
    .setModifier(modifier)
    .replace();
}

inline auto
replace(std::string const &subject,
        jp::Regex const &regex,
        std::string const &modifier,
        std::function<std::string(jp::NumSub const &)> const &formatter) {
  return jp::RegexReplace{}
    .setRegexObject(&regex)
    .setSubject(subject)
    .setModifier(modifier)
    .replace(jp::MatchEvaluator{[&formatter](jp::NumSub const &numbered, void *, void *) { return formatter(numbered); }});
}

inline auto
replace(std::string const &subject,
        jp::Regex const &regex,
        std::string const &modifier,
        std::function<std::string(jp::NumSub const &, jp::MapNas const &)> const &formatter) {
  return jp::RegexReplace{}
    .setRegexObject(&regex)
    .setSubject(subject)
    .setModifier(modifier)
    .replace(jp::MatchEvaluator{[&formatter](jp::NumSub const &numbered, jp::MapNas const &named, void *) { return formatter(numbered, named); }});
}

}
