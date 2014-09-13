#ifndef MTX_MKVTOOLNIX_GUI_MAIN_WINDOW_PREVIEW_WARNING_DIALOG_H
#define MTX_MKVTOOLNIX_GUI_MAIN_WINDOW_PREVIEW_WARNING_DIALOG_H

#include "common/common_pch.h"

#include <QDialog>

namespace Ui {
class PreviewWarningDialog;
}

class PreviewWarningDialog : public QDialog {
  Q_OBJECT;

protected:
  // UI stuff:
  std::unique_ptr<Ui::PreviewWarningDialog> ui;

public:
  explicit PreviewWarningDialog(QWidget *parent);
  ~PreviewWarningDialog();
};

#endif // MTX_MKVTOOLNIX_GUI_MAIN_WINDOW_PREVIEW_WARNING_DIALOG_H
