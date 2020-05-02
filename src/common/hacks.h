/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   some hacks that the author might want to use

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

namespace mtx::hacks {

// Some hacks that are configurable via command line but which should ONLY!
// be used by the author.

namespace {
constexpr unsigned int SPACE_AFTER_CHAPTERS         =  0;
constexpr unsigned int NO_CHAPTERS_IN_META_SEEK     =  1;
constexpr unsigned int NO_META_SEEK                 =  2;
constexpr unsigned int LACING_XIPH                  =  3;
constexpr unsigned int LACING_EBML                  =  4;
constexpr unsigned int NATIVE_MPEG4                 =  5;
constexpr unsigned int NO_VARIABLE_DATA             =  6;
constexpr unsigned int FORCE_PASSTHROUGH_PACKETIZER =  7;
constexpr unsigned int WRITE_HEADERS_TWICE          =  8;
constexpr unsigned int ALLOW_AVC_IN_VFW_MODE        =  9;
constexpr unsigned int KEEP_BITSTREAM_AR_INFO       = 10;
constexpr unsigned int NO_SIMPLE_BLOCKS             = 11;
constexpr unsigned int USE_CODEC_STATE_ONLY         = 12;
constexpr unsigned int ENABLE_TIMESTAMP_WARNING     = 13;
constexpr unsigned int REMOVE_BITSTREAM_AR_INFO     = 14;
constexpr unsigned int VOBSUB_SUBPIC_STOP_CMDS      = 15;
constexpr unsigned int NO_CUE_DURATION              = 16;
constexpr unsigned int NO_CUE_RELATIVE_POSITION     = 17;
constexpr unsigned int NO_DELAY_FOR_GARBAGE_IN_AVI  = 18;
constexpr unsigned int KEEP_LAST_CHAPTER_IN_MPLS    = 19;
constexpr unsigned int KEEP_TRACK_STATISTICS_TAGS   = 20;
constexpr unsigned int ALL_I_SLICES_ARE_KEY_FRAMES  = 21;
constexpr unsigned int APPEND_AND_SPLIT_FLAC        = 22;
constexpr unsigned int MAX_IDX                      = 22;
}

void engage(const std::string &hacks);
void engage(unsigned int id);
bool is_engaged(unsigned int id);
void init();

}
