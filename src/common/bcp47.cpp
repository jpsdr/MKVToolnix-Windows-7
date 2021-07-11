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

#include <fmt/ranges.h>

#include "common/bcp47.h"
#include "common/bcp47_re.h"
#include "common/iana_language_subtag_registry.h"
#include "common/iso639.h"
#include "common/iso3166.h"
#include "common/iso15924.h"
#include "common/qt.h"
#include "common/strings/formatting.h"
#include "common/strings/parsing.h"

namespace mtx::bcp47 {

namespace {

bool
component_matches(std::string const &base,
                  std::string const &other) {
  return other.empty() || balg::iequals(base, other);
}

bool
components_match(std::vector<std::string> const &base,
                 std::vector<std::string> const &other) {
  for (auto const &one_other : other) {
    auto matches = false;

    for (auto const &one_base : base)
      if (balg::iequals(one_base, one_other)) {
        matches = true;
        break;
      }

    if (!matches)
      return false;
  }

  return true;
}

} // anonymous namespace

bool language_c::ms_disabled = false;

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

bool
language_c::has_valid_iso639_2_code()
  const noexcept {
  if (!m_valid || m_language.empty())
    return false;

  auto language_opt = mtx::iso639::look_up(get_language());
  return language_opt && language_opt->is_part_of_iso639_2;
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
    if (!output.empty())
      output += "-";

    output += "x";

    for (auto const &private_use : m_private_use)
      output += fmt::format("-{}", mtx::string::to_lower_ascii(private_use));
  }

  return output;
}

std::string
language_c::format_long(bool force)
  const noexcept {
  auto formatted = format(force);

  if (formatted.empty())
    return formatted;

  std::string text;

  if (!get_language().empty()) {
    auto language_opt = mtx::iso639::look_up(get_language());
    if (language_opt)
      return fmt::format("{0} ({1})", language_opt->english_name, formatted);
  }

  return formatted;
}

bool
language_c::parse_language(std::string const &code) {
  auto language = mtx::iso639::look_up(code);
  if (!language) {
    m_parser_error = fmt::format(Y("The value '{}' is not a valid ISO 639 language code."), code);
    return false;
  }

  m_language = !language->alpha_2_code.empty() ? language->alpha_2_code : language->alpha_3_code;

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

  auto normalized_code = to_utf8(Q(code).replace(QRegularExpression{"^0+"}, {}));
  if (normalized_code.empty())
    normalized_code = "0";

  auto number = 0u;
  mtx::string::parse_number(normalized_code, number);

  auto region = mtx::iso3166::look_up(number);
  if (!region) {
    m_parser_error = fmt::format(Y("The value '{}' is not a valid UN M.49 country number code."), code);
    return false;
  }

  if (region->alpha_2_code.empty())
    m_region = fmt::format("{0:03}", region->number);
  else
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

    if (is_extlangs)
      m_extended_language_subtags.push_back(entry->code);
    else
      m_variants.push_back(entry->code);
  }

  return true;
}

bool
language_c::validate_one_extlang_or_variant(std::string const &code,
                                            bool is_extlang) {
  auto entry = is_extlang ? mtx::iana::language_subtag_registry::look_up_extlang(code)
             :              mtx::iana::language_subtag_registry::look_up_variant(code);

  if (!entry)                   // Should not happen as the parsing checks this already.
    return false;

  if (entry->prefixes.empty())
    return true;

  for (auto const &prefix : entry->prefixes)
    if (matches(parse(prefix)))
      return true;

  auto message   = is_extlang ? Y("The extended language subtag '{}' must only be used with one of the following prefixes: {}.")
                 :              Y("The variant '{}' must only be used with one of the following prefixes: {}.");
  m_parser_error = fmt::format(message, code, fmt::join(entry->prefixes, ", "));
  return false;
}

bool
language_c::validate_extlangs_or_variants(std::vector<std::string> const &codes,
                                          bool is_extlangs) {
  for (auto const &code : codes)
    if (!validate_one_extlang_or_variant(code, is_extlangs))
      return false;

  return true;
}

language_c
language_c::parse(std::string const &language) {
  init_re();

  language_c l;
  auto language_lower = mtx::string::to_lower_ascii(language);
  auto matches        = s_bcp47_re->match(Q(language_lower));

  if (!matches.hasMatch()) {
    l.m_parser_error = Y("The value does not adhere to the general structure of IETF BCP 47/RFC 5646 language tags.");
    return l;
  }

  // global private use
  if (matches.capturedLength(10)) {
    l.m_private_use = mtx::string::split(to_utf8(matches.captured(10)).substr(1), "-");
    l.m_valid       = true;
    return l;
  }

  if (matches.capturedLength(1) && !l.parse_language(to_utf8(matches.captured(1))))
    return l;

  if (matches.capturedLength(2) && !l.parse_extlangs_or_variants(to_utf8(matches.captured(2)), true))
    return l;

  if (matches.capturedLength(3)) {
    l.m_parser_error = Y("Four-letter language codes are reserved for future use and not supported.");
    return l;
  }

  if (matches.capturedLength(4)) {
    l.m_parser_error = Y("Five- to eight-letter language codes are currently not supported.");
    return l;
  }

  if (matches.capturedLength(5) && !l.parse_script(to_utf8(matches.captured(5))))
    return l;

  if (matches.capturedLength(6) && !l.parse_region(to_utf8(matches.captured(6))))
    return l;

  if (matches.capturedLength(7) && !l.parse_extlangs_or_variants(to_utf8(matches.captured(7)), false))
    return l;

  if (matches.capturedLength(8)) {
    l.m_parser_error = Y("Language tag extensions are currently not supported.");
    return l;
  }

  if (matches.capturedLength(9))
    l.m_private_use = mtx::string::split(to_utf8(matches.captured(9)).substr(1), "-");

  if (   !l.validate_extlangs_or_variants(l.m_extended_language_subtags, true)
      || !l.validate_extlangs_or_variants(l.m_variants,                  false))
    return l;

  l.m_valid = true;

  return l;
}

std::string
language_c::get_iso639_alpha_3_code()
  const noexcept {
  if (!has_valid_iso639_code())
    return {};

  auto language = mtx::iso639::look_up(m_language);
  if (language)
    return language->alpha_3_code;

  return {};
}

std::string
language_c::get_iso639_2_alpha_3_code_or(std::string const &value_if_invalid)
  const noexcept {
  if (!m_valid || m_language.empty())
    return value_if_invalid;

  auto language = mtx::iso639::look_up(m_language);
  if (language && language->is_part_of_iso639_2)
    return language->alpha_3_code;

  return value_if_invalid;
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

bool
language_c::matches(language_c const &other)
  const noexcept {
  if (!component_matches(m_language, other.m_language))
    return false;

  if (!components_match(m_extended_language_subtags, other.m_extended_language_subtags))
    return false;

  if (!component_matches(m_script, other.m_script))
    return false;

  if (!component_matches(m_region, other.m_region))
    return false;

  if (!components_match(m_variants, other.m_variants))
    return false;

  if (!components_match(m_extensions, other.m_extensions))
    return false;

  if (!components_match(m_private_use, other.m_private_use))
    return false;

  return true;
}

void
language_c::disable() {
  ms_disabled = true;
}

bool
language_c::is_disabled() {
  return ms_disabled;
}

} // namespace mtx::bcp47
