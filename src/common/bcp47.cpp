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
#include "common/list_utils.h"
#include "common/qt.h"
#include "common/strings/formatting.h"
#include "common/strings/parsing.h"

namespace mtx::bcp47 {

bool language_c::ms_disabled                           = false;
normalization_mode_e language_c::ms_normalization_mode = normalization_mode_e::default_mode;

bool
operator <(language_c::extension_t const &a,
           language_c::extension_t const &b) {
  return mtx::string::to_lower_ascii(a.identifier) < mtx::string::to_lower_ascii(b.identifier);
}

language_c::extension_t::extension_t(std::string const &identifier_,
                                     std::vector<std::string> const &extensions_)
  : identifier{identifier_}
  , extensions{extensions_}
{
}

std::string
language_c::extension_t::format()
  const noexcept {
  if (identifier.empty() || extensions.empty())
    return {};
  return fmt::format("{0}-{1}", identifier, mtx::string::join(extensions, "-"));
}

bool
language_c::extension_t::operator ==(extension_t const &other)
  const noexcept {
  if (identifier != other.identifier)
    return false;

  return extensions == extensions;
}

bool
language_c::extension_t::operator !=(extension_t const &other)
  const noexcept {
  return !(*this == other);
}

// ------------------------------------------------------------

void
language_c::clear()
  noexcept {
  *this = mtx::bcp47::language_c{};
}

language_c
language_c::clone()
  const noexcept {
  return language_c{*this};
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

bool
language_c::has_valid_iso3166_1_alpha_2_or_top_level_domain_country_code()
  const noexcept {
  if (!m_valid || (m_region.size() != 2))
    return false;

  auto code = mtx::string::to_lower_ascii(m_region);

  if (   (code == "aa"s)
      || (code == "zz"s)
      || ((code[0] == 'q') && (code[1] >= 'm') && (code[1] <= 'z'))
      || ((code[0] == 'x') && (code[1] >= 'a') && (code[1] <= 'z')))
    return false;

  return true;
}

std::string
language_c::get_iso3166_1_alpha_2_code()
  const noexcept {
  if (has_valid_iso3166_1_alpha_2_or_top_level_domain_country_code())
    return mtx::string::to_upper_ascii(m_region);

  return {};
}

std::string
language_c::get_top_level_domain_country_code()
  const noexcept {
  auto code = mtx::string::to_lower_ascii(get_iso3166_1_alpha_2_code());
  return code == "gb"s ? "uk"s : code;
}

std::string const &
language_c::get_error()
  const noexcept {
  return m_parser_error;
}

std::string
language_c::dump()
  const noexcept{
  return fmt::format("[valid {0} language {1} extended_language_subtag {2} script {3} region {4} variants {5} extensions {6} private_use {7} grandfathered {8} parser_error {9}]",
                     m_valid, m_language, m_extended_language_subtag, m_script, m_region, m_variants, m_extensions, m_private_use, m_grandfathered, m_parser_error);
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

  if (!m_grandfathered.empty()) {
    auto idx = mtx::iana::language_subtag_registry::look_up_grandfathered(m_grandfathered);
    if (idx)
      return idx->code;
    return {};
  }

  auto output = mtx::string::to_lower_ascii(m_language);

  if (!m_extended_language_subtag.empty())
    output += fmt::format("-{}", mtx::string::to_lower_ascii(m_extended_language_subtag));

  if (!m_script.empty())
    output += fmt::format("-{}{}", mtx::string::to_upper_ascii(m_script.substr(0, 1)), mtx::string::to_lower_ascii(m_script.substr(1)));

  if (!m_region.empty())
    output += fmt::format("-{}", mtx::string::to_upper_ascii(m_region));

  for (auto const &variant : m_variants)
    output += fmt::format("-{}", mtx::string::to_lower_ascii(variant));

  for (auto const &extension : m_extensions)
    output += fmt::format("-{}", mtx::string::to_lower_ascii(extension.format()));

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
  if (!m_grandfathered.empty()) {
    auto idx = mtx::iana::language_subtag_registry::look_up_grandfathered(m_grandfathered);
    if (idx)
      return fmt::format("{0} ({1})", idx->description, idx->code);;
    return {};
  }

  auto formatted = format(force);

  if (formatted.empty())
    return formatted;

  std::string text;

  if (!get_language().empty()) {
    auto language_opt = mtx::iso639::look_up(get_language());
    if (language_opt)
      return fmt::format("{0} ({1})", gettext(language_opt->english_name.c_str()), formatted);
  }

  return formatted;
}

bool
language_c::parse_language(std::string const &code) {
  auto language = mtx::iso639::look_up(code);
  if (!language) {
    m_parser_error = fmt::format(FY("The value '{}' is not a valid ISO 639 language code."), code);
    return false;
  }

  m_language = !language->alpha_2_code.empty() ? language->alpha_2_code : language->alpha_3_code;

  return true;
}

bool
language_c::parse_script(std::string const &code) {
  auto script = mtx::iso15924::look_up(code);
  if (!script) {
    m_parser_error = fmt::format(FY("The value '{}' is not a valid ISO 15924 script code."), code);
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
      m_parser_error = fmt::format(FY("The value '{}' is not a valid ISO 3166-1 country code."), code);
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
    m_parser_error = fmt::format(FY("The value '{}' is not a valid UN M.49 country number code."), code);
    return false;
  }

  if (region->alpha_2_code.empty())
    m_region = fmt::format("{0:03}", region->number);
  else
    m_region = region->alpha_2_code;

  return true;
}

bool
language_c::parse_extlang(std::string const &str) {
  auto entry = mtx::iana::language_subtag_registry::look_up_extlang(str);

  if (!entry) {
    m_parser_error = fmt::format(FY("The value '{}' is not part of the IANA Language Subtag Registry for extended language subtags."), str);
    return false;
  }

  m_extended_language_subtag = entry->code;

  return true;
}

bool
language_c::parse_variants(std::string const &str) {
  for (auto const &code : mtx::string::split(str.substr(1), "-")) {
    auto entry = mtx::iana::language_subtag_registry::look_up_variant(code);

    if (!entry) {
      m_parser_error = fmt::format(FY("The value '{}' is not part of the IANA Language Subtag Registry for language variants."), code);
      return false;
    }

    m_variants.push_back(entry->code);
  }

  return true;
}

bool
language_c::parse_extensions(std::string const &str) {
  if (str.empty())
    return true;

  for (auto &part : mtx::string::split(mtx::string::to_lower_ascii(str.substr(1)), "-"s))
    if (part.size() == 1)
      m_extensions.emplace_back(part, std::vector<std::string>{});

    else
      m_extensions.back().extensions.emplace_back(part);

  return validate_extensions();
}

bool
language_c::matches_prefix(language_c const &prefix,
                           prefix_restrictions_t const &restrictions)
  const noexcept {

  if (   (restrictions.language                 && prefix.m_language                .empty() && !m_language                .empty())
      || (restrictions.extended_language_subtag && prefix.m_extended_language_subtag.empty() && !m_extended_language_subtag.empty())
      || (restrictions.script                   && prefix.m_script                  .empty() && !m_script                  .empty())
      || (restrictions.region                   && prefix.m_region                  .empty() && !m_region                  .empty())
      || (restrictions.variants                 && prefix.m_variants                .empty() && !m_variants                .empty()))
    return false;

  std::vector<std::string> this_relevant_parts;

  if (!prefix.m_language.empty())
    this_relevant_parts.emplace_back(m_language);

  if (!prefix.m_extended_language_subtag.empty())
    this_relevant_parts.emplace_back(m_extended_language_subtag);

  if (!prefix.m_script.empty())
    this_relevant_parts.emplace_back(m_script);

  if (!prefix.m_region.empty())
    this_relevant_parts.emplace_back(m_region);

  for (int idx = 0, num_variants = std::min<int>(prefix.m_variants.size(), m_variants.size()); idx < num_variants; ++idx)
    this_relevant_parts.emplace_back(m_variants[idx]);

  auto this_relevant_formatted = mtx::string::join(this_relevant_parts, "-");
  auto prefix_formatted        = prefix.format();

  if (this_relevant_formatted.size() < prefix_formatted.size())
    return false;

  this_relevant_formatted.resize(prefix_formatted.size());

  return balg::iequals(prefix_formatted, this_relevant_formatted);
}

bool
language_c::validate_prefixes(std::vector<std::string> const &prefixes)
  const noexcept {
  if (prefixes.empty())
    return true;

  prefix_restrictions_t restrictions;
  std::vector<language_c> parsed_prefixes;

  auto account = [](bool &value, bool is_unset) {
    if (!value && !is_unset)
      value = true;
  };

  for (auto const &prefix : prefixes) {
    parsed_prefixes.emplace_back(parse(prefix));
    auto const &tag = parsed_prefixes.back();

    account(restrictions.language,                 tag.m_language.empty());
    account(restrictions.extended_language_subtag, tag.m_extended_language_subtag.empty());
    account(restrictions.script,                   tag.m_script.empty());
    account(restrictions.region,                   tag.m_region.empty());
    account(restrictions.variants,                 tag.m_variants.empty());
  }

  for (auto const &parsed_prefix : parsed_prefixes)
    if (matches_prefix(parsed_prefix, restrictions))
      return true;

  return false;
}

std::string
language_c::get_first_variant_not_matching_prefixes()
  const noexcept {
  if (m_variants.empty())
    return {};

  for (auto const &variant_str : m_variants) {
    auto variant = mtx::iana::language_subtag_registry::look_up_variant(variant_str);

    if (!variant)               // Should not happen as the parsing checks this already.
      continue;

    if (variant->prefixes.empty())
      continue;

    if (!validate_prefixes(variant->prefixes))
      return variant_str;
  }

  return {};
}

bool
language_c::validate_extlang() {
  if (m_extended_language_subtag.empty())
    return true;

  auto extlang = mtx::iana::language_subtag_registry::look_up_extlang(m_extended_language_subtag);

  if (!extlang)                 // Should not happen as the parsing checks this already.
    return false;

  if (validate_prefixes(extlang->prefixes))
    return true;

  auto message   = Y("The extended language subtag '{}' must only be used with one of the following prefixes: {}.");
  m_parser_error = fmt::format(fmt::runtime(message), m_extended_language_subtag, fmt::join(extlang->prefixes, ", "));

  return false;
}

bool
language_c::validate_variants() {
  std::map<std::string, bool> variants_seen;

  for (auto const &variant : m_variants) {
    if (variants_seen[variant]) {
      m_parser_error = fmt::format(FY("The variant '{}' occurs more than once."), variant);
      return false;
    }

    variants_seen[variant] = true;
  }

  return true;
}

bool
language_c::validate_extensions() {
  if (m_extensions.empty())
    return true;

  if (m_language.empty()) {
    m_parser_error = Y("Extension subtags must follow at least a primary language subtag.");
    return false;
  }

  std::map<std::string, bool> identifiers_seen;

  for (auto const &extension : m_extensions) {
    if (identifiers_seen[extension.identifier]) {
      m_parser_error = Y("Each extension identifier must be used at most once.");
      return false;
    }

    identifiers_seen[extension.identifier] = true;

    // As of 2021-08-07 the IANA language tag extensions registry at
    // https://www.iana.org/assignments/language-tag-extensions-registry/language-tag-extensions-registry
    // only contains the following registered identifiers:
    if (!mtx::included_in(extension.identifier, "t"s, "u"s)) {
      m_parser_error = fmt::format(FY("The value '{0}' is not a registered IANA language tag identifier."), extension.identifier);
      return false;
    }
  }

  return true;
}

language_c
language_c::parse(std::string const &language,
                  normalization_mode_e normalization_mode) {
  init_re();

  language_c l;
  auto language_lower = mtx::string::to_lower_ascii(language);
  auto matches        = s_bcp47_grandfathered_re->match(Q(language_lower));

  if (matches.hasMatch()) {
    l.m_grandfathered = language;
    l.m_valid         = true;
    return l.normalize(normalization_mode);
  }

  matches = s_bcp47_re->match(Q(language_lower));

  if (!matches.hasMatch()) {
    l.m_parser_error = Y("The value does not adhere to the general structure of IETF BCP 47/RFC 5646 language tags.");
    return l;
  }

  // global private use
  if (matches.capturedLength(10)) {
    l.m_private_use = mtx::string::split(to_utf8(matches.captured(10)).substr(1), "-");
    l.m_valid       = true;
    return l.normalize(normalization_mode);
  }

  if (matches.capturedLength(1) && !l.parse_language(to_utf8(matches.captured(1))))
    return l;

  if (matches.capturedLength(2) && !l.parse_extlang(to_utf8(matches.captured(2))))
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

  if (matches.capturedLength(7) && !l.parse_variants(to_utf8(matches.captured(7))))
    return l;

  if (matches.capturedLength(8) && !l.parse_extensions(to_utf8(matches.captured(8))))
    return l;

  if (matches.capturedLength(9))
    l.m_private_use = mtx::string::split(to_utf8(matches.captured(9)).substr(1), "-");

  if (!l.validate_extlang() || !l.validate_variants())
    return l;

  l.m_valid = true;

  return l.normalize(normalization_mode);
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
language_c::get_closest_iso639_2_alpha_3_code()
  const noexcept {
  if (!m_valid || m_language.empty())
    return "und"s;

  auto language = mtx::iso639::look_up(m_language);
  if (!language)
    return "und"s;

  if (language->is_part_of_iso639_2)
    return language->alpha_3_code;

  auto extlang = mtx::iana::language_subtag_registry::look_up_extlang(language->alpha_3_code);
  if (!extlang || extlang->prefixes.empty())
    return "und"s;

  auto prefix_language = mtx::iso639::look_up(extlang->prefixes.front());

  if (prefix_language && prefix_language->is_part_of_iso639_2)
    return prefix_language->alpha_3_code;

  return "und"s;
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
language_c::set_extended_language_subtag(std::string const &extended_language_subtag) {
  m_extended_language_subtag = mtx::string::to_lower_ascii(extended_language_subtag);
  m_formatted_up_to_date     = false;

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
language_c::set_extensions(std::vector<extension_t> const &extensions) {
  m_extensions.clear();
  m_extensions.reserve(extensions.size());

  for (auto const &extension : extensions)
    add_extension(extension);

  return *this;
}

language_c &
language_c::add_extension(extension_t const &extension) {
  std::vector<std::string> extensions_lower;
  extensions_lower.reserve(extension.extensions.size());

  for (auto const &extension_subtag : extension.extensions)
    extensions_lower.emplace_back(mtx::string::to_lower_ascii(extension_subtag));

  m_extensions.emplace_back(mtx::string::to_lower_ascii(extension.identifier), extensions_lower);
  m_formatted_up_to_date = false;

  return *this;
}

language_c &
language_c::set_private_use(std::vector<std::string> const &private_use) {
  m_private_use          = mtx::string::to_lower_ascii(private_use);
  m_formatted_up_to_date = false;

  return *this;
}

language_c &
language_c::set_grandfathered(std::string const &grandfathered) {
  m_grandfathered        = grandfathered;
  m_formatted_up_to_date = false;

  return *this;
}

std::string const &
language_c::get_language()
  const noexcept {
  return m_language;
}

std::string const &
language_c::get_extended_language_subtag()
  const noexcept {
  return m_extended_language_subtag;
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

std::vector<language_c::extension_t> const &
language_c::get_extensions()
  const noexcept {
  return m_extensions;
}

std::vector<std::string> const &
language_c::get_private_use()
  const noexcept {
  return m_private_use;
}

std::string const &
language_c::get_grandfathered()
  const noexcept {
  return m_grandfathered;
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
language_c::matches(language_c const &match)
  const noexcept {
  if (!is_valid() || !match.is_valid())
    return false;

  if (!match.m_language.empty() && (m_language != match.m_language))
    return false;

  if (!match.m_extended_language_subtag.empty() && (m_extended_language_subtag != match.m_extended_language_subtag))
    return false;

  if (!match.m_script.empty() && (m_script != match.m_script))
    return false;

  if (!match.m_region.empty() && (m_region != match.m_region))
    return false;

  if (!match.m_variants.empty() && (m_variants != match.m_variants))
    return false;

  if (!match.m_extensions.empty() && (m_extensions != match.m_extensions))
    return false;

  if (!match.m_private_use.empty() && (m_private_use != match.m_private_use))
    return false;

  if (!match.m_grandfathered.empty() && (m_grandfathered != match.m_grandfathered))
    return false;

  return true;
}

language_c
language_c::find_best_match(std::vector<language_c> const &potential_matches)
  const noexcept {
  language_c best_match;
  auto num_components_best_match = 0;

  for (auto const &potential_match : potential_matches) {
    if (!matches(potential_match))
      continue;

    auto num_components = 1;
    auto formatted      = potential_match.format();

    for (auto const &chr : formatted)
      if (chr == '-')
        ++num_components;

    if (num_components > num_components_best_match) {
      best_match                = potential_match;
      num_components_best_match = num_components;
    }
  }

  return best_match;
}

language_c &
language_c::canonicalize_preferred_values() {
  auto &preferred_values = mtx::iana::language_subtag_registry::g_preferred_values;

  for (auto const &[match, preferred] : preferred_values) {
    if (!matches(match))
      continue;

     if (!preferred.m_language.empty()) {
       if (!match.m_language.empty())
         m_language.clear();

       if (!match.m_extended_language_subtag.empty())
         m_extended_language_subtag.clear();

       if (!match.m_script.empty())
         m_script.clear();

       if (!match.m_region.empty())
         m_region.clear();

       if (!match.m_variants.empty())
         m_variants.clear();

       if (!match.m_extensions.empty())
         m_extensions.clear();

       if (!match.m_private_use.empty())
         m_private_use.clear();

       if (!match.m_grandfathered.empty())
         m_grandfathered.clear();
     }

     if (!preferred.m_language.empty())
       m_language = preferred.m_language;

     if (!preferred.m_extended_language_subtag.empty())
       m_extended_language_subtag = preferred.m_extended_language_subtag;

     if (!preferred.m_script.empty())
       m_script = preferred.m_script;

     if (!preferred.m_region.empty())
       m_region = preferred.m_region;

     if (!preferred.m_variants.empty())
       m_variants = preferred.m_variants;

     if (!preferred.m_extensions.empty())
       m_extensions = preferred.m_extensions;

     if (!preferred.m_private_use.empty())
       m_private_use = preferred.m_private_use;

     if (!preferred.m_grandfathered.empty())
       m_grandfathered = preferred.m_grandfathered;
  }

  m_formatted_up_to_date = false;

  return *this;
}

language_c &
language_c::normalize(normalization_mode_e normalization_mode) {
  if (normalization_mode == normalization_mode_e::canonical)
    return to_canonical_form();

  if (normalization_mode == normalization_mode_e::extlang)
    return to_extlang_form();

  return *this;
}

language_c &
language_c::to_canonical_form() {
  m_formatted_up_to_date = false;

  std::sort(m_extensions.begin(), m_extensions.end());

  return canonicalize_preferred_values();
}

language_c &
language_c::to_extlang_form() {
  if (!m_valid)
    return *this;

  to_canonical_form();

  if (m_language.empty())
    return *this;

  auto extlang = mtx::iana::language_subtag_registry::look_up_extlang(m_language);

  if (!extlang || extlang->prefixes.empty())
    return *this;

  m_extended_language_subtag = m_language;
  m_language                 = extlang->prefixes[0];

  return *this;
}

bool
language_c::should_script_be_suppressed()
  const noexcept {
  if (m_script.empty())
    return false;

  auto check = [this](std::string const &code) -> bool {
    if (code.empty())
      return false;

    auto language = mtx::iso639::look_up(code);
    if (!language)
      return false;

    auto const &suppressions = mtx::iana::language_subtag_registry::g_suppress_scripts;
    auto itr                 = suppressions.find(language->alpha_3_code);

    if ((itr == suppressions.end()) && !language->alpha_2_code.empty())
      itr = suppressions.find(language->alpha_2_code);

    return (itr != suppressions.end()) && (mtx::string::to_lower_ascii(itr->second) == mtx::string::to_lower_ascii(m_script));
  };

  return check(m_language) || check(m_extended_language_subtag);
}

void
language_c::disable() {
  ms_disabled = true;
}

bool
language_c::is_disabled() {
  return ms_disabled;
}

void
language_c::set_normalization_mode(normalization_mode_e normalization_mode) {
  ms_normalization_mode = normalization_mode;
}

normalization_mode_e
language_c::get_normalization_mode() {
  return ms_normalization_mode;
}

} // namespace mtx::bcp47
