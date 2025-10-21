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

#if HAVE_NLOHMANN_JSONCPP != 2 // 0 == internal, 1 == system in subdirectory, 2 == system without subdirectory
# include <nlohmann/json.hpp>
#else
# include <json.hpp>
#endif // HAVE_NLOHMANN_JSONCPP

namespace mtx::json {

nlohmann::json parse(nlohmann::json::string_t const &data, nlohmann::json::parser_callback_t callback = nullptr);
nlohmann::json::string_t dump(nlohmann::json const &json, int indentation = 0);

} // namespace mtx::json
