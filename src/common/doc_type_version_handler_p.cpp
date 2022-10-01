/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include <matroska/KaxBlock.h>
#include <matroska/KaxCuesData.h>
#include <matroska/KaxSemantic.h>

#include "common/doc_type_version_handler_p.h"

using namespace libmatroska;

namespace mtx {

std::unordered_map<unsigned int, unsigned int> doc_type_version_handler_private_c::s_version_by_element, doc_type_version_handler_private_c::s_read_version_by_element;

void
doc_type_version_handler_private_c::init_tables() {
  if (!s_version_by_element.empty())
    return;

  s_version_by_element[KaxCodecDecodeAll::ClassInfos.GlobalId.GetValue()]                   = 2;
  s_version_by_element[KaxCodecState::ClassInfos.GlobalId.GetValue()]                       = 2;
  s_version_by_element[KaxCueCodecState::ClassInfos.GlobalId.GetValue()]                    = 2;
  s_version_by_element[KaxCueRefTime::ClassInfos.GlobalId.GetValue()]                       = 2;
  s_version_by_element[KaxCueReference::ClassInfos.GlobalId.GetValue()]                     = 2;
  s_version_by_element[KaxSimpleBlock::ClassInfos.GlobalId.GetValue()]                      = 2;
  s_version_by_element[KaxTrackFlagEnabled::ClassInfos.GlobalId.GetValue()]                 = 2;
  s_version_by_element[KaxVideoFlagInterlaced::ClassInfos.GlobalId.GetValue()]              = 2;

  s_version_by_element[KaxChapterStringUID::ClassInfos.GlobalId.GetValue()]                 = 3;
  s_version_by_element[KaxTrackCombinePlanes::ClassInfos.GlobalId.GetValue()]               = 3;
  s_version_by_element[KaxTrackJoinBlocks::ClassInfos.GlobalId.GetValue()]                  = 3;
  s_version_by_element[KaxTrackJoinUID::ClassInfos.GlobalId.GetValue()]                     = 3;
  s_version_by_element[KaxTrackOperation::ClassInfos.GlobalId.GetValue()]                   = 3;
  s_version_by_element[KaxTrackPlane::ClassInfos.GlobalId.GetValue()]                       = 3;
  s_version_by_element[KaxTrackPlaneType::ClassInfos.GlobalId.GetValue()]                   = 3;
  s_version_by_element[KaxTrackPlaneUID::ClassInfos.GlobalId.GetValue()]                    = 3;
  s_version_by_element[KaxVideoAlphaMode::ClassInfos.GlobalId.GetValue()]                   = 3;
  s_version_by_element[KaxVideoStereoMode::ClassInfos.GlobalId.GetValue()]                  = 3;

  s_version_by_element[KaxChapLanguageIETF::ClassInfos.GlobalId.GetValue()]                 = 4;
  s_version_by_element[KaxCodecDelay::ClassInfos.GlobalId.GetValue()]                       = 4;
  s_version_by_element[KaxCueDuration::ClassInfos.GlobalId.GetValue()]                      = 4;
  s_version_by_element[KaxCueRelativePosition::ClassInfos.GlobalId.GetValue()]              = 4;
  s_version_by_element[KaxDiscardPadding::ClassInfos.GlobalId.GetValue()]                   = 4;
  s_version_by_element[KaxFlagCommentary::ClassInfos.GlobalId.GetValue()]                   = 4;
  s_version_by_element[KaxFlagHearingImpaired::ClassInfos.GlobalId.GetValue()]              = 4;
  s_version_by_element[KaxFlagOriginal::ClassInfos.GlobalId.GetValue()]                     = 4;
  s_version_by_element[KaxFlagTextDescriptions::ClassInfos.GlobalId.GetValue()]             = 4;
  s_version_by_element[KaxFlagVisualImpaired::ClassInfos.GlobalId.GetValue()]               = 4;
  s_version_by_element[KaxLanguageIETF::ClassInfos.GlobalId.GetValue()]                     = 4;
  s_version_by_element[KaxSeekPreRoll::ClassInfos.GlobalId.GetValue()]                      = 4;
  s_version_by_element[KaxTagLanguageIETF::ClassInfos.GlobalId.GetValue()]                  = 4;
  s_version_by_element[KaxTrackDefaultDecodedFieldDuration::ClassInfos.GlobalId.GetValue()] = 4;
  s_version_by_element[KaxVideoBChromaX::ClassInfos.GlobalId.GetValue()]                    = 4;
  s_version_by_element[KaxVideoBChromaY::ClassInfos.GlobalId.GetValue()]                    = 4;
  s_version_by_element[KaxVideoBitsPerChannel::ClassInfos.GlobalId.GetValue()]              = 4;
  s_version_by_element[KaxVideoCbSubsampHorz::ClassInfos.GlobalId.GetValue()]               = 4;
  s_version_by_element[KaxVideoCbSubsampVert::ClassInfos.GlobalId.GetValue()]               = 4;
  s_version_by_element[KaxVideoChromaSitHorz::ClassInfos.GlobalId.GetValue()]               = 4;
  s_version_by_element[KaxVideoChromaSitVert::ClassInfos.GlobalId.GetValue()]               = 4;
  s_version_by_element[KaxVideoChromaSubsampHorz::ClassInfos.GlobalId.GetValue()]           = 4;
  s_version_by_element[KaxVideoChromaSubsampVert::ClassInfos.GlobalId.GetValue()]           = 4;
  s_version_by_element[KaxVideoColour::ClassInfos.GlobalId.GetValue()]                      = 4;
  s_version_by_element[KaxVideoColourMasterMeta::ClassInfos.GlobalId.GetValue()]            = 4;
  s_version_by_element[KaxVideoColourMatrix::ClassInfos.GlobalId.GetValue()]                = 4;
  s_version_by_element[KaxVideoColourMaxCLL::ClassInfos.GlobalId.GetValue()]                = 4;
  s_version_by_element[KaxVideoColourMaxFALL::ClassInfos.GlobalId.GetValue()]               = 4;
  s_version_by_element[KaxVideoColourPrimaries::ClassInfos.GlobalId.GetValue()]             = 4;
  s_version_by_element[KaxVideoColourRange::ClassInfos.GlobalId.GetValue()]                 = 4;
  s_version_by_element[KaxVideoColourTransferCharacter::ClassInfos.GlobalId.GetValue()]     = 4;
  s_version_by_element[KaxVideoFieldOrder::ClassInfos.GlobalId.GetValue()]                  = 4;
  s_version_by_element[KaxVideoGChromaX::ClassInfos.GlobalId.GetValue()]                    = 4;
  s_version_by_element[KaxVideoGChromaY::ClassInfos.GlobalId.GetValue()]                    = 4;
  s_version_by_element[KaxVideoLuminanceMax::ClassInfos.GlobalId.GetValue()]                = 4;
  s_version_by_element[KaxVideoLuminanceMin::ClassInfos.GlobalId.GetValue()]                = 4;
  s_version_by_element[KaxVideoProjection::ClassInfos.GlobalId.GetValue()]                  = 4;
  s_version_by_element[KaxVideoProjectionPosePitch::ClassInfos.GlobalId.GetValue()]         = 4;
  s_version_by_element[KaxVideoProjectionPoseRoll::ClassInfos.GlobalId.GetValue()]          = 4;
  s_version_by_element[KaxVideoProjectionPoseYaw::ClassInfos.GlobalId.GetValue()]           = 4;
  s_version_by_element[KaxVideoProjectionPrivate::ClassInfos.GlobalId.GetValue()]           = 4;
  s_version_by_element[KaxVideoProjectionType::ClassInfos.GlobalId.GetValue()]              = 4;
  s_version_by_element[KaxVideoRChromaX::ClassInfos.GlobalId.GetValue()]                    = 4;
  s_version_by_element[KaxVideoRChromaY::ClassInfos.GlobalId.GetValue()]                    = 4;
  s_version_by_element[KaxVideoWhitePointChromaX::ClassInfos.GlobalId.GetValue()]           = 4;
  s_version_by_element[KaxVideoWhitePointChromaY::ClassInfos.GlobalId.GetValue()]           = 4;

  s_version_by_element[KaxEditionDisplay::ClassInfos.GlobalId.GetValue()]                   = 5;
  s_version_by_element[KaxEditionLanguageIETF::ClassInfos.GlobalId.GetValue()]              = 5;
  s_version_by_element[KaxEditionString::ClassInfos.GlobalId.GetValue()]                    = 5;
  s_version_by_element[KaxEmphasis::ClassInfos.GlobalId.GetValue()]                         = 5;

  s_read_version_by_element[KaxSimpleBlock::ClassInfos.GlobalId.GetValue()]                 = 2;
}

} // namespace mtx
