/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   definitions used in all programs, helper functions

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include <sstream>

#include <QRegularExpression>

#include "common/mm_io_x.h"
#include "common/mm_file_io.h"
#include "common/mm_proxy_io.h"
#include "common/mm_text_io.h"
#include "common/qt.h"
#include "common/strings/editing.h"
#include "common/xml/xml.h"

namespace mtx::xml {

std::string
escape(const std::string &source) {
  std::string dst;
  std::string::const_iterator src = source.begin();
  while (src != source.end()) {
    if (*src == '&')
      dst += "&amp;";
    else if (*src == '>')
      dst += "&gt;";
    else if (*src == '<')
      dst += "&lt;";
    else if (*src == '"')
      dst += "&quot;";
    else
      dst += *src;
    src++;
  }

  return dst;
}

std::string
create_node_name(const char *name,
                 const char **atts) {
  int i;
  std::string node_name = "<"s + name;
  for (i = 0; atts[i] && atts[i + 1]; i += 2)
    node_name += " "s + atts[i] + "=\"" +
      escape(atts[i + 1]) + "\"";
  node_name += ">";

  return node_name;
}

document_cptr
load_file(std::string const &file_name,
          unsigned int options,
          std::optional<int64_t> max_read_size) {
  auto af_in = mm_file_io_c::open(file_name, libebml::MODE_READ);
  mm_text_io_c in(af_in);
  std::string content;
  auto bytes_to_read = (max_read_size ? std::min(in.get_size(), *max_read_size) : in.get_size()) - in.get_byte_order_length();

  if (in.read(content, bytes_to_read) != bytes_to_read)
    throw mtx::mm_io::end_of_file_x{};

  if (byte_order_mark_e::none == in.get_byte_order_mark()) {
    QRegularExpression encoding_re{
      "("
      "^[ \\t]*"                // ignore leading whitespace
      "<\\?xml"                 // XML declaration start
      "[^?]+?"                  // skip to encoding, but don't go beyond XML declaration
      "encoding[ \\t]*=[ \\t]*" // encoding attribute
      "\")([^\"]+)",            // attribute value
      QRegularExpression::CaseInsensitiveOption};

    // Extract the old encoding, replace the string with "UTF-8" so
    // that pugixml doesn't recode, and recode to UTF-8.
    if (auto matches = encoding_re.match(Q(content)); matches.hasMatch())
      content = charset_converter_c::init(to_utf8(matches.captured(2)))->utf8(content);

    content = to_utf8(Q(content).replace(encoding_re, Q("\\1UTF-8")));
  }

  std::stringstream scontent(content);
  auto doc = std::make_shared<pugi::xml_document>();
  auto result = doc->load(scontent, options);
  if (!result)
    throw xml_parser_x{result};

  return doc;
}

}
