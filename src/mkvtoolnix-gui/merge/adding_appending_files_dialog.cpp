#include "common/common_pch.h"

#include <QFileInfo>

#include "common/qt.h"
#include "mkvtoolnix-gui/forms/merge/adding_appending_files_dialog.h"
#include "mkvtoolnix-gui/merge/adding_appending_files_dialog.h"

namespace mtx { namespace gui { namespace Merge {

using namespace mtx::gui;

AddingAppendingFilesDialog::AddingAppendingFilesDialog(QWidget *parent,
                                                       QList<SourceFilePtr> const &files)
  : QDialog{parent}
  , ui{new Ui::AddingAppendingFilesDialog}
{
  // Setup UI controls.
  ui->setupUi(this);

  ui->rbAdd->setChecked(true);
  ui->cbFileName->setEnabled(false);

  for (auto const &file : files) {
    auto info = QFileInfo{file->m_fileName};
    ui->cbFileName->addItem(Q("%1 (%2)").arg(info.fileName()).arg(info.path()));
  }

  if (files.isEmpty()) {
    ui->lAppendTo->setEnabled(false);
    ui->rbAppend->setEnabled(false);
    ui->rbAddAdditionalParts->setEnabled(false);
  }

  adjustSize();
}

AddingAppendingFilesDialog::~AddingAppendingFilesDialog() {
}

Util::Settings::AddingAppendingFilesPolicy
AddingAppendingFilesDialog::decision()
  const {
  return ui->rbAdd->isChecked()          ? Util::Settings::AddingAppendingFilesPolicy::Add
       : ui->rbAppend->isChecked()       ? Util::Settings::AddingAppendingFilesPolicy::Append
       : ui->rbAddToNew->isChecked()     ? Util::Settings::AddingAppendingFilesPolicy::AddToNew
       : ui->rbAddEachToNew->isChecked() ? Util::Settings::AddingAppendingFilesPolicy::AddEachToNew
       :                                   Util::Settings::AddingAppendingFilesPolicy::AddAdditionalParts;
}

int
AddingAppendingFilesDialog::fileIndex()
  const {
  return ui->cbFileName->currentIndex();
}

void
AddingAppendingFilesDialog::selectionChanged() {
  ui->cbFileName->setEnabled(ui->rbAppend->isChecked() || ui->rbAddAdditionalParts->isChecked());
  ui->cbAlwaysUseThisDecision->setEnabled(ui->rbAdd->isChecked() || ui->rbAddToNew->isChecked() || ui->rbAddEachToNew->isChecked());
}

bool
AddingAppendingFilesDialog::alwaysUseThisDecision()
  const {
  return ui->cbAlwaysUseThisDecision->isEnabled() && ui->cbAlwaysUseThisDecision->isChecked();
}

}}}
