/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   JSON helper routines

   Written by Moritz Bunkus <moritz@bunkus.org>.
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

nlohmann::json::string_t
strip_comments(nlohmann::json::string_t const &data)  {
  mm_text_io_c in{std::make_shared<mm_mem_io_c>(reinterpret_cast<uint8_t const *>(data.c_str()), data.length())};
  mm_mem_io_c out{nullptr, data.length(), 100};

  auto state = parser_state_e::normal;

  in.set_byte_order_mark(byte_order_mark_e::utf8);

  try {
    while (!in.eof()) {
      auto codepoint = in.read_next_codepoint();
      if (codepoint.empty())
        break;

      if (state == parser_state_e::in_string_escaping) {
        out.write(codepoint);
        state = parser_state_e::in_string;
        continue;
      }

      if (state == parser_state_e::in_string) {
        out.write(codepoint);

        state = codepoint == "\\" ? parser_state_e::in_string_escaping
              : codepoint == "\"" ? parser_state_e::normal
              :                     parser_state_e::in_string;

        continue;
      }

      if (state == parser_state_e::in_comment) {
        if ((codepoint == "\r") || (codepoint == "\n")) {
          state = parser_state_e::normal;
          out.write(codepoint);
        }

        continue;
      }

      if (codepoint == "/") {
        try {
          auto next_codepoint = in.read_next_codepoint();
          if (next_codepoint == "/") {
            state = parser_state_e::in_comment;
            continue;
          }

          out.write(codepoint + next_codepoint);
          continue;

        } catch (mtx::mm_io::exception &) {
        }

      } else if (codepoint == "\"")
        state = parser_state_e::in_string;

      out.write(codepoint);
    }

  } catch (mtx::mm_io::exception &) {
  }

  return std::string{reinterpret_cast<char *>(out.get_buffer()), static_cast<std::string::size_type>(out.getFilePointer())};
}

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

  return nlohmann::json::parse(strip_comments(data), callback);
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
