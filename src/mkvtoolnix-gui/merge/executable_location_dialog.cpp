#include "common/common_pch.h"

#include <QDialog>
#include <QPushButton>

#include "common/qt.h"
#include "mkvtoolnix-gui/forms/merge/executable_location_dialog.h"
#include "mkvtoolnix-gui/merge/executable_location_dialog.h"
#include "mkvtoolnix-gui/util/file_dialog.h"

namespace mtx::gui::Merge {

using namespace mtx::gui;

ExecutableLocationDialog::ExecutableLocationDialog(QWidget *parent,
                                                   QString const &executable)
  : QDialog{parent}
  , m_ui{new Ui::ExecutableLocationDialog}
{
  // Setup UI controls.
  m_ui->setupUi(this);

  m_ui->leExecutable->setText(executable);
  m_ui->lURL->setVisible(false);

  connect(m_ui->pbBrowse,  &QPushButton::clicked,       this, &ExecutableLocationDialog::browse);
  connect(m_ui->buttonBox, &QDialogButtonBox::accepted, this, &ExecutableLocationDialog::accept);
  connect(m_ui->buttonBox, &QDialogButtonBox::rejected, this, &ExecutableLocationDialog::reject);
}

ExecutableLocationDialog::~ExecutableLocationDialog() {
}

ExecutableLocationDialog &
ExecutableLocationDialog::setInfo(QString const &title,
                                  QString const &text) {
  setWindowTitle(title);
  m_ui->lTitle->setText(title);
  m_ui->lText->setText(text);

  return *this;
}

ExecutableLocationDialog &
ExecutableLocationDialog::setURL(QString const &url) {
  m_ui->lURL->setText(Q("<a href=\"%1\">%1</a>").arg(url.toHtmlEscaped()));
  m_ui->lURL->setVisible(true);

  return *this;
}

QString
ExecutableLocationDialog::executable()
  const {
  return m_ui->leExecutable->text();
}

void
ExecutableLocationDialog::browse() {
  auto filters = QStringList{};

#if defined(SYS_WINDOWS)
  filters << QY("Executable files") + Q(" (*.exe)");
#endif
  filters << QY("All files") + Q(" (*)");

  auto fileName = Util::getOpenFileName(this, QY("Select executable"), m_ui->leExecutable->text(), filters.join(Q(";;")));
  if (!fileName.isEmpty())
    m_ui->leExecutable->setText(fileName);
}

}
