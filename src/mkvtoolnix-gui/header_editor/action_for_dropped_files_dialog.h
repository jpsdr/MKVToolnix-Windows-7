#ifndef MTX_MKVTOOLNIX_GUI_ACTION_FOR_DROPPED_FILES_DIALOG_H
#define MTX_MKVTOOLNIX_GUI_ACTION_FOR_DROPPED_FILES_DIALOG_H

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

#endif // MTX_MKVTOOLNIX_GUI_ACTION_FOR_DROPPED_FILES_DIALOG_H
