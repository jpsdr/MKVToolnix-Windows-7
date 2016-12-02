/** VC-1 video helper functions

   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   \file

   \author Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#ifndef MTX_COMMON_VC1_FWD_H
#define MTX_COMMON_VC1_FWD_H

namespace mtx { namespace vc1 {

struct frame_t;
using frame_cptr = std::shared_ptr<frame_t>;

class es_parser_c;
using es_parser_cptr = std::shared_ptr<es_parser_c>;

}}

#endif  // MTX_COMMON_VC1_FWD_H
