#include "common/common_pch.h"

#include <QFileInfo>

#include "common/qt.h"
#include "mkvtoolnix-gui/forms/merge/adding_appending_files_dialog.h"
#include "mkvtoolnix-gui/merge/adding_appending_files_dialog.h"
#include "mkvtoolnix-gui/util/util.h"

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

  adjustSize();
}

AddingAppendingFilesDialog::~AddingAppendingFilesDialog() {
}

AddingAppendingFilesDialog::Decision
AddingAppendingFilesDialog::decision()
  const {
  return ui->rbAdd->isChecked()    ? Decision::Add
       : ui->rbAppend->isChecked() ? Decision::Append
       :                             Decision::AddAdditionalParts;
}

int
AddingAppendingFilesDialog::fileIndex()
  const {
  return ui->cbFileName->currentIndex();
}

void
AddingAppendingFilesDialog::selectionChanged() {
  ui->cbFileName->setEnabled(!ui->rbAdd->isChecked());
}

}}}
