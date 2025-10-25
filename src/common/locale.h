/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Definitions for locale handling functions

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include <iconv.h>

class charset_converter_c;
using charset_converter_cptr = std::shared_ptr<charset_converter_c>;

class charset_converter_c {
protected:
  std::string m_charset;
  bool m_detect_byte_order_marker{};

public:
  charset_converter_c() = default;
  charset_converter_c(std::string charset);
  virtual ~charset_converter_c() = default;

  virtual std::string utf8(const std::string &source);
  virtual std::string native(const std::string &source);
  virtual void enable_byte_order_marker_detection(bool enable);
  std::string const &get_charset() const;

protected:
  bool handle_string_with_bom(const std::string &source, std::string &recoded);

public:                         // Static members
  static charset_converter_cptr init(const std::string &charset, bool ignore_errors = false);
  static bool is_utf8_charset_name(const std::string &charset);

private:
  static std::map<std::string, charset_converter_cptr> s_converters;
};

class iconv_charset_converter_c: public charset_converter_c {
private:
  bool m_is_utf8;
  iconv_t m_to_utf8_handle, m_from_utf8_handle;

public:
  iconv_charset_converter_c(const std::string &charset);
  virtual ~iconv_charset_converter_c();

  virtual std::string utf8(const std::string &source);
  virtual std::string native(const std::string &source);

public:                         // Static functions
  static bool is_available(const std::string &charset);

private:                        // Static functions
  static std::string convert(iconv_t handle, const std::string &source);
};

#if defined(SYS_WINDOWS)
class windows_charset_converter_c: public charset_converter_c {
private:
  bool m_is_utf8;
  unsigned int m_code_page;

public:
  windows_charset_converter_c(const std::string &charset);
  virtual ~windows_charset_converter_c();

  virtual std::string utf8(const std::string &source);
  virtual std::string native(const std::string &source);

public:                         // Static functions
  static bool is_available(const std::string &charset);

private:                        // Static functions
  static std::string convert(unsigned int source_code_page, unsigned int destination_code_page, const std::string &source);
  static unsigned int extract_code_page(const std::string &charset);
};
#endif

extern charset_converter_cptr g_cc_local_utf8;

std::string get_local_charset();
std::string get_local_console_charset();
void initialize_std_and_boost_filesystem_locales();
