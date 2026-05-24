#include "common/common_pch.h"

#include <QFileInfo>
#include <QPushButton>

#include "common/qt.h"
#include "mkvtoolnix-gui/forms/merge/adding_directories_dialog.h"
#include "mkvtoolnix-gui/merge/adding_directories_dialog.h"
#include "mkvtoolnix-gui/merge/tab.h"

namespace mtx::gui::Merge {

using namespace mtx::gui;

AddingDirectoriesDialog::AddingDirectoriesDialog(QWidget *parent,
                                                 Util::Settings::MergeAddingDirectoriesPolicy decision)
  : QDialog{parent}
  , ui{new Ui::AddingDirectoriesDialog}
{
  // Setup UI controls.
  ui->setupUi(this);

  if (decision == Util::Settings::MergeAddingDirectoriesPolicy::AddEachDirectoryToNew)
    ui->rbAddEachDirectoryToNew->setChecked(true);
  else
    ui->rbFlat->setChecked(true);

  QString fullText;

  for (auto const &text : optionDescriptions())
    fullText += Q("<li>%1</li>").arg(text.toHtmlEscaped());

  ui->lOptions->setText(Q("<ol>%1</ol>").arg(fullText));

  adjustSize();

  connect(ui->buttonBox,            &QDialogButtonBox::accepted, this, &AddingDirectoriesDialog::accept);
  connect(ui->buttonBox,            &QDialogButtonBox::rejected, this, &AddingDirectoriesDialog::reject);
}

AddingDirectoriesDialog::~AddingDirectoriesDialog() {
}

QStringList
AddingDirectoriesDialog::optionDescriptions() {
  QStringList texts;

  texts << QY("The program can collect all files from all directories in a single list. What happens with that list is determined by how dragging & dropping/copying & pasting files is configured.");
  texts << QY("For each directory a new multiplex settings tab can be created. All files from that directory will be added to this new tab. The configuration about dragging & dropping/copying & pasting files is ignored in this case.");

  return texts;
}

Util::Settings::MergeAddingDirectoriesPolicy
AddingDirectoriesDialog::decision()
  const {
  return ui->rbFlat->isChecked() ? Util::Settings::MergeAddingDirectoriesPolicy::Flat
       :                           Util::Settings::MergeAddingDirectoriesPolicy::AddEachDirectoryToNew;
}

bool
AddingDirectoriesDialog::alwaysUseThisDecision()
  const {
  return ui->cbAlwaysUseThisDecision->isEnabled() && ui->cbAlwaysUseThisDecision->isChecked();
}

}
