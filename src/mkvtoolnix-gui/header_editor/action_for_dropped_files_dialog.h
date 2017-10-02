#pragma once

#include "common/common_pch.h"

#include <QDialog>

#include "mkvtoolnix-gui/util/settings.h"

namespace mtx { namespace gui { namespace HeaderEditor {

namespace Ui {
class ActionForDroppedFilesDialog;
}

class ActionForDroppedFilesDialog : public QDialog {
  Q_OBJECT;
protected:
  std::unique_ptr<Ui::ActionForDroppedFilesDialog> ui;

public:
  explicit ActionForDroppedFilesDialog(QWidget *parent);
  ~ActionForDroppedFilesDialog();

  Util::Settings::HeaderEditorDroppedFilesPolicy decision() const;
  bool alwaysUseThisDecision() const;
};

}}}
