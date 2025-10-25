/*
   mkvpropedit -- utility for editing properties of existing Matroska files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/translation.h"

namespace mtx::cli {

constexpr auto INDENT_DEFAULT = -1;

using parser_cb_t = std::function<void(void)>;

class parser_c {
protected:
  struct option_t {
    enum option_type_e {
      ot_option,
      ot_section_header,
      ot_information,
      ot_informational_option,
    };

    option_type_e m_type;
    std::string m_spec, m_name;
    translatable_string_c m_description;
    parser_cb_t m_callback;
    bool m_needs_arg;
    int m_indent;

    option_t();
    option_t(option_type_e type, translatable_string_c description, int indent = INDENT_DEFAULT);
    option_t(std::string spec, translatable_string_c description, parser_cb_t callback, bool needs_arg);
    option_t(std::string name, translatable_string_c description);

    std::string format_text();
  };

  enum hook_type_e {
    ht_common_options_parsed,
    ht_unknown_option,
  };

  std::map<std::string, option_t> m_option_map;
  std::vector<option_t> m_options;
  std::vector<std::string> m_args;
  std::unordered_map<std::string, bool> m_parse_first;

  std::string m_current_arg, m_next_arg;

  std::map<hook_type_e, std::vector<parser_cb_t>> m_hooks;

  bool m_no_common_cli_args;

protected:
  parser_c(std::vector<std::string> args);

  void add_option(std::string const &spec, parser_cb_t const &callback, translatable_string_c description);
  void add_informational_option(std::string const &name, translatable_string_c description);
  void add_section_header(translatable_string_c const &title, int indent = INDENT_DEFAULT);
  void add_information(translatable_string_c const &information, int indent = INDENT_DEFAULT);
  void add_separator();
  void add_common_options();

  void parse_args();
  void parse_args_pass(bool first_pass);
  void set_usage();
  void set_to_parse_first(std::vector<std::string> const &names);
  void set_to_parse_first(std::string const &name);

  void dummy_callback();

  void add_hook(hook_type_e hook_type, parser_cb_t const &callback);
  bool run_hooks(hook_type_e hook_type);
};

}
