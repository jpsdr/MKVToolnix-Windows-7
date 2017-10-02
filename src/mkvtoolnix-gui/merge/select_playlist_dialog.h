#pragma once

#include "common/common_pch.h"

#include "mkvtoolnix-gui/merge/source_file.h"

#include <QDialog>
#include <QList>

class QFileInfo;
class QTreeWidgetItem;
class Track;

namespace mtx { namespace gui { namespace Merge {

namespace Ui {
class SelectPlaylistDialog;
}

class SelectPlaylistDialog : public QDialog {
  Q_OBJECT;
protected:
  std::unique_ptr<Ui::SelectPlaylistDialog> ui;
  QList<SourceFilePtr> m_scannedFiles;

public:
  explicit SelectPlaylistDialog(QWidget *parent, QList<SourceFilePtr> const &scannedFiles);
  ~SelectPlaylistDialog();

  SourceFilePtr select();

protected slots:
  void onScannedFileSelected(QTreeWidgetItem *current, QTreeWidgetItem *);

protected:
  void setupUi();
};

}}}
