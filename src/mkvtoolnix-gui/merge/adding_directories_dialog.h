#pragma once

#include "common/common_pch.h"

#include <QDialog>

#include "mkvtoolnix-gui/util/settings.h"

namespace mtx::gui::Merge {

class Tab;

namespace Ui {
class AddingDirectoriesDialog;
}

class AddingDirectoriesDialog : public QDialog {
  Q_OBJECT
protected:
  std::unique_ptr<Ui::AddingDirectoriesDialog> ui;

public:
  explicit AddingDirectoriesDialog(QWidget *parent, Util::Settings::MergeAddingDirectoriesPolicy decision);
  ~AddingDirectoriesDialog();

  Util::Settings::MergeAddingDirectoriesPolicy decision() const;
  bool alwaysUseThisDecision() const;

public:
  static QStringList optionDescriptions();
};

}
