#pragma once

#include "common/common_pch.h"

#include <QString>
#include <QHash>

#include "mkvtoolnix-gui/merge/track.h"

namespace mtx { namespace gui { namespace Merge {

struct MkvmergeOptionBuilder {
  QStringList options;
  QHash<Track::Type, unsigned int> numTracksOfType;
  QHash<Track::Type, QStringList> enabledTrackIds;

  MkvmergeOptionBuilder() {}
};

}}}
