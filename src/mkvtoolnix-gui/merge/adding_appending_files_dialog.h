#pragma once

#include "common/common_pch.h"

#include <QDialog>

#include "mkvtoolnix-gui/merge/source_file.h"
#include "mkvtoolnix-gui/util/settings.h"

namespace mtx::gui::Merge {

class Tab;

namespace Ui {
class AddingAppendingFilesDialog;
}

class AddingAppendingFilesDialog : public QDialog {
  Q_OBJECT
protected:
  std::unique_ptr<Ui::AddingAppendingFilesDialog> ui;

public:
  explicit AddingAppendingFilesDialog(QWidget *parent, Tab &tab);
  ~AddingAppendingFilesDialog();

  void setDefaults(Util::Settings::MergeAddingAppendingFilesPolicy decision, int fileNum);

  Util::Settings::MergeAddingAppendingFilesPolicy decision() const;
  int fileNum() const;
  bool alwaysUseThisDecision() const;

public Q_SLOTS:
  void selectionChanged();
};

}
