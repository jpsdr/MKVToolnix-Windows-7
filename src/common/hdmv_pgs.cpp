/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   definitions and helper functions for PGS/SUP subtitles

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/hdmv_pgs.h"

namespace mtx::hdmv_pgs {

char const *
name_for_type(uint8_t type) {
  return type == PALETTE_DEFINITION_SEGMENT       ? "Palette Definition"
       : type == OBJECT_DEFINITION_SEGMENT        ? "Object Definition"
       : type == PRESENTATION_COMPOSITION_SEGMENT ? "Presentation Composition"
       : type == WINDOW_DEFINITION_SEGMENT        ? "Window Definition"
       : type == INTERACTIVE_COMPOSITION_SEGMENT  ? "Interactive Composition"
       : type == END_OF_DISPLAY_SEGMENT           ? "End Of Display"
       :                                            "reserved";
}

}
