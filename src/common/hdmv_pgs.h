/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   definitions and helper functions for PGS/SUP subtitles

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

namespace mtx::hdmv_pgs {

namespace {

uint16_t constexpr FILE_MAGIC                       = 0x5047; // "PG" big endian

uint8_t  constexpr PALETTE_DEFINITION_SEGMENT       =   0x14;
uint8_t  constexpr OBJECT_DEFINITION_SEGMENT        =   0x15;
uint8_t  constexpr PRESENTATION_COMPOSITION_SEGMENT =   0x16;
uint8_t  constexpr WINDOW_DEFINITION_SEGMENT        =   0x17;
uint8_t  constexpr INTERACTIVE_COMPOSITION_SEGMENT  =   0x18;
uint8_t  constexpr END_OF_DISPLAY_SEGMENT           =   0x80;

} // anonymous namespace

char const *name_for_type(uint8_t type);

} // namespace mtx::hdmv_pgs
