/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#if !defined(HAVE_NLOHMANN_JSONCPP)
# include "nlohmann-json/src/json.hpp"
#elif HAVE_NLOHMANN_JSONCPP == 1
# include <nlohmann/json.hpp>
#else
# include <json.hpp>
#endif // HAVE_NLOHMANN_JSONCPP

namespace mtx { namespace json {

nlohmann::json parse(nlohmann::json::string_t const &data, nlohmann::json::parser_callback_t callback = nullptr);
nlohmann::json::string_t dump(nlohmann::json const &json, int indentation = 0);

}} // namespace mtx::json
