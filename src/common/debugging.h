/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   definitions used in all programs, helper functions

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include <sstream>
#include <unordered_map>

#include "common/memory.h"
#include "common/mm_io_fwd.h"

class debugging_c {
protected:
  static bool ms_send_to_logger;
  static std::unordered_map<std::string, std::string> ms_debugging_options;

public:
  static void send_to_logger(bool enable);
  static void output(std::string const &msg);

  static void hexdump(const void *buffer_to_dump, size_t lenth);
  static void hexdump(memory_c const &buffer_to_dump, std::optional<std::size_t> max_length = std::nullopt);
  static void hexdump(memory_cptr const &buffer_to_dump, std::optional<std::size_t> max_length = std::nullopt);

  static bool requested(const char *option, std::string *arg = nullptr);
  static bool requested(const std::string &option, std::string *arg = nullptr) {
    return requested(option.c_str(), arg);
  }
  static void request(const std::string &options, bool enable = true);
  static void init();
};

class debugging_option_c {
  struct option_c {
    std::optional<bool> m_requested;
    std::string m_option;

    option_c(std::string const &option)
      : m_option{option}
    {
    }

    bool get() {
      if (!m_requested.has_value())
        m_requested = debugging_c::requested(m_option);

      return m_requested.value();
    }
  };

protected:
  mutable size_t m_registered_idx;
  std::string m_option;

private:
  static std::vector<option_c> ms_registered_options;

public:
  debugging_option_c(std::string const &option)
    : m_registered_idx{std::numeric_limits<size_t>::max()}
    , m_option{option}
  {
  }

  operator bool() const {
    return ms_registered_options.at(get_idx()).get();
  }

  void set(std::optional<bool> requested) {
    ms_registered_options.at(get_idx()).m_requested = requested;
  }

protected:
  std::size_t get_idx() const {
    if (m_registered_idx == std::numeric_limits<size_t>::max())
      m_registered_idx = register_option(m_option);

    return m_registered_idx;
  }

public:
  static size_t register_option(std::string const &option);
  static void invalidate_cache();
};

#define mxdebug(msg) debugging_c::output(fmt::format("Debug> {0}:{1:04}: {2}", __FILE__, __LINE__, msg))

#define mxdebug_if(condition, msg) \
  if (condition) {                 \
    mxdebug(msg);                  \
  }

namespace libebml {
class EbmlElement;
}

class ebml_dumper_c {
public:
  enum target_type_e {
    STDOUT,
    MM_IO,
    LOGGER,
  };

  enum dump_style_e {
    style_with_values    = 0x01,
    style_with_addresses = 0x02,
    style_with_indexes   = 0x04,

    style_default        = style_with_addresses | style_with_indexes | style_with_values,
  };

private:
  bool m_values, m_addresses, m_indexes;
  size_t m_max_level;
  target_type_e m_target_type;
  mm_io_c *m_io_target;
  std::stringstream m_buffer;

public:
  ebml_dumper_c();

  ebml_dumper_c &values(bool p_values);
  ebml_dumper_c &addresses(bool p_addresses);
  ebml_dumper_c &indexes(bool p_indexes);
  ebml_dumper_c &max_level(int p_max_level);
  ebml_dumper_c &target(target_type_e p_target_type, mm_io_c *p_io_target = nullptr);

  ebml_dumper_c &dump(libebml::EbmlElement const *element);
  ebml_dumper_c &dump_if(bool do_it, libebml::EbmlElement const *element) {
    return do_it ? dump(element) : *this;
  }

public:
  static std::string dump_to_string(libebml::EbmlElement const *element, dump_style_e style = style_default);
  static std::string to_string(libebml::EbmlElement const &element);

private:
  void dump_impl(libebml::EbmlElement const *element, size_t level, size_t index);
};

void dump_ebml_elements(libebml::EbmlElement *element, bool with_values = true);

inline std::ostream &
operator <<(std::ostream &out,
            libebml::EbmlElement const &element) {
  out << ebml_dumper_c::to_string(element);
  return out;
}
