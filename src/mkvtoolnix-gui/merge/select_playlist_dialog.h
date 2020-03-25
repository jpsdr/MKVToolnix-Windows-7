#pragma once

#include "common/common_pch.h"

#include "common/bluray/disc_library.h"
#include "mkvtoolnix-gui/merge/source_file.h"

#include <QDialog>
#include <QList>

class QFileInfo;
class QTreeWidgetItem;
class Track;

namespace mtx::gui::Merge {

namespace Ui {
class SelectPlaylistDialog;
}

class SelectPlaylistDialog : public QDialog {
  Q_OBJECT
protected:
  std::unique_ptr<Ui::SelectPlaylistDialog> ui;
  QList<SourceFilePtr> m_scannedFiles;
  std::optional<mtx::bluray::disc_library::disc_library_t> m_discLibrary;

public:
  explicit SelectPlaylistDialog(QWidget *parent, QList<SourceFilePtr> const &scannedFiles, std::optional<mtx::bluray::disc_library::disc_library_t> const &discLibrary);
  ~SelectPlaylistDialog();

  SourceFilePtr select();

protected slots:
  void onScannedFileSelected(QTreeWidgetItem *current, QTreeWidgetItem *);

protected:
  void setupUi();
  void setupScannedFiles();
  void setupDiscLibrary();
};

}
