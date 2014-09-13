#include "common/common_pch.h"

#include <QApplication>
#include <QStyle>

#include "common/qt.h"
#include "mkvtoolnix-gui/forms/preview_warning_dialog.h"
#include "mkvtoolnix-gui/main_window/preview_warning_dialog.h"

PreviewWarningDialog::PreviewWarningDialog(QWidget *parent)
  : QDialog{parent}
  , ui{new Ui::PreviewWarningDialog}
{
  // Setup UI controls.
  ui->setupUi(this);

  auto style    = QApplication::style();
  auto iconSize = style->pixelMetric(QStyle::PM_MessageBoxIconSize);

  ui->iconLabel->setPixmap(style->standardIcon(QStyle::SP_MessageBoxWarning).pixmap(iconSize, iconSize));
}

PreviewWarningDialog::~PreviewWarningDialog() {
}
