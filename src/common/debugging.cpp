/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   debugging functions

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include <sstream>

#include <ebml/EbmlDate.h>
#include <ebml/EbmlDummy.h>
#include <ebml/EbmlFloat.h>
#include <ebml/EbmlMaster.h>
#include <ebml/EbmlSInteger.h>
#include <ebml/EbmlString.h>
#include <ebml/EbmlUInteger.h>
#include <ebml/EbmlUnicodeString.h>
#include <ebml/EbmlVoid.h>

#include "common/debugging.h"
#include "common/ebml.h"
#include "common/logger.h"
#include "common/mm_mem_io.h"
#include "common/strings/editing.h"
#include "common/strings/formatting.h"

std::unordered_map<std::string, std::string> debugging_c::ms_debugging_options;
bool debugging_c::ms_send_to_logger = false;

bool
debugging_c::requested(const char *option,
                       std::string *arg) {
  auto options = mtx::string::split(option, "|");

  for (auto &current_option : options) {
    auto option_ptr = ms_debugging_options.find(current_option);

    if (ms_debugging_options.end() != option_ptr) {
      if (arg)
        *arg = option_ptr->second;
      return true;
    }
  }

  return false;
}

void
debugging_c::request(const std::string &options,
                     bool enable) {
  auto all_options = mtx::string::split(options);

  for (auto &one_option : all_options) {
    auto parts = mtx::string::split(one_option, "=", 2);
    if (!parts[0].size())
      continue;
    if (parts[0] == "!")
      ms_debugging_options.clear();
    else if (parts[0] == "to_logger")
      debugging_c::send_to_logger(true);
    else if (!enable)
      ms_debugging_options.erase(parts[0]);
    else
      ms_debugging_options[parts[0]] = 1 == parts.size() ? ""s : parts[1];
  }

  debugging_option_c::invalidate_cache();
}

void
debugging_c::init() {
  auto env_vars = std::vector<std::string>{ "MKVTOOLNIX_DEBUG", "MTX_DEBUG", balg::to_upper_copy(get_program_name()) + "_DEBUG" };

  for (auto &name : env_vars) {
    auto value = getenv(name.c_str());
    if (value)
      request(value);
  }

#if defined(SYS_WINDOWS)
  send_to_logger(true);
#endif
}

void
debugging_c::send_to_logger(bool enable) {
  ms_send_to_logger = enable;
}

void
debugging_c::output(std::string const &msg) {
  if (ms_send_to_logger)
    log_it(msg);
  else
    mxmsg(MXMSG_INFO, msg);
}

void
debugging_c::hexdump(const void *buffer_to_dump,
                     size_t length) {
  std::stringstream dump, ascii;
  auto buffer     = static_cast<const uint8_t *>(buffer_to_dump);
  auto buffer_idx = 0u;

  while (buffer_idx < length) {
    if ((buffer_idx % 16) == 0) {
      if (0 < buffer_idx) {
        dump << " [" << ascii.str() << "]\n";
        ascii.str("");
      }
      dump << fmt::format("Debug> {0:08x}  ", buffer_idx);

    } else if ((buffer_idx % 8) == 0) {
      dump  << ' ';
      ascii << ' ';
    }

    ascii << (((32 <= buffer[buffer_idx]) && (127 > buffer[buffer_idx])) ? static_cast<char>(buffer[buffer_idx]) : '.');
    dump  << fmt::format("{0:02x} ", static_cast<unsigned int>(buffer[buffer_idx]));

    ++buffer_idx;
  }

  if ((buffer_idx % 16) != 0) {
    auto remaining = 16u - (buffer_idx % 16);
    dump << std::string(3u * remaining + (remaining >= 8 ? 1 : 0), ' ');
  }

  dump << " [" << ascii.str() << "]\n";

  debugging_c::output(dump.str());
}

void
debugging_c::hexdump(memory_cptr const &buffer_to_dump,
                     std::optional<std::size_t> max_length) {
  if (buffer_to_dump)
    hexdump(*buffer_to_dump, max_length);
}

void
debugging_c::hexdump(memory_c const &buffer_to_dump,
                     std::optional<std::size_t> max_length) {

  auto length = std::min<std::size_t>(max_length ? *max_length : buffer_to_dump.get_size(), buffer_to_dump.get_size());
  hexdump(buffer_to_dump.get_buffer(), length);
}

// ------------------------------------------------------------

std::vector<debugging_option_c::option_c> debugging_option_c::ms_registered_options;

size_t
debugging_option_c::register_option(std::string const &option) {
  auto itr = std::find_if(ms_registered_options.begin(), ms_registered_options.end(), [&option](option_c const &opt) { return opt.m_option == option; });
  if (itr != ms_registered_options.end())
    return std::distance(ms_registered_options.begin(), itr);

  ms_registered_options.emplace_back(option);

  return ms_registered_options.size() - 1;
}

