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

  s_version_by_element[EBML_ID(KaxCodecDecodeAll).GetValue()]                   = 2;
  s_version_by_element[EBML_ID(KaxCodecState).GetValue()]                       = 2;
  s_version_by_element[EBML_ID(KaxCueCodecState).GetValue()]                    = 2;
  s_version_by_element[EBML_ID(KaxCueRefTime).GetValue()]                       = 2;
  s_version_by_element[EBML_ID(KaxCueReference).GetValue()]                     = 2;
  s_version_by_element[EBML_ID(KaxSimpleBlock).GetValue()]                      = 2;
  s_version_by_element[EBML_ID(KaxTrackFlagEnabled).GetValue()]                 = 2;
  s_version_by_element[EBML_ID(KaxVideoFlagInterlaced).GetValue()]              = 2;

  s_version_by_element[EBML_ID(KaxChapterStringUID).GetValue()]                 = 3;
  s_version_by_element[EBML_ID(KaxTrackCombinePlanes).GetValue()]               = 3;
  s_version_by_element[EBML_ID(KaxTrackJoinBlocks).GetValue()]                  = 3;
  s_version_by_element[EBML_ID(KaxTrackJoinUID).GetValue()]                     = 3;
  s_version_by_element[EBML_ID(KaxTrackOperation).GetValue()]                   = 3;
  s_version_by_element[EBML_ID(KaxTrackPlane).GetValue()]                       = 3;
  s_version_by_element[EBML_ID(KaxTrackPlaneType).GetValue()]                   = 3;
  s_version_by_element[EBML_ID(KaxTrackPlaneUID).GetValue()]                    = 3;
  s_version_by_element[EBML_ID(KaxVideoAlphaMode).GetValue()]                   = 3;
  s_version_by_element[EBML_ID(KaxVideoStereoMode).GetValue()]                  = 3;

  s_version_by_element[EBML_ID(KaxChapLanguageIETF).GetValue()]                 = 4;
  s_version_by_element[EBML_ID(KaxCodecDelay).GetValue()]                       = 4;
  s_version_by_element[EBML_ID(KaxCueDuration).GetValue()]                      = 4;
  s_version_by_element[EBML_ID(KaxCueRelativePosition).GetValue()]              = 4;
  s_version_by_element[EBML_ID(KaxDiscardPadding).GetValue()]                   = 4;
  s_version_by_element[EBML_ID(KaxFlagCommentary).GetValue()]                   = 4;
  s_version_by_element[EBML_ID(KaxFlagHearingImpaired).GetValue()]              = 4;
  s_version_by_element[EBML_ID(KaxFlagOriginal).GetValue()]                     = 4;
  s_version_by_element[EBML_ID(KaxFlagTextDescriptions).GetValue()]             = 4;
  s_version_by_element[EBML_ID(KaxFlagVisualImpaired).GetValue()]               = 4;
  s_version_by_element[EBML_ID(KaxLanguageIETF).GetValue()]                     = 4;
  s_version_by_element[EBML_ID(KaxSeekPreRoll).GetValue()]                      = 4;
  s_version_by_element[EBML_ID(KaxTagLanguageIETF).GetValue()]                  = 4;
  s_version_by_element[EBML_ID(KaxTrackDefaultDecodedFieldDuration).GetValue()] = 4;
  s_version_by_element[EBML_ID(KaxVideoBChromaX).GetValue()]                    = 4;
  s_version_by_element[EBML_ID(KaxVideoBChromaY).GetValue()]                    = 4;
  s_version_by_element[EBML_ID(KaxVideoBitsPerChannel).GetValue()]              = 4;
  s_version_by_element[EBML_ID(KaxVideoCbSubsampHorz).GetValue()]               = 4;
  s_version_by_element[EBML_ID(KaxVideoCbSubsampVert).GetValue()]               = 4;
  s_version_by_element[EBML_ID(KaxVideoChromaSitHorz).GetValue()]               = 4;
  s_version_by_element[EBML_ID(KaxVideoChromaSitVert).GetValue()]               = 4;
  s_version_by_element[EBML_ID(KaxVideoChromaSubsampHorz).GetValue()]           = 4;
  s_version_by_element[EBML_ID(KaxVideoChromaSubsampVert).GetValue()]           = 4;
  s_version_by_element[EBML_ID(KaxVideoColour).GetValue()]                      = 4;
  s_version_by_element[EBML_ID(KaxVideoColourMasterMeta).GetValue()]            = 4;
  s_version_by_element[EBML_ID(KaxVideoColourMatrix).GetValue()]                = 4;
  s_version_by_element[EBML_ID(KaxVideoColourMaxCLL).GetValue()]                = 4;
  s_version_by_element[EBML_ID(KaxVideoColourMaxFALL).GetValue()]               = 4;
  s_version_by_element[EBML_ID(KaxVideoColourPrimaries).GetValue()]             = 4;
  s_version_by_element[EBML_ID(KaxVideoColourRange).GetValue()]                 = 4;
  s_version_by_element[EBML_ID(KaxVideoColourTransferCharacter).GetValue()]     = 4;
  s_version_by_element[EBML_ID(KaxVideoFieldOrder).GetValue()]                  = 4;
  s_version_by_element[EBML_ID(KaxVideoGChromaX).GetValue()]                    = 4;
  s_version_by_element[EBML_ID(KaxVideoGChromaY).GetValue()]                    = 4;
  s_version_by_element[EBML_ID(KaxVideoLuminanceMax).GetValue()]                = 4;
  s_version_by_element[EBML_ID(KaxVideoLuminanceMin).GetValue()]                = 4;
  s_version_by_element[EBML_ID(KaxVideoProjection).GetValue()]                  = 4;
  s_version_by_element[EBML_ID(KaxVideoProjectionPosePitch).GetValue()]         = 4;
  s_version_by_element[EBML_ID(KaxVideoProjectionPoseRoll).GetValue()]          = 4;
  s_version_by_element[EBML_ID(KaxVideoProjectionPoseYaw).GetValue()]           = 4;
  s_version_by_element[EBML_ID(KaxVideoProjectionPrivate).GetValue()]           = 4;
  s_version_by_element[EBML_ID(KaxVideoProjectionType).GetValue()]              = 4;
  s_version_by_element[EBML_ID(KaxVideoRChromaX).GetValue()]                    = 4;
  s_version_by_element[EBML_ID(KaxVideoRChromaY).GetValue()]                    = 4;
  s_version_by_element[EBML_ID(KaxVideoWhitePointChromaX).GetValue()]           = 4;
  s_version_by_element[EBML_ID(KaxVideoWhitePointChromaY).GetValue()]           = 4;

  s_version_by_element[EBML_ID(KaxEditionDisplay).GetValue()]                   = 5;
  s_version_by_element[EBML_ID(KaxEditionLanguageIETF).GetValue()]              = 5;
  s_version_by_element[EBML_ID(KaxEditionString).GetValue()]                    = 5;
  s_version_by_element[EBML_ID(KaxEmphasis).GetValue()]                         = 5;

  s_read_version_by_element[EBML_ID(KaxSimpleBlock).GetValue()]                 = 2;
}

} // namespace mtx
