/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   definitions used in all programs, helper functions

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include <sstream>

#include "common/mm_io_x.h"
#include "common/xml/xml.h"

namespace mtx {
namespace xml {

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
  std::string node_name = std::string("<") + name;
  for (i = 0; atts[i] && atts[i + 1]; i += 2)
    node_name += std::string(" ") + atts[i] + "=\"" +
      escape(atts[i + 1]) + "\"";
  node_name += ">";

  return node_name;
}

document_cptr
load_file(std::string const &file_name,
          unsigned int options,
          boost::optional<int64_t> max_read_size) {
  auto af_in = mm_file_io_c::open(file_name, MODE_READ);
  mm_text_io_c in(af_in.get(), false);
  std::string content;
  auto bytes_to_read = (max_read_size ? std::min(in.get_size(), *max_read_size) : in.get_size()) - in.get_byte_order_length();

  if (in.read(content, bytes_to_read) != bytes_to_read)
    throw mtx::mm_io::end_of_file_x{};

  if (BO_NONE == in.get_byte_order()) {
    boost::regex encoding_re("^ \\s* "              // ignore leading whitespace
                             "<\\?xml"              // XML declaration start
                             "[^\\?]+"              // skip to encoding, but don't go beyond XML declaration
                             "encoding \\s* = \\s*" // encoding attribute
                             "\" ( [^\"]+ ) \"",    // attribute value
                             boost::regex::perl | boost::regex::mod_x | boost::regex::icase);

    boost::smatch matches;
    if (boost::regex_search(content, matches, encoding_re)) {
      // Extract the old encoding, replace the string with "UTF-8" so
      // that pugixml doesn't recode, and recode to UTF-8.
      auto encoding = matches[1].str();
      content.replace(matches.position(1), matches.length(1), "UTF-8");
      content = charset_converter_c::init(encoding)->utf8(content);
    }
  }

  std::stringstream scontent(content);
  auto doc = std::make_shared<pugi::xml_document>();
  auto result = doc->load(scontent, options);
  if (!result)
    throw xml_parser_x{result};

  return doc;
}

}}
