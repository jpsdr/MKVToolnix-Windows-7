/** \brief command line parsing

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   \file

   \author Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/cli_parser.h"
#include "common/command_line.h"
#include "common/container.h"
#include "common/strings/editing.h"
#include "common/strings/formatting.h"
#include "common/translation.h"

namespace mtx::cli {

constexpr auto INDENT_COLUMN_OPTION_NAME        =  2;
constexpr auto INDENT_COLUMN_OPTION_DESCRIPTION = 30;
constexpr auto INDENT_COLUMN_SECTION_HEADER     =  1;

parser_c::option_t::option_t()
  : m_needs_arg{}
{
}

parser_c::option_t::option_t(parser_c::option_t::option_type_e type,
                             translatable_string_c description,
                             int indent)
  : m_type{type}
  , m_description{std::move(description)}
  , m_needs_arg{}
  , m_indent{indent}
{
}

parser_c::option_t::option_t(std::string name,
                             translatable_string_c description)
  : m_type{parser_c::option_t::ot_informational_option}
  , m_name{std::move(name)}
  , m_description{std::move(description)}
  , m_needs_arg{}
  , m_indent{INDENT_DEFAULT}
{
}

parser_c::option_t::option_t(std::string spec,
                             translatable_string_c description,
                             parser_cb_t callback,
                             bool needs_arg)
  : m_type{parser_c::option_t::ot_option}
  , m_spec{std::move(spec)}
  , m_description{std::move(description)}
  , m_callback{std::move(callback)}
  , m_needs_arg{needs_arg}
  , m_indent{INDENT_DEFAULT}
{
}

std::string
parser_c::option_t::format_text() {
  auto description = m_description.get_translated();
  if (description.empty())
    return {};

  if ((parser_c::option_t::ot_option == m_type) || (parser_c::option_t::ot_informational_option == m_type))
    return mtx::string::format_paragraph(description, INDENT_DEFAULT == m_indent ? INDENT_COLUMN_OPTION_DESCRIPTION : m_indent, std::string(INDENT_COLUMN_OPTION_NAME, ' ') + m_name);

  else if (parser_c::option_t::ot_section_header == m_type)
    return "\n"s + mtx::string::format_paragraph(description + ":", INDENT_DEFAULT == m_indent ? INDENT_COLUMN_SECTION_HEADER : m_indent);

  return mtx::string::format_paragraph(description, INDENT_DEFAULT == m_indent ? 0 : m_indent);
}

// ------------------------------------------------------------

parser_c::parser_c(std::vector<std::string> args)
  : m_args{std::move(args)}
  , m_no_common_cli_args{}
{
  m_hooks[parser_c::ht_common_options_parsed] = std::vector<parser_cb_t>();
  m_hooks[parser_c::ht_unknown_option]        = std::vector<parser_cb_t>();
}

void
parser_c::parse_args() {
  set_usage();
  while (!m_no_common_cli_args && mtx::cli::handle_common_args(m_args, ""))
    set_usage();

  run_hooks(parser_c::ht_common_options_parsed);

  parse_args_pass(true);
  parse_args_pass(false);
}

void
parser_c::parse_args_pass(bool first_pass) {
  for (auto sit = m_args.cbegin(), sit_end = m_args.cend(); sit != sit_end; sit++) {
    auto sit_next    = sit + 1;
    auto no_next_arg = sit_next == sit_end;
    m_current_arg    = *sit;
    m_next_arg       = !no_next_arg ? *sit_next : "";

    auto option_it = m_option_map.find(m_current_arg);
    if (option_it != m_option_map.end()) {
      auto &option = option_it->second;

      if (option.m_needs_arg) {
        if (no_next_arg)
          mxerror(fmt::format(FY("Missing argument to '{0}'.\n"), m_current_arg));
        ++sit;
      }

      if (   ( first_pass &&  m_parse_first[m_current_arg])
          || (!first_pass && !m_parse_first[m_current_arg]))
        option.m_callback();

    } else if (!first_pass && !run_hooks(parser_c::ht_unknown_option))
      mxerror(fmt::format(FY("Unknown option '{0}'.\n"), m_current_arg));
  }
}

void
parser_c::add_informational_option(std::string const &name,
                                   translatable_string_c description) {
  m_options.emplace_back(name, std::move(description));
}

void
parser_c::add_option(std::string const &spec,
                     parser_cb_t const &callback,
                     translatable_string_c description) {
  auto parts     = mtx::string::split(spec, "=", 2);
  auto needs_arg = parts.size() == 2;
  auto option    = parser_c::option_t{spec, std::move(description), callback, needs_arg};
  auto names     = mtx::string::split(parts[0], "|");

  for (auto &name : names) {
    auto full_name = '@' == name[0]       ? name
                   : 1   == name.length() ? std::string( "-") + name
                   :                        "--"s + name;

    if (mtx::includes(m_option_map, full_name))
      mxerror(fmt::format("parser_c::add_option(): Programming error: option '{0}' is already used for spec '{1}' and cannot be used for spec '{2}'.\n", full_name, m_option_map[full_name].m_spec, spec));

    m_option_map[full_name] = option;

    if (!option.m_name.empty())
      option.m_name += ", ";
    option.m_name += full_name;
  }

  if (needs_arg)
    option.m_name += " " + parts[1];

  m_options.push_back(option);
}

void
parser_c::add_section_header(translatable_string_c const &title,
                             int indent) {
  m_options.emplace_back(parser_c::option_t::ot_section_header, title, indent);
}

void
parser_c::add_information(translatable_string_c const &information,
                          int indent) {
  m_options.emplace_back(parser_c::option_t::ot_information, information, indent);
}

void
parser_c::add_separator() {
  m_options.emplace_back(parser_c::option_t::ot_information, translatable_string_c(""));
}

void
parser_c::add_common_options() {
  auto OPT = [this](char const *name, translatable_string_c const &description) {
    add_option(name, std::bind(&parser_c::dummy_callback, this), description);
  };

  OPT("v|verbose",                      YT("Increase verbosity."));
  OPT("q|quiet",                        YT("Suppress status output."));
  OPT("ui-language=<code>",             YT("Force the translations for 'code' to be used."));
  OPT("command-line-charset=<charset>", YT("Charset for strings on the command line"));
  OPT("output-charset=<cset>",          YT("Output messages in this charset"));
  OPT("r|redirect-output=<file>",       YT("Redirects all messages into this file."));
  OPT("no-bom",                         YT("Disables writing byte order markers to text files."));
  OPT("flush-on-close",                 YT("Flushes all cached data to storage when closing a file opened for writing."));
  OPT("abort-on-warnings",              YT("Aborts the program after the first warning is emitted."));
  OPT("@option-file.json",              YT("Reads additional command line options from the specified JSON file (see man page)."));
  OPT("h|help",                         YT("Show this help."));
  OPT("V|version",                      YT("Show version information."));
}

void
parser_c::set_usage() {
  mtx::cli::g_usage_text = "";
  for (auto &option : m_options)
    mtx::cli::g_usage_text += option.format_text();
}

void
parser_c::set_to_parse_first(std::vector<std::string> const &names) {
  for (auto const &name : names)
    m_parse_first[name] = true;
}

void
parser_c::set_to_parse_first(std::string const &name) {
  m_parse_first[name] = true;
}

void
parser_c::dummy_callback() {
}

void
parser_c::add_hook(parser_c::hook_type_e hook_type,
                   parser_cb_t const &callback) {
  m_hooks[hook_type].push_back(callback);
}

bool
parser_c::run_hooks(parser_c::hook_type_e hook_type) {
  if (m_hooks[hook_type].empty())
    return false;

  for (auto &hook : m_hooks[hook_type])
    if (hook)
      (hook)();

  return true;
}

}
