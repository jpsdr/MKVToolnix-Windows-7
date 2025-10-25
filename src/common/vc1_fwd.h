/** VC-1 video helper functions

   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   \file

   \author Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

namespace mtx::vc1 {

struct frame_t;
using frame_cptr = std::shared_ptr<frame_t>;

class es_parser_c;
using es_parser_cptr = std::shared_ptr<es_parser_c>;

}
