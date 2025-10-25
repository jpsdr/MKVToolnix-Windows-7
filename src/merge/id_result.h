/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definition for the generic reader and packetizer

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/id_info.h"
#include "common/translation.h"

constexpr auto ID_RESULT_TRACK_AUDIO     = "audio";
constexpr auto ID_RESULT_TRACK_VIDEO     = "video";
constexpr auto ID_RESULT_TRACK_SUBTITLES = "subtitles";
constexpr auto ID_RESULT_TRACK_BUTTONS   = "buttons";
constexpr auto ID_RESULT_TRACK_UNKNOWN   = "unknown";
constexpr auto ID_RESULT_CHAPTERS        = "chapters";
constexpr auto ID_RESULT_TAGS            = "tags";
constexpr auto ID_RESULT_GLOBAL_TAGS_ID  = -1;

// When bumping the schema version:
// • increase `ID_JSON_FORMAT_VERSION` here
// • copy `doc/json-schema/mkvmerge-identification-output-schema-v….json` with the next version number
// • in the new copy adjust the following elements:
//   1. `id`
//   2. `properties` → `identification_format_version` → `minimum` and `maximum`
// • adjust the link in `doc/man/mkvmerge.xml`
constexpr auto ID_JSON_FORMAT_VERSION = 20;

struct id_result_t {
  int64_t id;
  std::string type, info, description;
  mtx::id::verbose_info_t verbose_info;
  int64_t size;

  id_result_t() {
  };

  id_result_t(int64_t p_id,
              const std::string &p_type,
              const std::string &p_info,
              const std::string &p_description,
              int64_t p_size)
    : id{p_id}
    , type{p_type}
    , info{p_info}
    , description{p_description}
    , size{p_size}
  {
  }

  id_result_t(const id_result_t &src)
    : id{src.id}
    , type{src.type}
    , info{src.info}
    , description{src.description}
    , verbose_info{src.verbose_info}
    , size{src.size}
  {
  }
};

void id_result_container_unsupported(std::string const &filename, translatable_string_c const &info);
