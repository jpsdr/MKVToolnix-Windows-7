/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

namespace mtx::mime {

std::string guess_type_for_data(memory_c const &data);
std::string guess_type_for_data(std::string const &data);
std::string guess_type_for_file(std::string const &file_name);

std::string primary_file_extension_for_type(std::string const &type_name);

std::vector<std::string> sorted_type_names();

enum class font_mime_type_type_e {
  current,
  legacy,
};
std::string get_font_mime_type_to_use(std::string const &mime_type, font_mime_type_type_e type);

}
