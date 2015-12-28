/** MPEG helper functions

   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   \author Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#ifndef MTX_COMMON_MPEG_COMMON_H
#define MTX_COMMON_MPEG_COMMON_H

#include "common/common_pch.h"

namespace mpeg {

memory_cptr nalu_to_rbsp(memory_cptr const &buffer);
memory_cptr rbsp_to_nalu(memory_cptr const &buffer);

}

#endif  // MTX_COMMON_MPEG_COMMON_H
