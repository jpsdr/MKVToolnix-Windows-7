/*
   mkvextract -- extract tracks from Matroska files into other files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   extracts tracks and other items from Matroska files into other files

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/file_types.h"
#include "common/kax_analyzer.h"
#include "common/mm_io.h"
#include "extract/options.h"
#include "librmff/librmff.h"

// Helper functions in mkvextract.cpp
void show_element(libebml::EbmlElement *l, int level, const std::string &message);
void show_error(const std::string &error);

void find_and_verify_track_uids(libmatroska::KaxTracks &tracks, std::vector<track_spec_t> &tspecs);
void write_cuesheet(std::string file_name, libmatroska::KaxChapters &chapters, libmatroska::KaxTags &tags, int64_t tuid, mm_io_c &out);

bool extract_tracks(kax_analyzer_c &analyzer, options_c::mode_options_c &options);
bool extract_tags(kax_analyzer_c &analyzer, options_c::mode_options_c &options);
bool extract_chapters(kax_analyzer_c &analyzer, options_c::mode_options_c &options);
bool extract_attachments(kax_analyzer_c &analyzer, options_c::mode_options_c &options);
bool extract_cuesheet(kax_analyzer_c &analyzer, options_c::mode_options_c &options);
bool extract_timestamps(kax_analyzer_c &analyzer, options_c::mode_options_c &options);
bool extract_cues(kax_analyzer_c &analyzer, options_c::mode_options_c &options);

kax_analyzer_cptr open_and_analyze(std::string const &file_name, kax_analyzer_c::parse_mode_e parse_mode, bool exit_on_error = true);
mm_io_cptr open_output_file(std::string const &file_name);
