/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   BCP 47 language tags (regular expression)

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include <QRegularExpression>

#include "common/iana_language_subtag_registry.h"
#include "common/qt.h"
#include "common/strings/formatting.h"

namespace mtx::bcp47 {

std::optional<QRegularExpression> s_bcp47_re, s_bcp47_grandfathered_re;

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
    "        (?:-"              // 2: extlang
    "          ([a-z]{3})"
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
    "      )*"
    "    )"

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
    ")"
    "$"s;

  auto re_cleaned = Q(re_stringified).replace(QRegularExpression{" +|\n+"}, {});
  s_bcp47_re      = QRegularExpression{ re_cleaned };


  std::vector<std::string> grandfathered_list;
  grandfathered_list.reserve(mtx::iana::language_subtag_registry::g_grandfathered.size());

  for (auto const &entry : mtx::iana::language_subtag_registry::g_grandfathered)
    grandfathered_list.emplace_back(mtx::string::to_lower_ascii(entry.code));

  s_bcp47_grandfathered_re = QRegularExpression{ Q(fmt::format("^(?:{0})$", mtx::string::join(grandfathered_list, "|"))) };
}

} // namespace mtx::bcp47
