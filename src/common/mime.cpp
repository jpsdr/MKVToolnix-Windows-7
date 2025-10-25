/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include <QByteArray>
#include <QMimeDatabase>
#include <QMimeType>

#include "common/mime.h"
#include "common/qt.h"

namespace mtx::mime {

namespace {
std::unique_ptr<QMimeDatabase> s_database;
std::unordered_map<std::string, std::string> s_font_mime_type_mapping_current_to_legacy, s_font_mime_type_mapping_legacy_to_current;

QMimeDatabase &
database() {
  if (!s_database)
    s_database.reset(new QMimeDatabase);
  return *s_database;
}

const std::unordered_map<std::string, std::string> &
get_font_mime_type_mapping(font_mime_type_type_e type) {
  if (s_font_mime_type_mapping_current_to_legacy.empty()) {
    s_font_mime_type_mapping_legacy_to_current["application/x-font-otf"s] = "application/vnd.ms-opentype"s;
    s_font_mime_type_mapping_legacy_to_current["application/x-font-ttf"s] = "application/x-truetype-font"s;
    s_font_mime_type_mapping_current_to_legacy["font/otf"s]               = "application/vnd.ms-opentype"s;
    s_font_mime_type_mapping_current_to_legacy["font/sfnt"s]              = "application/x-truetype-font"s;
    s_font_mime_type_mapping_current_to_legacy["font/ttf"s]               = "application/x-truetype-font"s;
    s_font_mime_type_mapping_current_to_legacy["font/collection"s]        = "application/x-truetype-font"s;
  }

  if (s_font_mime_type_mapping_legacy_to_current.empty()) {
    s_font_mime_type_mapping_legacy_to_current["application/vnd.ms-opentype"s] = "font/otf"s;
    s_font_mime_type_mapping_legacy_to_current["application/x-font-otf"s]      = "font/otf"s;
    s_font_mime_type_mapping_legacy_to_current["application/x-font-ttf"s]      = "font/ttf"s;
    s_font_mime_type_mapping_legacy_to_current["application/x-truetype-font"s] = "font/ttf"s;
  }

  return type == font_mime_type_type_e::legacy ? s_font_mime_type_mapping_current_to_legacy : s_font_mime_type_mapping_legacy_to_current;
}

} // anonymous namespace

std::string
guess_type_for_data(memory_c const &data) {
  auto type = database().mimeTypeForData(QByteArray{reinterpret_cast<char const *>(data.get_buffer()), static_cast<int>(data.get_size())});

  return to_utf8(type.name());
}

std::string
guess_type_for_data(std::string const &data) {
  auto type = database().mimeTypeForData(QByteArray::fromStdString(data));

  return to_utf8(type.name());
}

std::string
guess_type_for_file(std::string const &file_name) {
  auto type = database().mimeTypeForFile(Q(file_name));

  return to_utf8(type.name());
}

std::string
primary_file_extension_for_type(std::string const &type_name) {
  auto all_types   = database().allMimeTypes();
  auto q_type_name = Q(type_name).toLower();

  for (auto const &type : all_types)
    if (type.name() == q_type_name) {
      auto extension = to_utf8(type.preferredSuffix());
      return extension == "jfif" ? "jpg"s : extension;
    }

  return {};
}

std::vector<std::string>
sorted_type_names() {
  std::vector<std::string> names, names_to_exclude;

  for (auto const &type : { font_mime_type_type_e::legacy, font_mime_type_type_e::current }) {
    auto const &mapping = get_font_mime_type_mapping(type);

    for (auto const &pair : mapping)
      names_to_exclude.emplace_back(pair.first);
  }

  auto all_types = database().allMimeTypes();
  names.reserve(all_types.size());

  for (auto const &type : all_types) {
    auto u8_name = to_utf8(type.name());

    if (std::find(names_to_exclude.begin(), names_to_exclude.end(), u8_name) == names_to_exclude.end())
      names.emplace_back(u8_name);
  }

  for (auto const &name : { "font/otf"s, "application/vnd.ms-opentype"s, "font/ttf"s, "application/x-truetype-font"s })
    if (std::find(names.begin(), names.end(), name) == names.end())
      names.emplace_back(name);

  std::sort(names.begin(), names.end());

  return names;
}

std::string
get_font_mime_type_to_use(std::string const &mime_type,
                          font_mime_type_type_e type) {
  auto const &map_to_use = get_font_mime_type_mapping(type);
  auto itr               = map_to_use.find(mime_type);

  if (itr != map_to_use.end())
    return itr->second;

  return mime_type;
}

}                              // namespace mtx::mime
