/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   BCP 47 language tags

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

namespace mtx::bcp47 {

enum class normalization_mode_e {
  none,
  canonical,
  extlang,
  default_mode = canonical,
};

class language_c {
public:
  struct extension_t {
    std::string identifier;
    std::vector<std::string> extensions;

    extension_t(std::string const &identifier_, std::vector<std::string> const &extensions_);

    std::string format() const noexcept;

    bool operator ==(extension_t const &other) const noexcept;
    bool operator !=(extension_t const &other) const noexcept;
  };

protected:
  struct prefix_restrictions_t {
    bool language{}, extended_language_subtag{}, script{}, region{}, variants{};
  };

  std::string m_language;                               // shortest ISO 639 code or reserved or registered language subtag
  std::string m_extended_language_subtag;               // selected ISO 639 codes
  std::string m_script;                                 // ISO 15924 code
  std::string m_region;                                 // either ISO 3166-1 code or UN M.49 code
  std::vector<std::string> m_variants;                  // registered variants
  std::vector<extension_t> m_extensions;
  std::vector<std::string> m_private_use;
  std::string m_grandfathered;

  bool m_valid{false};
  std::string m_parser_error;

  mutable std::string m_formatted;
  mutable bool m_formatted_up_to_date{};

  static bool ms_disabled;
  static normalization_mode_e ms_normalization_mode;

public:
  void clear() noexcept;
  language_c clone() const noexcept;

  bool has_valid_iso639_code() const noexcept;
  bool has_valid_iso639_2_code() const noexcept;
  std::string get_iso639_alpha_3_code() const noexcept;
  std::string get_closest_iso639_2_alpha_3_code() const noexcept;

  bool has_valid_iso3166_1_alpha_2_or_top_level_domain_country_code() const noexcept;
  std::string get_iso3166_1_alpha_2_code() const noexcept;
  std::string get_top_level_domain_country_code() const noexcept;

  std::string dump() const noexcept;
  std::string format(bool force = false) const noexcept;
  std::string format_long(bool force = false) const noexcept;
  bool is_valid() const noexcept;
  std::string const &get_error() const noexcept;

  bool operator ==(language_c const &other) const noexcept;
  bool operator !=(language_c const &other) const noexcept;

  bool matches(language_c const &match) const noexcept;
  language_c find_best_match(std::vector<language_c> const &potential_matches) const noexcept;

  language_c &normalize(normalization_mode_e normalization_mode);
  language_c &to_canonical_form();
  language_c &to_extlang_form();

  language_c &set_valid(bool valid);
  language_c &set_language(std::string const &language);
  language_c &set_extended_language_subtag(std::string const &extended_language_subtag);
  language_c &set_script(std::string const &script);
  language_c &set_region(std::string const &region);
  language_c &set_variants(std::vector<std::string> const &variants);
  language_c &set_extensions(std::vector<extension_t> const &extensions);
  language_c &set_private_use(std::vector<std::string> const &private_use);
  language_c &set_grandfathered(std::string const &grandfathered);

  language_c &add_extension(extension_t const &extensions);

  std::string const &get_language() const noexcept;
  std::string const &get_extended_language_subtag() const noexcept;
  std::string const &get_script() const noexcept;
  std::string const &get_region() const noexcept;
  std::vector<std::string> const &get_variants() const noexcept;
  std::vector<extension_t> const &get_extensions() const noexcept;
  std::vector<std::string> const &get_private_use() const noexcept;
  std::string const &get_grandfathered() const noexcept;

  std::string get_first_variant_not_matching_prefixes() const noexcept;
  bool should_script_be_suppressed() const noexcept;

protected:
  std::string format_internal(bool force) const noexcept;

  bool parse_language(std::string const &code);
  bool parse_extensions(std::string const &str);
  bool parse_script(std::string const &code);
  bool parse_region(std::string const &code);
  bool parse_extlang(std::string const &str);
  bool parse_variants(std::string const &str);

  bool validate_extensions();
  bool validate_extlang();
  bool validate_variants();
  bool validate_prefixes(std::vector<std::string> const &prefixes) const noexcept;
  bool matches_prefix(language_c const &prefix, prefix_restrictions_t const &restrictions) const noexcept;

  language_c &canonicalize_preferred_values();

public:
  static normalization_mode_e get_normalization_mode();
  static void set_normalization_mode(normalization_mode_e normalization_mode);

  static language_c parse(std::string const &language, normalization_mode_e normalization_mode = get_normalization_mode());

  static void disable();
  static bool is_disabled();
};

void init_re();

inline std::ostream &
operator<<(std::ostream &out,
           language_c::extension_t const &extension) {
  out << extension.format();
  return out;
}

inline std::ostream &
operator <<(std::ostream &out,
            language_c const &language) {
  out << language.format();
  return out;
}

inline bool
operator<(language_c const &a,
          language_c const &b) {
  return a.format() < b.format();
}

} // namespace mtx::bcp47

namespace std {

template<>
struct hash<mtx::bcp47::language_c> {
  std::size_t operator()(mtx::bcp47::language_c const &key) const {
    return std::hash<std::string>()(key.format());
  }
};

} // namespace mtx::bcp47

#if FMT_VERSION >= 90000
template <> struct fmt::formatter<mtx::bcp47::language_c::extension_t> : ostream_formatter {};
template <> struct fmt::formatter<mtx::bcp47::language_c>              : ostream_formatter {};
#endif  // FMT_VERSION >= 90000
