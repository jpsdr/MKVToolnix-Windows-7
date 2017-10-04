/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   class definition for the generic reader and packetizer

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/id_info.h"

#define ID_RESULT_TRACK_AUDIO     "audio"
#define ID_RESULT_TRACK_VIDEO     "video"
#define ID_RESULT_TRACK_SUBTITLES "subtitles"
#define ID_RESULT_TRACK_BUTTONS   "buttons"
#define ID_RESULT_TRACK_UNKNOWN   "unknown"
#define ID_RESULT_CHAPTERS        "chapters"
#define ID_RESULT_TAGS            "tags"
#define ID_RESULT_GLOBAL_TAGS_ID  -1

// When bumping the schema version:
// • increase `ID_JSON_FORMAT_VERSION` here
// • copy `doc/json-schema/mkvmerge-identification-output-schema-v….json` with the next version number
// • in the new copy adjust the following elements:
//   1. `id`
//   2. `properties` → `identification_format_version` → `minimum` and `maximum`
// • adjust the link in `doc/man/mkvmerge.xml`
#define ID_JSON_FORMAT_VERSION    10

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
