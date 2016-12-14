/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   JSON helper routines

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include <locale.h>

#include "common/at_scope_exit.h"
#include "common/json.h"

namespace mtx { namespace json {

nlohmann::json
parse(nlohmann::json::string_t const &data,
      nlohmann::json::parser_callback_t callback) {
  auto old_locale = std::string{::setlocale(LC_NUMERIC, "C")};
  at_scope_exit_c restore_locale{ [&old_locale]() { ::setlocale(LC_NUMERIC, old_locale.c_str()); } };

  return nlohmann::json::parse(data, callback);
}

nlohmann::json::string_t
dump(nlohmann::json const &json,
     int indentation) {
  auto old_locale = std::string{::setlocale(LC_NUMERIC, "C")};
  at_scope_exit_c restore_locale{ [&old_locale]() { ::setlocale(LC_NUMERIC, old_locale.c_str()); } };

  return json.dump(indentation);
}

}} // namespace mtx::json
