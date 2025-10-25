/*
  mkvmerge -- utility for splicing together matroska files
  from component media subtypes

  Distributed under the GPL v2
  see the file COPYING for details
  or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

  EBML/XML converter

  Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include <QRegularExpression>

#include <ebml/EbmlVoid.h>
#include <matroska/KaxSegment.h>

#include "common/at_scope_exit.h"
#include "common/base64.h"
#include "common/ebml.h"
#include "common/mm_io_x.h"
#include "common/mm_file_io.h"
#include "common/qt.h"
#include "common/strings/formatting.h"
#include "common/strings/parsing.h"
#include "common/strings/utf8.h"
#include "common/xml/ebml_converter.h"

namespace mtx::xml {

ebml_converter_c::limits_t::limits_t()
  : has_min{false}
  , has_max{false}
  , min{0}
  , max{0}
{
}

ebml_converter_c::limits_t::limits_t(bool p_has_min,
                                     bool p_has_max,
                                     int64_t p_min,
                                     int64_t p_max)
  : has_min{p_has_min}
  , has_max{p_has_max}
  , min{p_min}
  , max{p_max}
{
}

ebml_converter_c::ebml_converter_c()
{
}

ebml_converter_c::~ebml_converter_c() {
}

document_cptr
ebml_converter_c::to_xml(libebml::EbmlElement &e,
                         document_cptr const &destination)
  const {
  document_cptr doc = destination ? destination : document_cptr(new pugi::xml_document);
  to_xml_recursively(*doc, e);
  fix_xml(doc);
  return doc;
}

std::string
ebml_converter_c::get_tag_name(libebml::EbmlElement &e)
  const {
  auto mapped_name = m_debug_to_tag_name_map.find(EBML_NAME(&e));
  return mapped_name == m_debug_to_tag_name_map.end() ? std::string(EBML_NAME(&e)) : mapped_name->second;
}

std::string
ebml_converter_c::get_debug_name(std::string const &tag_name)
  const {
  auto mapped_name = m_tag_to_debug_name_map.find(tag_name);
  return mapped_name == m_tag_to_debug_name_map.end() ? tag_name : mapped_name->second;
}

void
ebml_converter_c::format_value(pugi::xml_node &node,
                               libebml::EbmlElement &e,
                               value_formatter_t default_formatter)
  const {
  std::string name = get_tag_name(e);
  auto formatter   = m_formatter_map.find(name);

  if (formatter != m_formatter_map.end())
    formatter->second(node, e);
  else
    default_formatter(node, e);
}

void
ebml_converter_c::parse_value(parser_context_t &ctx,
                              value_parser_t default_parser)
  const {
  auto parser = m_parser_map.find(ctx.name);

  if (parser != m_parser_map.end())
    parser->second(ctx);
  else
    default_parser(ctx);
}

void
ebml_converter_c::reverse_debug_to_tag_name_map() {
  m_tag_to_debug_name_map.clear();
  for (auto &pair : m_debug_to_tag_name_map)
    m_tag_to_debug_name_map[pair.second] = pair.first;
}

void
ebml_converter_c::fix_xml(document_cptr &)
  const {
}

void
ebml_converter_c::fix_ebml(libebml::EbmlMaster &)
  const {
}

void
ebml_converter_c::format_uint(pugi::xml_node &node,
                              libebml::EbmlElement &e) {
  node.append_child(pugi::node_pcdata).set_value(fmt::to_string(static_cast<libebml::EbmlUInteger &>(e).GetValue()).c_str());
}

void
ebml_converter_c::format_int(pugi::xml_node &node,
                             libebml::EbmlElement &e) {
  node.append_child(pugi::node_pcdata).set_value(fmt::to_string(static_cast<libebml::EbmlSInteger &>(e).GetValue()).c_str());
}

void
ebml_converter_c::format_timestamp(pugi::xml_node &node,
                                  libebml::EbmlElement &e) {
  node.append_child(pugi::node_pcdata).set_value(mtx::string::format_timestamp(static_cast<libebml::EbmlUInteger &>(e).GetValue()).c_str());
}

void
ebml_converter_c::format_string(pugi::xml_node &node,
                                libebml::EbmlElement &e) {
  node.append_child(pugi::node_pcdata).set_value(static_cast<libebml::EbmlString &>(e).GetValue().c_str());
}

void
ebml_converter_c::format_ustring(pugi::xml_node &node,
                                 libebml::EbmlElement &e) {
  node.append_child(pugi::node_pcdata).set_value(static_cast<libebml::EbmlUnicodeString &>(e).GetValueUTF8().c_str());
}

void
ebml_converter_c::format_binary(pugi::xml_node &node,
                                libebml::EbmlElement &e) {
  auto &binary = static_cast<libebml::EbmlBinary &>(e);
  auto hex     = mtx::string::to_hex(binary.GetBuffer(), binary.GetSize(), true);

  node.append_child(pugi::node_pcdata).set_value(hex.c_str());
  node.append_attribute("format") = "hex";
}

void
ebml_converter_c::parse_uint(parser_context_t &ctx) {
  uint64_t value;
  if (!mtx::string::parse_number(mtx::string::strip_copy(ctx.content), value))
    throw malformed_data_x{ ctx.name, ctx.node.offset_debug(), Y("An unsigned integer was expected.") };

  if (ctx.limits.has_min && (value < static_cast<uint64_t>(ctx.limits.min)))
    throw out_of_range_x{ ctx.name, ctx.node.offset_debug(), fmt::format(FY("Minimum allowed value: {0}, actual value: {1}"), ctx.limits.min, value) };
  if (ctx.limits.has_max && (value > static_cast<uint64_t>(ctx.limits.max)))
    throw out_of_range_x{ ctx.name, ctx.node.offset_debug(), fmt::format(FY("Maximum allowed value: {0}, actual value: {1}"), ctx.limits.max, value) };

  static_cast<libebml::EbmlUInteger &>(ctx.e).SetValue(value);
}

void
ebml_converter_c::parse_int(parser_context_t &ctx) {
  int64_t value;
  if (!mtx::string::parse_number(mtx::string::strip_copy(ctx.content), value))
    throw malformed_data_x{ ctx.name, ctx.node.offset_debug() };

  if (ctx.limits.has_min && (value < ctx.limits.min))
    throw out_of_range_x{ ctx.name, ctx.node.offset_debug(), fmt::format(FY("Minimum allowed value: {0}, actual value: {1}"), ctx.limits.min, value) };
  if (ctx.limits.has_max && (value > ctx.limits.max))
    throw out_of_range_x{ ctx.name, ctx.node.offset_debug(), fmt::format(FY("Maximum allowed value: {0}, actual value: {1}"), ctx.limits.max, value) };

  static_cast<libebml::EbmlSInteger &>(ctx.e).SetValue(value);
}

void
ebml_converter_c::parse_string(parser_context_t &ctx) {
  static_cast<libebml::EbmlString &>(ctx.e).SetValue(ctx.content);
}

void
ebml_converter_c::parse_ustring(parser_context_t &ctx) {
  static_cast<libebml::EbmlUnicodeString &>(ctx.e).SetValueUTF8(ctx.content);
}

void
ebml_converter_c::parse_timestamp(parser_context_t &ctx) {
  int64_t value;
  if (!mtx::string::parse_timestamp(mtx::string::strip_copy(ctx.content), value)) {
    auto details = fmt::format(FY("Expected a time in the following format: HH:MM:SS.nnn "
                                  "(HH = hour, MM = minute, SS = second, nnn = millisecond up to nanosecond. "
                                  "You may use up to nine digits for 'n' which would mean nanosecond precision). "
                                  "You may omit the hour as well. Found '{0}' instead. Additional error message: {1}"),
                               ctx.content, mtx::string::timestamp_parser_error.c_str());

    throw malformed_data_x{ ctx.name, ctx.node.offset_debug(), details };
  }

  if (ctx.limits.has_min && (value < ctx.limits.min))
    throw out_of_range_x{ ctx.name, ctx.node.offset_debug(), fmt::format(FY("Minimum allowed value: {0}, actual value: {1}"), ctx.limits.min, value) };
  if (ctx.limits.has_max && (value > ctx.limits.max))
    throw out_of_range_x{ ctx.name, ctx.node.offset_debug(), fmt::format(FY("Maximum allowed value: {0}, actual value: {1}"), ctx.limits.max, value) };

  static_cast<libebml::EbmlUInteger &>(ctx.e).SetValue(value);
}

void
ebml_converter_c::parse_binary(parser_context_t &ctx) {
  auto test_min_max = [&ctx](auto const &test_content) {
    if (ctx.limits.has_min && (test_content.length() < static_cast<size_t>(ctx.limits.min)))
      throw out_of_range_x{ ctx.name, ctx.node.offset_debug(), fmt::format(FY("Minimum allowed length: {0}, actual length: {1}"), ctx.limits.min, test_content.length()) };
    if (ctx.limits.has_max && (test_content.length() > static_cast<size_t>(ctx.limits.max)))
      throw out_of_range_x{ ctx.name, ctx.node.offset_debug(), fmt::format(FY("Maximum allowed length: {0}, actual length: {1}"), ctx.limits.max, test_content.length()) };
  };

  ctx.handled_attributes["format"] = true;

  auto content = mtx::string::strip_copy(ctx.content);

  if (balg::starts_with(content, "@")) {
    if (content.length() == 1)
      throw malformed_data_x{ ctx.name, ctx.node.offset_debug(), Y("No filename found after the '@'.") };

    auto file_name = content.substr(1);
    try {
      mm_file_io_c in(file_name, libebml::MODE_READ);
      auto size = in.get_size();
      content.resize(size);

      if (in.read(content, size) != size)
        throw mtx::mm_io::end_of_file_x{};

      test_min_max(content);

      static_cast<libebml::EbmlBinary &>(ctx.e).CopyBuffer(reinterpret_cast<uint8_t const *>(content.c_str()), size);

    } catch (mtx::mm_io::exception &) {
      throw malformed_data_x{ ctx.name, ctx.node.offset_debug(), fmt::format(FY("Could not open/read the file '{0}'."), file_name) };
    }

    return;
  }

  auto format = balg::to_lower_copy(std::string{ctx.node.attribute("format").value()});
  if (format.empty())
    format = "base64";

  if (format == "hex") {
    auto hex_content = to_utf8(Q(content).replace(QRegularExpression{"(0x|\\s|\\r|\\n)+", QRegularExpression::CaseInsensitiveOption}, {}));
    if (Q(hex_content).contains(QRegularExpression{"[^0-9a-f]", QRegularExpression::CaseInsensitiveOption}))
      throw malformed_data_x{ ctx.name, ctx.node.offset_debug(), Y("Non-hex digits encountered.") };

    if ((hex_content.size() % 2) == 1)
      throw malformed_data_x{ ctx.name, ctx.node.offset_debug(), Y("Invalid length of hexadecimal content: must be divisable by 2.") };

    content.clear();
    content.resize(hex_content.size() / 2);

    for (auto idx = 0u; idx < hex_content.length(); idx += 2)
      content[idx / 2] = static_cast<char>(mtx::string::from_hex(hex_content.substr(idx, 2)));

  } else if (format == "base64") {
    try {
      content = mtx::base64::decode(content)->to_string();

    } catch (mtx::base64::exception &) {
      throw malformed_data_x{ ctx.name, ctx.node.offset_debug(), Y("Invalid data for Base64 encoding found.") };
    }

  } else if (format != "ascii")
    throw malformed_data_x{ ctx.name, ctx.node.offset_debug(), fmt::format(FY("Invalid 'format' attribute '{0}'."), format) };

  test_min_max(content);

  static_cast<libebml::EbmlBinary &>(ctx.e).CopyBuffer(reinterpret_cast<uint8_t const *>(content.c_str()), content.length());
}

void
ebml_converter_c::to_xml_recursively(pugi::xml_node &parent,
                                     libebml::EbmlElement &e)
  const {
  // Don't write EBML void elements.
  if (dynamic_cast<libebml::EbmlVoid *>(&e))
    return;

  auto name     = get_tag_name(e);
  auto new_node = parent.append_child(name.c_str());

  if (dynamic_cast<libebml::EbmlMaster *>(&e))
    for (auto child : static_cast<libebml::EbmlMaster &>(e))
      to_xml_recursively(new_node, *child);

  else if (dynamic_cast<libebml::EbmlUInteger *>(&e))
    format_value(new_node, e, format_uint);

  else if (dynamic_cast<libebml::EbmlSInteger *>(&e))
    format_value(new_node, e, format_uint);

  else if (dynamic_cast<libebml::EbmlString *>(&e))
    format_value(new_node, e, format_string);

  else if (dynamic_cast<libebml::EbmlUnicodeString *>(&e))
    format_value(new_node, e, format_ustring);

  else if (dynamic_cast<libebml::EbmlBinary *>(&e))
    format_value(new_node, e, format_binary);

  else {
    parent.remove_child(new_node);
    parent.append_child(pugi::node_comment).set_value(fmt::format(" unknown EBML element '{0}' ", name).c_str());
  }
}

ebml_master_cptr
ebml_converter_c::to_ebml(std::string const &file_name,
                          std::string const &root_name) {
  auto doc       = load_file(file_name);
  auto root_node = doc->document_element();
  if (!root_node)
    return ebml_master_cptr();

  if (root_name != root_node.name())
    throw conversion_x{fmt::format(FY("The root element must be <{0}>."), root_name)};

  ebml_master_cptr ebml_root{new libmatroska::KaxSegment};

  to_ebml_recursively(*ebml_root, root_node);

  auto master = dynamic_cast<libebml::EbmlMaster *>((*ebml_root)[0]);
  if (!master)
    throw conversion_x{Y("The XML root element is not a master element.")};

  fix_ebml(*master);

  ebml_root->Remove(0);

  if (debugging_c::requested("ebml_converter"))
    dump_ebml_elements(master, true);

  return ebml_master_cptr{master};
}

void
ebml_converter_c::to_ebml_recursively(libebml::EbmlMaster &parent,
                                      pugi::xml_node &node)
  const {
  // Skip <EBMLVoid> elements.
  if (std::string(node.name()) == "EBMLVoid")
    return;

  std::map<std::string, bool> handled_attributes;

  auto converted_node   = convert_node_or_attribute_to_ebml(parent, node, pugi::xml_attribute{}, handled_attributes);
  auto converted_master = dynamic_cast<libebml::EbmlMaster *>(converted_node);

  for (auto attribute = node.attributes_begin(); node.attributes_end() != attribute; attribute++) {
    if (handled_attributes[attribute->name()])
      continue;

    if (!converted_master)
      throw invalid_attribute_x{ node.name(), attribute->name(), node.offset_debug() };

    convert_node_or_attribute_to_ebml(*converted_master, node, *attribute, handled_attributes);
  }

  for (auto child : node) {
    if (child.type() != pugi::node_element)
      continue;

    if (converted_master)
      to_ebml_recursively(*converted_master, child);
    else
      throw invalid_child_node_x{ node.first_child().name(), node.name(), node.offset_debug() };
  }
}

libebml::EbmlElement *
ebml_converter_c::convert_node_or_attribute_to_ebml(libebml::EbmlMaster &parent,
                                                    pugi::xml_node const &node,
                                                    pugi::xml_attribute const &attribute,
                                                    std::map<std::string, bool> &handled_attributes)
  const {
  std::string name  = attribute ? attribute.name()  : node.name();
  std::string value = attribute ? attribute.value() : node.child_value();
  auto new_element  = verify_and_create_element(parent, name, node);
  auto limits       = m_limits.find(name);
  auto is_ok        = false;
  at_scope_exit_c cleanup([&new_element, &is_ok]() { if (!is_ok) delete new_element; });

  parser_context_t ctx { name, value, *new_element, node, handled_attributes, limits == m_limits.end() ? limits_t{} : limits->second };

  if (dynamic_cast<libebml::EbmlUInteger *>(new_element))
    parse_value(ctx, parse_uint);

  else if (dynamic_cast<libebml::EbmlSInteger *>(new_element))
    parse_value(ctx, parse_int);

  else if (dynamic_cast<libebml::EbmlString *>(new_element))
    parse_value(ctx, parse_string);

  else if (dynamic_cast<libebml::EbmlUnicodeString *>(new_element))
    parse_value(ctx, parse_ustring);

  else if (dynamic_cast<libebml::EbmlBinary *>(new_element))
    parse_value(ctx, parse_binary);

  else if (!dynamic_cast<libebml::EbmlMaster *>(new_element))
    throw invalid_child_node_x{ name, get_tag_name(parent), node.offset_debug() };

  is_ok = true;
  parent.PushElement(*new_element);

  return new_element;
}

libebml::EbmlElement *
ebml_converter_c::verify_and_create_element(libebml::EbmlMaster &parent,
                                            std::string const &name,
                                            pugi::xml_node const &node)
  const {
  if (m_invalid_elements_map.find(name) != m_invalid_elements_map.end())
    throw invalid_child_node_x{ name, get_tag_name(parent), node.offset_debug() };

  auto debug_name = get_debug_name(name);
  auto &context   = EBML_CONTEXT(&parent);
  std::optional<libebml::EbmlId> id;
  size_t i;

  for (i = 0; i < EBML_CTX_SIZE(context); i++)
    if (debug_name == libebml::EbmlCallbacks(EBML_CTX_IDX_INFO(context, i)).GetName()) {
      id = EBML_CTX_IDX_ID(context, i);
      break;
    }

  if (!id)
    throw invalid_child_node_x{ name, get_tag_name(parent), node.offset_debug() };

  auto semantic = find_ebml_semantic(EBML_INFO(libmatroska::KaxSegment), *id);
  if (semantic && semantic->IsUnique())
    for (auto child : parent)
      if (get_ebml_id(*child) == *id)
        throw duplicate_child_node_x{ name, get_tag_name(parent), node.offset_debug() };

  return create_ebml_element(EBML_INFO(libmatroska::KaxSegment), *id);
}

void
ebml_converter_c::dump_semantics(std::string const &top_element_name)
  const {
  auto &context   = EBML_CLASS_CONTEXT(libmatroska::KaxSegment);
  auto debug_name = get_debug_name(top_element_name);
  size_t i;

  for (i = 0; i < EBML_CTX_SIZE(context); i++)
    if (debug_name == libebml::EbmlCallbacks(EBML_CTX_IDX_INFO(context, i)).GetName()) {
      std::shared_ptr<libebml::EbmlElement> child{ &EBML_SEM_CREATE(EBML_CTX_IDX(context, i)) };
      std::map<std::string, bool> visited_masters;
      dump_semantics_recursively(0, *child, visited_masters);
      return;
    }
}

void
ebml_converter_c::dump_semantics_recursively(int level,
                                             libebml::EbmlElement &element,
                                             std::map<std::string, bool> &visited_masters)
  const {
  auto tag_name = get_tag_name(element);

  if (m_invalid_elements_map.find(tag_name) != m_invalid_elements_map.end())
    return;

  auto limits = m_limits.find(tag_name);
  std::string limits_str;

  if ((m_limits.end() != limits) && (limits->second.has_min || limits->second.has_max)) {
    auto label = dynamic_cast<libebml::EbmlBinary *>(&element) ? "length in bytes" : "value";

    if (limits->second.has_min && limits->second.has_max) {
      if (limits->second.min == limits->second.max)
        limits_str = fmt::format("{1} == {0}",        limits->second.min,                     label);
      else
        limits_str = fmt::format("{0} <= {2} <= {1}", limits->second.min, limits->second.max, label);
    } else if (limits->second.has_min)
      limits_str = fmt::format("{0} <= {1}",          limits->second.min,                     label);
    else
      limits_str = fmt::format("{1} <= {0}",                              limits->second.max, label);

    limits_str = ", valid range: "s + limits_str;
  }

  auto type = dynamic_cast<libebml::EbmlMaster        *>(&element) ? "master"
            : dynamic_cast<libebml::EbmlUInteger      *>(&element) ? "unsigned integer"
            : dynamic_cast<libebml::EbmlSInteger      *>(&element) ? "signed integer"
            : dynamic_cast<libebml::EbmlString        *>(&element) ? "UTF-8 string"
            : dynamic_cast<libebml::EbmlUnicodeString *>(&element) ? "Unicode string"
            : dynamic_cast<libebml::EbmlBinary        *>(&element) ? "binary"
            :                                                        "unknown";

  mxinfo(fmt::format("{0}{1} ({2}{3})\n", std::string(level * 2, ' '), tag_name, type, limits_str));

  if (!dynamic_cast<libebml::EbmlMaster *>(&element) || visited_masters[tag_name])
    return;

  visited_masters[tag_name] = true;

  auto &context = EBML_CONTEXT(static_cast<libebml::EbmlMaster *>(&element));
  size_t i;

  for (i = 0; i < EBML_CTX_SIZE(context); i++) {
    std::shared_ptr<libebml::EbmlElement> child{ &EBML_SEM_CREATE(EBML_CTX_IDX(context, i)) };
    dump_semantics_recursively(level + 1, *child, visited_masters);
  }
}

}