void
debugging_option_c::invalidate_cache() {
  for (auto &opt : ms_registered_options)
    opt.m_requested.reset();
}

// ------------------------------------------------------------

ebml_dumper_c::ebml_dumper_c()
  : m_values{true}
  , m_addresses{true}
  , m_indexes{true}
  , m_max_level{std::numeric_limits<size_t>::max()}
  , m_target_type{STDOUT}
  , m_io_target{}
{
}

std::string
ebml_dumper_c::to_string(libebml::EbmlElement const &element) {
  return dynamic_cast<libebml::EbmlUInteger      const *>(&element) ? fmt::to_string(        static_cast<libebml::EbmlUInteger      const &>(element).GetValue())
       : dynamic_cast<libebml::EbmlSInteger      const *>(&element) ? fmt::to_string(        static_cast<libebml::EbmlSInteger      const &>(element).GetValue())
       : dynamic_cast<libebml::EbmlFloat         const *>(&element) ? fmt::format("{0:.9f}", static_cast<libebml::EbmlFloat         const &>(element).GetValue())
       : dynamic_cast<libebml::EbmlUnicodeString const *>(&element) ?                        static_cast<libebml::EbmlUnicodeString const &>(element).GetValueUTF8()
       : dynamic_cast<libebml::EbmlString        const *>(&element) ?                        static_cast<libebml::EbmlString        const &>(element).GetValue()
       : dynamic_cast<libebml::EbmlDate          const *>(&element) ? fmt::to_string(        static_cast<libebml::EbmlDate          const &>(element).GetEpochDate())
       :                                                              fmt::format("(type: {0} size: {1})",
                                                                                    dynamic_cast<libebml::EbmlBinary const *>(&element) ? "binary"
                                                                                  : dynamic_cast<libebml::EbmlMaster const *>(&element) ? "master"
                                                                                  : dynamic_cast<libebml::EbmlVoid   const *>(&element) ? "void"
                                                                                  :                                                       "unknown",
                                                                                  element.GetSize());
}

ebml_dumper_c &
ebml_dumper_c::values(bool p_values) {
  m_values = p_values;
  return *this;
}

ebml_dumper_c &
ebml_dumper_c::addresses(bool p_addresses) {
  m_addresses = p_addresses;
  return *this;
}

ebml_dumper_c &
ebml_dumper_c::indexes(bool p_indexes) {
  m_indexes = p_indexes;
  return *this;
}

ebml_dumper_c &
ebml_dumper_c::max_level(int p_max_level) {
  m_max_level = p_max_level;
  return *this;
}

ebml_dumper_c &
ebml_dumper_c::target(target_type_e p_target_type,
                      mm_io_c *p_io_target) {
  m_target_type = p_target_type;
  m_io_target   = p_io_target;
  return *this;
}

ebml_dumper_c &
ebml_dumper_c::dump(libebml::EbmlElement const *element) {
  dump_impl(element, 0, 0);

  switch (m_target_type) {
    case STDOUT: mxinfo(m_buffer.str()); break;
    case LOGGER: log_it(m_buffer.str()); break;
    case MM_IO:  assert(!!m_io_target); m_io_target->puts(m_buffer.str()); break;
    default:     assert(false);
  }

  m_buffer.str("");

  return *this;
}

void
ebml_dumper_c::dump_impl(libebml::EbmlElement const *element,
                         size_t level,
                         size_t index) {
  if (level > m_max_level)
    return;

  m_buffer << std::string(level, ' ');

  if (m_indexes)
    m_buffer << index << " ";

  if (!element) {
    m_buffer << "nullptr" << std::endl;
    return;
  }

  m_buffer << EBML_NAME(element);

  if (m_addresses)
    m_buffer << fmt::format(" @{0}", static_cast<void const *>(element));

  if (m_values)
    m_buffer << " " << to_string(*element);

  m_buffer << fmt::format(" ID 0x{0:x} valueIsSet {1} has_default_value {2}", get_ebml_id(*element).GetValue(), element->ValueIsSet(), has_default_value(element));

  m_buffer << std::endl;

  auto master = dynamic_cast<libebml::EbmlMaster const *>(element);
  if (!master)
    return;

  for (auto idx = 0u; master->ListSize() > idx; ++idx)
    dump_impl((*master)[idx], level + 1, idx);
}

std::string
ebml_dumper_c::dump_to_string(libebml::EbmlElement const *element,
                              dump_style_e style) {
  mm_mem_io_c mem{nullptr, 0ull, 1000};
  ebml_dumper_c dumper{};

  dumper.target(MM_IO, &mem)
    .values(   !!(style & style_with_values))
    .addresses(!!(style & style_with_addresses))
    .indexes(  !!(style & style_with_indexes))
    .dump(element);

  return mem.get_content();
}

void
dump_ebml_elements(libebml::EbmlElement *element,
                   bool with_values) {
  ebml_dumper_c{}.values(with_values).dump(element);
}
