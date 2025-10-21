/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   JSON helper routines

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include <clocale>

#include "common/at_scope_exit.h"
#include "common/json.h"
#include "common/mm_io_x.h"
#include "common/mm_mem_io.h"
#include "common/mm_proxy_io.h"
#include "common/mm_text_io.h"
#include "common/strings/utf8.h"

namespace mtx::json {

namespace {

enum class parser_state_e {
  normal,
  in_string,
  in_string_escaping,
  in_comment,
};

void
fix_invalid_utf8_recursively(nlohmann::json &json) {
  if (json.type() == nlohmann::json::value_t::string)
    json = mtx::utf8::fix_invalid(json.get<std::string>());

  else if (json.type() == nlohmann::json::value_t::array) {
    for (auto &sub_json : json)
      fix_invalid_utf8_recursively(sub_json);

  } else if (json.type() == nlohmann::json::value_t::object) {
    for (auto it = json.begin(), end = json.end(); it != end; ++it)
      fix_invalid_utf8_recursively(it.value());
  }
}

}

nlohmann::json
parse(nlohmann::json::string_t const &data,
      nlohmann::json::parser_callback_t callback) {
  auto old_locale = std::string{::setlocale(LC_NUMERIC, "C")};
  at_scope_exit_c restore_locale{ [&old_locale]() { ::setlocale(LC_NUMERIC, old_locale.c_str()); } };

  return nlohmann::json::parse(data, callback, true, true); // allow exceptions & comments
}

nlohmann::json::string_t
dump(nlohmann::json const &json,
     int indentation) {
  auto old_locale = std::string{::setlocale(LC_NUMERIC, "C")};
  at_scope_exit_c restore_locale{ [&old_locale]() { ::setlocale(LC_NUMERIC, old_locale.c_str()); } };

  auto json_fixed = json;
  fix_invalid_utf8_recursively(json_fixed);

  return json_fixed.dump(indentation);
}

} // namespace mtx::json
