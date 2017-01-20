/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   definitions and helper functions for AAC data

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#ifndef MTX_COMMON_JSON_H
#define MTX_COMMON_JSON_H

#include "common/common_pch.h"

#if defined(HAVE_NLOHMANN_JSONCPP)
# include <json.hpp>
#else
# include "nlohmann-json/src/json.hpp"
#endif // HAVE_NLOHMANN_JSONCPP

namespace mtx { namespace json {

nlohmann::json parse(nlohmann::json::string_t const &data, nlohmann::json::parser_callback_t callback = nullptr);
nlohmann::json::string_t dump(nlohmann::json const &json, int indentation = 0);

}} // namespace mtx::json

#endif // MTX_COMMON_JSON_H
