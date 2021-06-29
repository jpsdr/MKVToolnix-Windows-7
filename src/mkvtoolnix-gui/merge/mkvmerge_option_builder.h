#pragma once

#include "common/common_pch.h"

#include <QString>
#include <QHash>

#include "mkvtoolnix-gui/merge/enums.h"
#include "mkvtoolnix-gui/util/command_line_options.h"

namespace mtx::gui::Merge {

struct MkvmergeOptionBuilder {
  Util::CommandLineOptions options;
  QHash<TrackType, unsigned int> numTracksOfType;
  QHash<TrackType, QStringList> enabledTrackIds;

  MkvmergeOptionBuilder() {}
};

}
