#include "common/common_pch.h"

#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>

#include "common/qt.h"
#include "common/strings/parsing.h"
#include "mkvtoolnix-gui/forms/chapter_editor/mass_modification_dialog.h"
#include "mkvtoolnix-gui/chapter_editor/mass_modification_dialog.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/util/util.h"

namespace mtx { namespace gui { namespace ChapterEditor {

using namespace mtx::gui;

MassModificationDialog::MassModificationDialog(QWidget *parent,
                                               bool editionOrChapterSelected)
  : QDialog{parent}
  , m_ui{new Ui::MassModificationDialog}
  , m_editionOrChapterSelected{editionOrChapterSelected}
{
  setupUi();
  retranslateUi();
}

MassModificationDialog::~MassModificationDialog() {
}

void
MassModificationDialog::setupUi() {
  m_ui->setupUi(this);

  m_ui->cbLanguage->setup();
  m_ui->cbCountry->setup(true, QY("– set to none –"));

  auto mw = MainWindow::get();
  connect(m_ui->cbShift,           &QCheckBox::toggled,             m_ui->leShiftBy,   &QLineEdit::setEnabled);
  connect(m_ui->leShiftBy,         &QLineEdit::textChanged,         this,              &MassModificationDialog::shiftByStateChanged);
  connect(m_ui->cbSetLanguage,     &QCheckBox::toggled,             m_ui->cbLanguage,  &QComboBox::setEnabled);
  connect(m_ui->cbSetCountry,      &QCheckBox::toggled,             m_ui->cbCountry,   &QComboBox::setEnabled);
  connect(m_ui->cbConstrictExpand, &QCheckBox::toggled,             m_ui->rbConstrict, &QRadioButton::setEnabled);
  connect(m_ui->cbConstrictExpand, &QCheckBox::toggled,             m_ui->rbExpand,    &QRadioButton::setEnabled);
  connect(m_ui->buttonBox,         &QDialogButtonBox::accepted,     this,              &MassModificationDialog::accept);
  connect(m_ui->buttonBox,         &QDialogButtonBox::rejected,     this,              &MassModificationDialog::reject);

  connect(mw,                      &MainWindow::preferencesChanged, m_ui->cbLanguage,  &Util::ComboBoxBase::reInitialize);
  connect(mw,                      &MainWindow::preferencesChanged, m_ui->cbCountry,   &Util::ComboBoxBase::reInitialize);

  m_ui->leShiftBy->setEnabled(false);
  m_ui->cbLanguage->setEnabled(false);
  m_ui->cbCountry->setEnabled(false);
  m_ui->rbConstrict->setEnabled(false);
  m_ui->rbExpand->setEnabled(false);
  m_ui->rbConstrict->setChecked(true);

  m_ui->cbShift->setFocus();
}

void
MassModificationDialog::retranslateUi() {
  Util::setToolTip(m_ui->leShiftBy,
                   Q("%1 %2")
                   .arg(QY("The format is either the form 'HH:MM:SS.nnnnnnnnn' or a number followed by one of the units 's', 'ms' or 'us'."))
                   .arg(QY("Negative values are allowed.")));

  if (m_editionOrChapterSelected)
    m_ui->lTitle->setText(QY("Please select the actions to apply to the selected edition or chapter and all of its children."));
  else
    m_ui->lTitle->setText(QY("Please select the actions to apply to all editions, chapters and sub-chapters."));
}

MassModificationDialog::Actions
MassModificationDialog::actions()
  const {
  auto result = Actions{};

  if (m_ui->cbShift->isChecked())                                             result |= Action::Shift;
  if (m_ui->cbSort->isChecked())                                              result |= Action::Sort;
  if (m_ui->cbConstrictExpand->isChecked() && m_ui->rbConstrict->isChecked()) result |= Action::Constrict;
  if (m_ui->cbConstrictExpand->isChecked() && m_ui->rbExpand->isChecked())    result |= Action::Expand;
  if (m_ui->cbSetLanguage->isChecked())                                       result |= Action::SetLanguage;
  if (m_ui->cbSetCountry->isChecked())                                        result |= Action::SetCountry;

  return result;
}

int64_t
MassModificationDialog::shiftBy()
  const {
  auto timecode = int64_t{};
  parse_timecode(to_utf8(m_ui->leShiftBy->text()), timecode, true);
  return timecode;
}

QString
MassModificationDialog::language()
  const {
  return m_ui->cbLanguage->currentData().toString();
}

QString
MassModificationDialog::country()
  const {
  return m_ui->cbCountry->currentData().toString();
}

void
MassModificationDialog::shiftByStateChanged() {
  auto shifting      = m_ui->cbShift->isChecked();
  auto timecode      = int64_t{};
  auto timecodeValid = parse_timecode(to_utf8(m_ui->leShiftBy->text()), timecode, true);

  m_ui->leShiftBy->setEnabled(shifting);
  Util::buttonForRole(m_ui->buttonBox)->setEnabled(!shifting || timecodeValid);
}

}}}
