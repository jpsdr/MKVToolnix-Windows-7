/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include <matroska/KaxBlock.h>
#include <matroska/KaxCuesData.h>
#include <matroska/KaxSemantic.h>

#include "common/doc_type_version_handler_p.h"

namespace mtx {

std::unordered_map<unsigned int, unsigned int> doc_type_version_handler_private_c::s_version_by_element, doc_type_version_handler_private_c::s_read_version_by_element;

void
doc_type_version_handler_private_c::init_tables() {
  if (!s_version_by_element.empty())
    return;

  s_version_by_element[EBML_ID(libmatroska::KaxCodecDecodeAll).GetValue()]                   = 2;
  s_version_by_element[EBML_ID(libmatroska::KaxCodecState).GetValue()]                       = 2;
  s_version_by_element[EBML_ID(libmatroska::KaxCueCodecState).GetValue()]                    = 2;
  s_version_by_element[EBML_ID(libmatroska::KaxCueRefTime).GetValue()]                       = 2;
  s_version_by_element[EBML_ID(libmatroska::KaxCueReference).GetValue()]                     = 2;
  s_version_by_element[EBML_ID(libmatroska::KaxSimpleBlock).GetValue()]                      = 2;
  s_version_by_element[EBML_ID(libmatroska::KaxTrackFlagEnabled).GetValue()]                 = 2;
  s_version_by_element[EBML_ID(libmatroska::KaxVideoFlagInterlaced).GetValue()]              = 2;

  s_version_by_element[EBML_ID(libmatroska::KaxChapterStringUID).GetValue()]                 = 3;
  s_version_by_element[EBML_ID(libmatroska::KaxTrackCombinePlanes).GetValue()]               = 3;
  s_version_by_element[EBML_ID(libmatroska::KaxTrackJoinBlocks).GetValue()]                  = 3;
  s_version_by_element[EBML_ID(libmatroska::KaxTrackJoinUID).GetValue()]                     = 3;
  s_version_by_element[EBML_ID(libmatroska::KaxTrackOperation).GetValue()]                   = 3;
  s_version_by_element[EBML_ID(libmatroska::KaxTrackPlane).GetValue()]                       = 3;
  s_version_by_element[EBML_ID(libmatroska::KaxTrackPlaneType).GetValue()]                   = 3;
  s_version_by_element[EBML_ID(libmatroska::KaxTrackPlaneUID).GetValue()]                    = 3;
  s_version_by_element[EBML_ID(libmatroska::KaxVideoAlphaMode).GetValue()]                   = 3;
  s_version_by_element[EBML_ID(libmatroska::KaxVideoStereoMode).GetValue()]                  = 3;

  s_version_by_element[EBML_ID(libmatroska::KaxChapLanguageIETF).GetValue()]                 = 4;
  s_version_by_element[EBML_ID(libmatroska::KaxCodecDelay).GetValue()]                       = 4;
  s_version_by_element[EBML_ID(libmatroska::KaxCueDuration).GetValue()]                      = 4;
  s_version_by_element[EBML_ID(libmatroska::KaxCueRelativePosition).GetValue()]              = 4;
  s_version_by_element[EBML_ID(libmatroska::KaxDiscardPadding).GetValue()]                   = 4;
  s_version_by_element[EBML_ID(libmatroska::KaxFlagCommentary).GetValue()]                   = 4;
  s_version_by_element[EBML_ID(libmatroska::KaxFlagHearingImpaired).GetValue()]              = 4;
  s_version_by_element[EBML_ID(libmatroska::KaxFlagOriginal).GetValue()]                     = 4;
  s_version_by_element[EBML_ID(libmatroska::KaxFlagTextDescriptions).GetValue()]             = 4;
  s_version_by_element[EBML_ID(libmatroska::KaxFlagVisualImpaired).GetValue()]               = 4;
  s_version_by_element[EBML_ID(libmatroska::KaxLanguageIETF).GetValue()]                     = 4;
  s_version_by_element[EBML_ID(libmatroska::KaxSeekPreRoll).GetValue()]                      = 4;
  s_version_by_element[EBML_ID(libmatroska::KaxTagLanguageIETF).GetValue()]                  = 4;
  s_version_by_element[EBML_ID(libmatroska::KaxTrackDefaultDecodedFieldDuration).GetValue()] = 4;
  s_version_by_element[EBML_ID(libmatroska::KaxVideoBChromaX).GetValue()]                    = 4;
  s_version_by_element[EBML_ID(libmatroska::KaxVideoBChromaY).GetValue()]                    = 4;
  s_version_by_element[EBML_ID(libmatroska::KaxVideoBitsPerChannel).GetValue()]              = 4;
  s_version_by_element[EBML_ID(libmatroska::KaxVideoCbSubsampHorz).GetValue()]               = 4;
  s_version_by_element[EBML_ID(libmatroska::KaxVideoCbSubsampVert).GetValue()]               = 4;
  s_version_by_element[EBML_ID(libmatroska::KaxVideoChromaSitHorz).GetValue()]               = 4;
  s_version_by_element[EBML_ID(libmatroska::KaxVideoChromaSitVert).GetValue()]               = 4;
  s_version_by_element[EBML_ID(libmatroska::KaxVideoChromaSubsampHorz).GetValue()]           = 4;
  s_version_by_element[EBML_ID(libmatroska::KaxVideoChromaSubsampVert).GetValue()]           = 4;
  s_version_by_element[EBML_ID(libmatroska::KaxVideoColour).GetValue()]                      = 4;
  s_version_by_element[EBML_ID(libmatroska::KaxVideoColourMasterMeta).GetValue()]            = 4;
  s_version_by_element[EBML_ID(libmatroska::KaxVideoColourMatrix).GetValue()]                = 4;
  s_version_by_element[EBML_ID(libmatroska::KaxVideoColourMaxCLL).GetValue()]                = 4;
  s_version_by_element[EBML_ID(libmatroska::KaxVideoColourMaxFALL).GetValue()]               = 4;
  s_version_by_element[EBML_ID(libmatroska::KaxVideoColourPrimaries).GetValue()]             = 4;
  s_version_by_element[EBML_ID(libmatroska::KaxVideoColourRange).GetValue()]                 = 4;
  s_version_by_element[EBML_ID(libmatroska::KaxVideoColourTransferCharacter).GetValue()]     = 4;
  s_version_by_element[EBML_ID(libmatroska::KaxVideoFieldOrder).GetValue()]                  = 4;
  s_version_by_element[EBML_ID(libmatroska::KaxVideoGChromaX).GetValue()]                    = 4;
  s_version_by_element[EBML_ID(libmatroska::KaxVideoGChromaY).GetValue()]                    = 4;
  s_version_by_element[EBML_ID(libmatroska::KaxVideoLuminanceMax).GetValue()]                = 4;
  s_version_by_element[EBML_ID(libmatroska::KaxVideoLuminanceMin).GetValue()]                = 4;
  s_version_by_element[EBML_ID(libmatroska::KaxVideoProjection).GetValue()]                  = 4;
  s_version_by_element[EBML_ID(libmatroska::KaxVideoProjectionPosePitch).GetValue()]         = 4;
  s_version_by_element[EBML_ID(libmatroska::KaxVideoProjectionPoseRoll).GetValue()]          = 4;
  s_version_by_element[EBML_ID(libmatroska::KaxVideoProjectionPoseYaw).GetValue()]           = 4;
  s_version_by_element[EBML_ID(libmatroska::KaxVideoProjectionPrivate).GetValue()]           = 4;
  s_version_by_element[EBML_ID(libmatroska::KaxVideoProjectionType).GetValue()]              = 4;
  s_version_by_element[EBML_ID(libmatroska::KaxVideoRChromaX).GetValue()]                    = 4;
  s_version_by_element[EBML_ID(libmatroska::KaxVideoRChromaY).GetValue()]                    = 4;
  s_version_by_element[EBML_ID(libmatroska::KaxVideoWhitePointChromaX).GetValue()]           = 4;
  s_version_by_element[EBML_ID(libmatroska::KaxVideoWhitePointChromaY).GetValue()]           = 4;

  s_version_by_element[EBML_ID(libmatroska::KaxEditionDisplay).GetValue()]                   = 5;
  s_version_by_element[EBML_ID(libmatroska::KaxEditionLanguageIETF).GetValue()]              = 5;
  s_version_by_element[EBML_ID(libmatroska::KaxEditionString).GetValue()]                    = 5;
  s_version_by_element[EBML_ID(libmatroska::KaxEmphasis).GetValue()]                         = 5;

  s_read_version_by_element[EBML_ID(libmatroska::KaxSimpleBlock).GetValue()]                 = 2;
}

} // namespace mtx
