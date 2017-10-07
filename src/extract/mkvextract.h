/*
   mkvextract -- extract tracks from Matroska files into other files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   extracts tracks and other items from Matroska files into other files

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#include <matroska/KaxChapters.h>
#include <matroska/KaxTags.h>
#include <matroska/KaxTracks.h>

#include "common/file_types.h"
#include "common/kax_analyzer.h"
#include "common/mm_io.h"
#include "extract/track_spec.h"
#include "librmff/librmff.h"

using namespace libebml;
using namespace libmatroska;

#define in_parent(p)                                                                     \
  (   !p->IsFiniteSize()                                                                 \
   || (in.getFilePointer() < (p->GetElementPosition() + p->HeadSize() + p->GetSize())))

// Helper functions in mkvextract.cpp
void show_element(EbmlElement *l, int level, const std::string &message);
inline void
show_element(EbmlElement *l,
             int level,
             const boost::format &format) {
  show_element(l, level, format.str());
}

void show_error(const std::string &error);
inline void
show_error(const boost::format &format) {
  show_error(format.str());
}

void find_and_verify_track_uids(KaxTracks &tracks, std::vector<track_spec_t> &tspecs);

bool extract_tracks(kax_analyzer_c &analyzer, std::vector<track_spec_t> &tspecs);
void extract_tags(kax_analyzer_c &analyzer);
void extract_chapters(kax_analyzer_c &analyzer, bool chapter_format_simple, boost::optional<std::string> const &language_to_extract);
void extract_attachments(kax_analyzer_c &analyzer, std::vector<track_spec_t> &tracks);
void extract_cuesheet(kax_analyzer_c &analyzer);
void write_cuesheet(std::string file_name, KaxChapters &chapters, KaxTags &tags, int64_t tuid, mm_io_c &out);
void extract_timestamps(kax_analyzer_c &analyzer, std::vector<track_spec_t> &tspecs, int version);
void extract_cues(kax_analyzer_c &analyzer, std::vector<track_spec_t> const &tracks);

kax_analyzer_cptr open_and_analyze(std::string const &file_name, kax_analyzer_c::parse_mode_e parse_mode, bool exit_on_error = true);
