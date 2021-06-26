/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   BCP 47 language tags (regular expression)

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include <QRegularExpression>

#include "common/qt.h"

namespace mtx::bcp47 {

std::optional<QRegularExpression> s_bcp47_re;

void
init_re() {
  if (s_bcp47_re)
    return;

  auto re_stringified = ""
    "^"
    "(?:"
    "  (?:"
    "    (?:"
    "      (?:"
    "        ("                 // 1: language3
    "          [a-z]{2,3}"
    "        )"
    "        ("                 // 2: extlangs
    "          (?:-[a-z]{3}){0,3}"
    "        )?"
    "      )"
    "      |"
    "      ("                   // 3: language4
    "        [a-z]{4}"
    "      )"
    "      |"
    "      ("                   // 4: language5-8
    "        [a-z]{5,8}"
    "      )"
    "    )"

    "    (?:-"
    "      ([a-z]{4})"          // 5: script
    "    )?"

    "    (?:-"
    "      ("                   // 6: region
    "        [a-z]{2}"
    "        |"
    "        [0-9]{3}"
    "      )"
    "    )?"

    "    ("                     // 7: variants
    "      (?:-"
    "        (?:"
    "          [a-z0-9]{5,8}"
    "          |"
    "          [0-9][a-z0-9]{3}"
    "        )"
    "      )*"
    "    )"

    "    ("                     // 8: extensions
    "      (?:-"
    "        [a-wyz0-9]"
    "        (?:-"
    "          [a-z0-9]{2,8}"
    "        )+"
    "      )"
    "    )*"

    "    (?:-x"
    "      ("                   // 9: privateuse
    "        (?:-"
    "          [a-z0-9]{1,8}"
    "        )+"
    "      )"
    "    )?"
    "  )"
    "  |"
    "  (?:x"
    "    ("                     // 10: privateuse global
    "      (?:-"
    "        [a-z0-9]{1,8}"
    "      )+"
    "    )"
    "  )"
    // "  |"
    // "  ("                       // 11: grandfathered
    // "    en-GB-oed |"           // irregular, ungrouped
    // "    i-ami |"
    // "    i-bnn |"
    // "    i-default |"
    // "    i-enochian |"
    // "    i-hak |"
    // "    i-klingon |"
    // "    i-lux |"
    // "    i-mingo |"
    // "    i-navajo |"
    // "    i-pwn |"
    // "    i-tao |"
    // "    i-tay |"
    // "    i-tsu |"
    // "    sgn-BE-FR |"
    // "    sgn-BE-NL |"
    // "    sgn-CH-DE |"
    // "    art-lojban |"          // regular, ungrouped
    // "    cel-gaulish |"
    // "    no-bok |"
    // "    no-nyn |"
    // "    zh-guoyu |"
    // "    zh-hakka |"
    // "    zh-min |"
    // "    zh-min-nan |"
    // "    zh-xiang"
    // "  )"
    ")"
    "$"s;

  auto re_cleaned = Q(re_stringified).replace(QRegularExpression{" +|\n+"}, {});
  s_bcp47_re      = QRegularExpression{ re_cleaned };
}

} // namespace mtx::bcp47
