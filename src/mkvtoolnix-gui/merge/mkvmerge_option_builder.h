#ifndef MTX_MKVTOOLNIX_GUI_MERGE_MKVMERGE_OPTION_BUILDER_H
#define MTX_MKVTOOLNIX_GUI_MERGE_MKVMERGE_OPTION_BUILDER_H

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

#endif // MTX_MKVTOOLNIX_GUI_MERGE_MKVMERGE_OPTION_BUILDER_H
