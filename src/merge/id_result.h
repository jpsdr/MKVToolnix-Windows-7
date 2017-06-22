/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   class definition for the generic reader and packetizer

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#ifndef MTX_MERGE_ID_RESULT_H
#define MTX_MERGE_ID_RESULT_H

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

#define ID_JSON_FORMAT_VERSION    8

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

#endif  // MTX_MERGE_ID_RESULT_H
