/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   definitions and helper functions for PGS/SUP subtitles

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include "common/hdmv_pgs.h"

namespace mtx { namespace hdmv_pgs {

char const *
name_for_type(uint8_t type) {
  return type == PALETTE_DEFINITION_SEGMENT       ? "Palette Definition Segment"
       : type == OBJECT_DEFINITION_SEGMENT        ? "Object Definition Segment"
       : type == PRESENTATION_COMPOSITION_SEGMENT ? "Presentation Composition Segment"
       : type == WINDOW_DEFINITION_SEGMENT        ? "Window Definition Segment"
       : type == INTERACTIVE_COMPOSITION_SEGMENT  ? "Interactive Composition Segment"
       : type == END_OF_DISPLAY_SEGMENT           ? "End Of Display Segment"
       :                                            "reserved";
}

}}
