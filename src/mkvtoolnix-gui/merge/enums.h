#pragma once

#include "common/common_pch.h"

#include <QString>
#include <QVariant>

#include "common/qt.h"
#include "mkvtoolnix-gui/merge/mux_config.h"

namespace mtx::gui::Merge {

enum class TrackType {
  Audio = 0,
  Video,
  Subtitles,
  Buttons,
  Chapters,
  GlobalTags,
  Tags,
  Attachment,
  Min = Audio,
  Max = Attachment
};

enum class TrackCompression {
  Default = 0,
  None,
  Zlib,
  Min = Default,
  Max = Zlib
};

inline uint
qHash(TrackType const &type) {
  return static_cast<uint>(type);
}

}
