/*
   mkvpropedit -- utility for editing properties of existing Matroska files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#ifndef MTX_COMMON_CLI_PARSER_H
#define MTX_COMMON_CLI_PARSER_H

#include "common/common_pch.h"

#include "common/translation.h"

#define INDENT_DEFAULT -1

using cli_parser_cb_t = std::function<void(void)>;

class cli_parser_c {
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
    cli_parser_cb_t m_callback;
    bool m_needs_arg;
    int m_indent;

    option_t();
    option_t(option_type_e type, translatable_string_c const &description, int indent = INDENT_DEFAULT);
    option_t(std::string const &spec, translatable_string_c const &description, cli_parser_cb_t const &callback, bool needs_arg);
    option_t(std::string const &name, translatable_string_c const &description);

    std::string format_text();
  };

  enum hook_type_e {
    ht_common_options_parsed,
    ht_unknown_option,
  };

  std::map<std::string, option_t> m_option_map;
  std::vector<option_t> m_options;
  std::vector<std::string> m_args;

  std::string m_current_arg, m_next_arg;

  std::map<hook_type_e, std::vector<cli_parser_cb_t>> m_hooks;

  bool m_no_common_cli_args;

protected:
  cli_parser_c(std::vector<std::string> const &args);

  void add_option(std::string const &spec, cli_parser_cb_t const &callback, translatable_string_c const &description);
  void add_informational_option(std::string const &name, translatable_string_c const &description);
  void add_section_header(translatable_string_c const &title, int indent = INDENT_DEFAULT);
  void add_information(translatable_string_c const &information, int indent = INDENT_DEFAULT);
  void add_separator();
  void add_common_options();

  void parse_args();
  void set_usage();

  void dummy_callback();

  void add_hook(hook_type_e hook_type, cli_parser_cb_t const &callback);
  bool run_hooks(hook_type_e hook_type);
};

#endif // MTX_COMMON_CLI_PARSER_H
