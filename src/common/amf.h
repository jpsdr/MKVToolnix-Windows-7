/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   helper functions for AMF (Action Message Format) data

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include <unordered_map>
#include <variant>

#include "common/mm_mem_io.h"

namespace mtx::amf {

using value_type_t = std::variant<double, bool, std::string>;
using meta_data_t  = std::unordered_map<std::string, value_type_t>;

class script_parser_c {
public:
  enum data_type_e {
    TYPE_NUMBER      = 0x00,
    TYPE_BOOL        = 0x01,
    TYPE_STRING      = 0x02,
    TYPE_OBJECT      = 0x03,
    TYPE_MOVIECLIP   = 0x04,
    TYPE_NULL        = 0x05,
    TYPE_UNDEFINED   = 0x06,
    TYPE_REFERENCE   = 0x07,
    TYPE_ECMAARRAY   = 0x08,
    TYPE_OBJECT_END  = 0x09,
    TYPE_ARRAY       = 0x0a,
    TYPE_DATE        = 0x0b,
    TYPE_LONG_STRING = 0x0c,
  };

private:
  memory_cptr m_data_mem;
  mm_mem_io_c m_data;
  meta_data_t m_meta_data;
  bool m_in_meta_data;
  unsigned int m_level;

  debugging_option_c m_debug;

public:
  script_parser_c(memory_cptr const &mem);

  bool parse();
  meta_data_t const &get_meta_data() const;

  template<typename T>
  std::optional<T>
  get_meta_data_value(std::string const &key) {
    auto itr = m_meta_data.find(key);
    if (m_meta_data.end() == itr)
      return {};

    return std::get<T>(itr->second);
  }

protected:
  std::string read_string(data_type_e type);
  std::pair<value_type_t, bool> read_value();
  void read_properties(std::unordered_map<std::string, value_type_t> &properties);
  std::string level_spacer() const;
};

}

namespace std {
template <>
struct hash<mtx::amf::script_parser_c::data_type_e> {
  size_t operator()(mtx::amf::script_parser_c::data_type_e value) const {
    return hash<unsigned int>()(static_cast<unsigned int>(value));
  }
};
}
