#include "common/common_pch.h"

#include <QPushButton>

#include "common/qt.h"
#include "common/strings/parsing.h"
#include "mkvtoolnix-gui/forms/chapter_editor/mass_modification_dialog.h"
#include "mkvtoolnix-gui/chapter_editor/mass_modification_dialog.h"
#include "mkvtoolnix-gui/util/util.h"

namespace mtx { namespace gui { namespace ChapterEditor {

using namespace mtx::gui;

MassModificationDialog::MassModificationDialog(QWidget *parent,
                                               bool editionOrChapterSelected)
  : QDialog{parent}
  , ui{new Ui::MassModificationDialog}
{
  // Setup UI controls.
  ui->setupUi(this);

  Util::setToolTip(ui->leShiftBy,
                   Q("%1 %2")
                   .arg(QY("The format is either the form 'HH:MM:SS.nnnnnnnnn' or a number followed by one of the units 's', 'ms' or 'us'."))
                   .arg(QY("Negative values are allowed.")));

  if (editionOrChapterSelected)
    ui->lTitle->setText(QY("Please select the action to apply to the selected edition or chapter and all of its children."));
  else
    ui->lTitle->setText(QY("Please select the action to apply to all editions, chapters and sub-chapters."));

  ui->rbShift->setChecked(true);
  ui->leShiftBy->setEnabled(true);
  ui->leShiftBy->setFocus();

  selectionOrShiftByChanged();
}

MassModificationDialog::~MassModificationDialog() {
}

MassModificationDialog::Decision
MassModificationDialog::decision()
  const {
  return ui->rbShift->isChecked()     ? Decision::Shift
       : ui->rbSort->isChecked()      ? Decision::Sort
       : ui->rbConstrict->isChecked() ? Decision::Constrict
       :                                Decision::Expand;
}

int64_t
MassModificationDialog::shiftBy()
  const {
  auto timecode = int64_t{};
  parse_timecode(to_utf8(ui->leShiftBy->text()), timecode, true);
  return timecode;
}

void
MassModificationDialog::selectionOrShiftByChanged() {
  auto shifting      = decision() == Decision::Shift;
  auto timecode      = int64_t{};
  auto timecodeValid = parse_timecode(to_utf8(ui->leShiftBy->text()), timecode, true);

  ui->leShiftBy->setEnabled(shifting);
  Util::buttonForRole(ui->buttonBox)->setEnabled(!shifting || timecodeValid);
}

}}}
