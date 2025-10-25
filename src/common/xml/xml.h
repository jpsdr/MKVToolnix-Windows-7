/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   XML helper functions

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "pugixml.hpp"

namespace mtx::xml {

class exception: public mtx::exception {
public:
  virtual const char *what() const throw() {
    return "generic XML error";
  }
};

class conversion_x: public exception {
protected:
  std::string m_message;
public:
  conversion_x(std::string const &message) : m_message{message} { }
  virtual ~conversion_x() throw() { }

  virtual char const *what() const throw() {
    return m_message.c_str();
  }
};

class invalid_attribute_x: public exception {
protected:
  std::string m_message, m_node, m_attribute;
  ptrdiff_t m_position;
public:
  invalid_attribute_x(std::string const &node, std::string const &attribute, ptrdiff_t position)
    : m_node(node)
    , m_attribute(attribute)
    , m_position(position)
  {
    m_message = fmt::format(FY("Invalid attribute '{0}' in node '{1}' at position {2}"), m_attribute, m_node, m_position);
  }
  virtual ~invalid_attribute_x() throw() { }

  virtual char const *what() const throw() {
    return m_message.c_str();
  }
};

class invalid_child_node_x: public exception {
protected:
  std::string m_message, m_node, m_parent;
  ptrdiff_t m_position;
public:
  invalid_child_node_x(std::string const &node, std::string const &parent, ptrdiff_t position)
    : m_node(node)
    , m_parent(parent)
    , m_position(position)
  {
    m_message = fmt::format(FY("<{0}> is not a valid child element of <{1}> at position {2}."), m_node, m_parent, m_position);
  }
  virtual ~invalid_child_node_x() throw() { }

  virtual char const *what() const throw() {
    return m_message.c_str();
  }
};

class duplicate_child_node_x: public exception {
protected:
  std::string m_message, m_node, m_parent;
  ptrdiff_t m_position;
public:
  duplicate_child_node_x(std::string const &node, std::string const &parent, ptrdiff_t position)
    : m_node(node)
    , m_parent(parent)
    , m_position(position)
  {
    m_message = fmt::format(FY("Only one instance of <{0}> is allowed beneath <{1}> at position {2}."), m_node, m_parent, m_position);
  }
  virtual ~duplicate_child_node_x() throw() { }

  virtual char const *what() const throw() {
    return m_message.c_str();
  }
};

class malformed_data_x: public exception {
protected:
  std::string m_message, m_node;
  ptrdiff_t m_position;
public:
  malformed_data_x(std::string const &node, ptrdiff_t position, std::string const &details = std::string{})
    : m_node(node)
    , m_position(position)
  {
    m_message = fmt::format(FY("The tag or attribute '{0}' at position {1} contains invalid or mal-formed data."), m_node, m_position);
    if (!details.empty())
      m_message += " " + details;
  }
  virtual ~malformed_data_x() throw() { }

  virtual char const *what() const throw() {
    return m_message.c_str();
  }
};

class out_of_range_x: public exception {
protected:
  std::string m_message, m_node;
  ptrdiff_t m_position;
public:
  out_of_range_x(std::string const &node, ptrdiff_t position, std::string const &details = std::string{})
    : m_node(node)
    , m_position(position)
  {
    m_message = fmt::format(FY("The tag or attribute '{0}' at position {1} contains data that is outside its allowed range."), m_node, m_position);
    if (!details.empty())
      m_message += " " + details;
  }
  virtual ~out_of_range_x() throw() { }

  virtual char const *what() const throw() {
    return m_message.c_str();
  }
};

class xml_parser_x: public exception {
protected:
  pugi::xml_parse_result m_result;
public:
  xml_parser_x(pugi::xml_parse_result const &result) : m_result(result) { }
  virtual const char *what() const throw() {
    return "XML parser error";
  }
  pugi::xml_parse_result const &result() {
    return m_result;
  }
};

std::string escape(const std::string &src);
std::string create_node_name(const char *name, const char **atts);

using document_cptr = std::shared_ptr<pugi::xml_document>;

document_cptr load_file(std::string const &file_name, unsigned int options = pugi::parse_default, std::optional<int64_t> max_read_size = std::optional<int64_t>{});

}
