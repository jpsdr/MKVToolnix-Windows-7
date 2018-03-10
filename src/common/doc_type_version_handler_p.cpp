/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include <matroska/KaxBlock.h>
#include <matroska/KaxSemantic.h>

#include "common/doc_type_version_handler_p.h"

using namespace libmatroska;

namespace mtx {

std::unordered_map<unsigned int, unsigned int> doc_type_version_handler_private_c::s_version_by_element, doc_type_version_handler_private_c::s_read_version_by_element;

void
doc_type_version_handler_private_c::init_tables() {
  if (!s_version_by_element.empty())
    return;

  s_version_by_element[KaxCodecState::ClassInfos.GlobalId.GetValue()]          = 2;
  s_version_by_element[KaxCueCodecState::ClassInfos.GlobalId.GetValue()]       = 2;
  s_version_by_element[KaxSeekPreRoll::ClassInfos.GlobalId.GetValue()]         = 4;
  s_version_by_element[KaxCodecDelay::ClassInfos.GlobalId.GetValue()]          = 4;
  s_version_by_element[KaxVideoFlagInterlaced::ClassInfos.GlobalId.GetValue()] = 2;
  s_version_by_element[KaxDiscardPadding::ClassInfos.GlobalId.GetValue()]      = 4;
  s_version_by_element[KaxCueDuration::ClassInfos.GlobalId.GetValue()]         = 4;
  s_version_by_element[KaxCueRelativePosition::ClassInfos.GlobalId.GetValue()] = 4;

  s_read_version_by_element[KaxSimpleBlock::ClassInfos.GlobalId.GetValue()]    = 2;
}

} // namespace mtx
