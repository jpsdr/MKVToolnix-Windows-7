/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Written by Moritz Bunkus <moritz@bunkus.org>.
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

QMimeDatabase &
database() {
  if (!s_database)
    s_database.reset(new QMimeDatabase);
  return *s_database;
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
    if (type.name() == q_type_name)
      return to_utf8(type.preferredSuffix());

  return {};
}

std::vector<std::string>
sorted_type_names() {
  std::vector<std::string> names;

  auto all_types = database().allMimeTypes();
  names.reserve(all_types.size());

  for (auto const &type : all_types)
    names.emplace_back(to_utf8(type.name()));

  std::sort(names.begin(), names.end());

  return names;
}

}                              // namespace mtx::mime
