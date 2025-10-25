/*
  mkvmerge -- utility for splicing together matroska files
  from component media subtypes

  Distributed under the GPL v2
  see the file COPYING for details
  or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

  EBML/XML converter

  Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/ebml.h"
#include "common/xml/xml.h"

namespace mtx::xml {

class ebml_converter_c {
public:
  struct limits_t {
    bool has_min, has_max;
    int64_t min, max;
    limits_t();
    limits_t(bool p_has_min, bool p_has_max, int64_t p_min, int64_t p_max);
  };

  struct parser_context_t {
    std::string const &name;
    std::string const &content;
    libebml::EbmlElement &e;
    pugi::xml_node const &node;
    std::map<std::string, bool> &handled_attributes;
    limits_t limits;
  };

  using value_formatter_t = std::function<void(pugi::xml_node &, libebml::EbmlElement &)>;
  using value_parser_t    = std::function<void(parser_context_t &ctx)>;

protected:
  std::map<std::string, std::string> m_debug_to_tag_name_map, m_tag_to_debug_name_map;
  std::map<std::string, value_formatter_t> m_formatter_map;
  std::map<std::string, value_parser_t> m_parser_map;
  std::map<std::string, limits_t> m_limits;
  std::map<std::string, bool> m_invalid_elements_map;

public:
  ebml_converter_c();
  virtual ~ebml_converter_c();

  document_cptr to_xml(libebml::EbmlElement &e, document_cptr const &destination = document_cptr{}) const;
  ebml_master_cptr to_ebml(std::string const &file_name, std::string const &required_root_name);

  std::string get_tag_name(libebml::EbmlElement &e) const;
  std::string get_debug_name(std::string const &tag_name) const;

public:
  static void format_uint(pugi::xml_node &node, libebml::EbmlElement &e);
  static void format_int(pugi::xml_node &node, libebml::EbmlElement &e);
  static void format_string(pugi::xml_node &node, libebml::EbmlElement &e);
  static void format_ustring(pugi::xml_node &node, libebml::EbmlElement &e);
  static void format_binary(pugi::xml_node &node, libebml::EbmlElement &e);
  static void format_timestamp(pugi::xml_node &node, libebml::EbmlElement &e);

  static void parse_uint(parser_context_t &ctx);
  static void parse_int(parser_context_t &ctx);
  static void parse_string(parser_context_t &ctx);
  static void parse_ustring(parser_context_t &ctx);
  static void parse_binary(parser_context_t &ctx);
  static void parse_timestamp(parser_context_t &ctx);

protected:
  void format_value(pugi::xml_node &node, libebml::EbmlElement &e, value_formatter_t default_formatter) const;
  void parse_value(parser_context_t &ctx, value_parser_t default_parser) const;

  void to_xml_recursively(pugi::xml_node &parent, libebml::EbmlElement &e) const;

  void to_ebml_recursively(libebml::EbmlMaster &parent, pugi::xml_node &node) const;
  libebml::EbmlElement *convert_node_or_attribute_to_ebml(libebml::EbmlMaster &parent, pugi::xml_node const &node, pugi::xml_attribute const &attribute, std::map<std::string, bool> &handled_attributes) const;
  libebml::EbmlElement *verify_and_create_element(libebml::EbmlMaster &parent, std::string const &name, pugi::xml_node const &node) const;

  virtual void fix_xml(document_cptr &doc) const;
  virtual void fix_ebml(libebml::EbmlMaster &root) const;

  void reverse_debug_to_tag_name_map();

  void dump_semantics(std::string const &top_element_name) const;
  void dump_semantics_recursively(int level, libebml::EbmlElement &element, std::map<std::string, bool> &visited_masters) const;
};

}
