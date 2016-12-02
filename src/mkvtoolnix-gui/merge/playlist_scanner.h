#ifndef MTX_MKVTOOLNIX_GUI_MERGE_PLAYLIST_SCANNER_H
#define MTX_MKVTOOLNIX_GUI_MERGE_PLAYLIST_SCANNER_H

#include "common/common_pch.h"

#include "mkvtoolnix-gui/merge/source_file.h"

#include <QList>

class QWidget;

namespace mtx { namespace gui { namespace Merge {

class PlaylistScanner {
protected:
  QWidget *m_parent;

public:
  explicit PlaylistScanner(QWidget *parent);

  QList<SourceFilePtr> checkAddingPlaylists(QList<SourceFilePtr> const &files) const;
  QList<SourceFilePtr> scanForPlaylists(QFileInfoList const &otherFiles) const;

protected:
  bool askScanForPlaylists(SourceFile const &file, unsigned int numOtherFiles) const;
  SourceFilePtr checkOneFile(SourceFilePtr const &file) const;
};

}}}

#endif // MTX_MKVTOOLNIX_GUI_MERGE_PLAYLIST_SCANNER_H
