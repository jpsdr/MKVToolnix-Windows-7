/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   BCP 47 language tags

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include "common/bcp47.h"
#include "common/bcp47_re.h"
#include "common/iana_language_subtag_registry.h"
#include "common/iso639.h"
#include "common/iso3166.h"
#include "common/iso15924.h"
#include "common/strings/formatting.h"
#include "common/strings/parsing.h"

#include <fmt/ranges.h>

namespace mtx::bcp47 {

void
language_c::clear()
  noexcept {
  *this = mtx::bcp47::language_c{};
}

bool
language_c::is_valid()
  const noexcept {
  return m_valid;
}

bool
language_c::has_valid_iso639_code()
  const noexcept {
  return m_valid && !m_language.empty();
}

std::string const &
language_c::get_error()
  const noexcept {
  return m_parser_error;
}

std::string
language_c::dump()
  const noexcept{
  return fmt::format("[valid {0} language {1} extended_language_subtags {2} script {3} region {4} variants {5} extensions {6} private_use {7} parser_error {8}]",
                     m_valid, m_language, m_extended_language_subtags, m_script, m_region, m_variants, m_extensions, m_private_use, m_parser_error);
}

std::string
language_c::format(bool force)
  const noexcept{
  if (force)
    return format_internal(true);

  if (!m_formatted_up_to_date) {
    m_formatted            = format_internal(force);
    m_formatted_up_to_date = true;
  }

  return m_formatted;
}

std::string
language_c::format_internal(bool force)
  const noexcept {
  if (!m_valid && !force)
    return {};

  if (m_language.empty()) {
    auto output = "x"s;

    for (auto const &private_use : m_private_use)
      output += fmt::format("-{}", mtx::string::to_lower_ascii(private_use));

    return output;
  }

  auto output = mtx::string::to_lower_ascii(m_language);

  for (auto const &subtag : m_extended_language_subtags)
    output += fmt::format("-{}", mtx::string::to_lower_ascii(subtag));

  if (!m_script.empty())
    output += fmt::format("-{}{}", mtx::string::to_upper_ascii(m_script.substr(0, 1)), mtx::string::to_lower_ascii(m_script.substr(1)));

  if (!m_region.empty())
    output += fmt::format("-{}", mtx::string::to_upper_ascii(m_region));

  for (auto const &variant : m_variants)
    output += fmt::format("-{}", mtx::string::to_lower_ascii(variant));

  for (auto const &extension : m_extensions)
    output += fmt::format("-{}", mtx::string::to_lower_ascii(extension));

  if (!m_private_use.empty()) {
    output += "-x";
    for (auto const &private_use : m_private_use)
      output += fmt::format("-{}", mtx::string::to_lower_ascii(private_use));
  }

  return output;
}

bool
language_c::parse_language(std::string const &code) {
  auto language = mtx::iso639::look_up(code);
  if (!language) {
    m_parser_error = fmt::format(Y("The value '{}' is not a valid ISO 639 language code."), code);
    return false;
  }

  m_language = !language->iso639_1_code.empty() ? language->iso639_1_code : language->iso639_2_code;

  return true;
}

bool
language_c::parse_script(std::string const &code) {
  auto script = mtx::iso15924::look_up(code);
  if (!script) {
    m_parser_error = fmt::format(Y("The value '{}' is not a valid ISO 15924 script code."), code);
    return false;
  }

  m_script = script->code;

  return true;
}

bool
language_c::parse_region(std::string const &code) {
  if (code.length() == 2) {
    auto region = mtx::iso3166::look_up(code);
    if (!region) {
      m_parser_error = fmt::format(Y("The value '{}' is not a valid ISO 3166-1 country code."), code);
      return false;
    }

    m_region = region->alpha_2_code;

    return true;
  }

  auto normalized_code = mtx::regex::replace(code, mtx::regex::jp::Regex{"^0+"}, "", "");
  if (normalized_code.empty())
    normalized_code = "0";

  auto number = 0u;
  mtx::string::parse_number(normalized_code, number);

  auto region = mtx::iso3166::look_up(number);
  if (!region) {
    m_parser_error = fmt::format(Y("The value '{}' is not a valid UN M.49 country number code."), code);
    return false;
  }

  m_region = region->alpha_2_code;

  return true;
}

bool
language_c::parse_extlangs_or_variants(std::string const &str,
                                       bool is_extlangs) {
  auto const current_str = mtx::string::to_lower_ascii(format_internal(true));

  for (auto const &code : mtx::string::split(str.substr(1), "-")) {
    auto entry = is_extlangs ? mtx::iana::language_subtag_registry::look_up_extlang(code)
               :               mtx::iana::language_subtag_registry::look_up_variant(code);

    if (!entry) {
      auto message   = is_extlangs ? Y("The value '{}' is not part of the IANA Language Subtag Registry for extended language subtags.")
                     :               Y("The value '{}' is not part of the IANA Language Subtag Registry for language variants.");
      m_parser_error = fmt::format(message, code);
      return false;
    }

    if (!entry->prefixes.empty()) {
      auto ok = false;

      for (auto const &prefix : entry->prefixes)
        if (current_str == mtx::string::to_lower_ascii(prefix)) {
          ok = true;
          break;
        }

      if (!ok) {
        auto message   = is_extlangs ? Y("The extended language subtag '{}' must only be used with one of the following prefixes: {}.")
                       :               Y("The variant '{}' must only be used with one of the following prefixes: {}.");
        m_parser_error = fmt::format(message, code, fmt::join(entry->prefixes, ", "));
        return false;
      }
    }

    if (is_extlangs)
      m_extended_language_subtags.push_back(entry->code);
    else
      m_variants.push_back(entry->code);
  }

  return true;
}

language_c
language_c::parse(std::string const &language) {
  init_re();

  mtx::regex::jp::VecNum matches;
  language_c l;
  auto language_lower = mtx::string::to_lower_ascii(language);

  if (!mtx::regex::match(language_lower, matches, *s_bcp47_re) || (matches[0].size() != 11)) {
    l.m_parser_error = Y("The value does not adhere to the general structure of IETF BCP 47/RFC 5646 language tags.");
    return l;
  }

  // global private use
  if (!matches[0][10].empty()) {
    l.m_private_use = mtx::string::split(matches[0][10].substr(1), "-");
    l.m_valid       = true;
    return l;
  }

  if (!matches[0][1].empty() && !l.parse_language(matches[0][1]))
    return l;

  if (!matches[0][2].empty() && !l.parse_extlangs_or_variants(matches[0][2], true))
    return l;

  if (!matches[0][3].empty()) {
    l.m_parser_error = Y("Four-letter language codes are reserved for future use and not supported.");
    return l;
  }

  if (!matches[0][4].empty()) {
    l.m_parser_error = Y("Five- to eight-letter language codes are currently not supported.");
    return l;
  }

  if (!matches[0][5].empty() && !l.parse_script(matches[0][5]))
    return l;

  if (!matches[0][6].empty() && !l.parse_region(matches[0][6]))
    return l;

  if (!matches[0][7].empty() && !l.parse_extlangs_or_variants(matches[0][7], false))
    return l;

  if (!matches[0][8].empty()) {
    l.m_parser_error = Y("Language tag extensions are currently not supported.");
    return l;
  }

  if (!matches[0][9].empty())
    l.m_private_use = mtx::string::split(matches[0][9].substr(1), "-");

  l.m_valid = true;

  return l;
}

std::string
language_c::get_iso639_2_code()
  const noexcept {
  if (!has_valid_iso639_code())
    return {};

  auto language = mtx::iso639::look_up(m_language);
  if (language)
    return language->iso639_2_code;

  return {};
}

std::string
language_c::get_iso639_2_code_or(std::string const &value_if_invalid)
  const noexcept {
  auto code = get_iso639_2_code();
  return !code.empty() ? code : value_if_invalid;
}

language_c &
language_c::set_valid(bool valid) {
  m_valid                = valid;
  m_formatted_up_to_date = false;

  return *this;
}

language_c &
language_c::set_language(std::string const &language) {
  m_language             = mtx::string::to_lower_ascii(language);
  m_formatted_up_to_date = false;

  return *this;
}

language_c &
language_c::set_extended_language_subtags(std::vector<std::string> const &extended_language_subtags) {
  m_extended_language_subtags = mtx::string::to_lower_ascii(extended_language_subtags);
  m_formatted_up_to_date      = false;

  return *this;
}

language_c &
language_c::set_script(std::string const &script) {
  m_script               = script;
  m_formatted_up_to_date = false;

  return *this;
}

language_c &
language_c::set_region(std::string const &region) {
  m_region               = region;
  m_formatted_up_to_date = false;

  return *this;
}

language_c &
language_c::set_variants(std::vector<std::string> const &variants) {
  m_variants             = mtx::string::to_lower_ascii(variants);
  m_formatted_up_to_date = false;

  return *this;
}

language_c &
language_c::set_extensions(std::vector<std::string> const &extensions) {
  m_extensions           = mtx::string::to_lower_ascii(extensions);
  m_formatted_up_to_date = false;

  return *this;
}

language_c &
language_c::set_private_use(std::vector<std::string> const &private_use) {
  m_private_use          = mtx::string::to_lower_ascii(private_use);
  m_formatted_up_to_date = false;

  return *this;
}

std::string const &
language_c::get_language()
  const noexcept {
  return m_language;
}

std::vector<std::string> const &
language_c::get_extended_language_subtags()
  const noexcept {
  return m_extended_language_subtags;
}

std::string const &
language_c::get_script()
  const noexcept {
  return m_script;
}

std::string const &
language_c::get_region()
  const noexcept {
  return m_region;
}

std::vector<std::string> const &
language_c::get_variants()
  const noexcept {
  return m_variants;
}

std::vector<std::string> const &
language_c::get_extensions()
  const noexcept {
  return m_extensions;
}

std::vector<std::string> const &
language_c::get_private_use()
  const noexcept {
  return m_private_use;
}

bool
language_c::operator ==(language_c const &other)
  const noexcept {
  return format() == other.format();
}

bool
language_c::operator !=(language_c const &other)
  const noexcept {
  return format() != other.format();
}

} // namespace mtx::bcp47
