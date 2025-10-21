/*
   mkvpropedit -- utility for editing properties of existing Matroska files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "propedit/globals.h"

std::unique_ptr<mtx::doc_type_version_handler_c> g_doc_type_version_handler;
std::unordered_map<uint64_t, uint64_t> g_track_uid_changes;
bool g_use_legacy_font_mime_types{};
