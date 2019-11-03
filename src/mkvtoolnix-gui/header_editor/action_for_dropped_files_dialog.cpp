#include "common/common_pch.h"

#include "common/qt.h"
#include "mkvtoolnix-gui/forms/header_editor/action_for_dropped_files_dialog.h"
#include "mkvtoolnix-gui/header_editor/action_for_dropped_files_dialog.h"

namespace mtx::gui::HeaderEditor {

using namespace mtx::gui;

ActionForDroppedFilesDialog::ActionForDroppedFilesDialog(QWidget *parent)
  : QDialog{parent}
  , ui{new Ui::ActionForDroppedFilesDialog}
{
  // Setup UI controls.
  ui->setupUi(this);

  ui->rbOpen->setChecked(true);

  adjustSize();

  connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &ActionForDroppedFilesDialog::accept);
  connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &ActionForDroppedFilesDialog::reject);
}

ActionForDroppedFilesDialog::~ActionForDroppedFilesDialog() {
}

Util::Settings::HeaderEditorDroppedFilesPolicy
ActionForDroppedFilesDialog::decision()
  const {
  return ui->rbOpen->isChecked() ? Util::Settings::HeaderEditorDroppedFilesPolicy::Open
       :                           Util::Settings::HeaderEditorDroppedFilesPolicy::AddAttachments;
}

bool
ActionForDroppedFilesDialog::alwaysUseThisDecision()
  const {
  return ui->cbAlwaysUseThisDecision->isChecked();
}

}
