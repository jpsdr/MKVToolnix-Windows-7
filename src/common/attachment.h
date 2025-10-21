/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   definition of functions for converting between Vorbis comments and Matroska tags

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

struct attachment_t {
  std::string name, stored_name, mime_type, description, source_file;
  uint64_t id{};
  bool to_all_files{};
  memory_cptr data;
  int64_t ui_id{};
};
using attachment_cptr = std::shared_ptr<attachment_t>;
