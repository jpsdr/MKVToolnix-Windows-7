#include "common/common_pch.h"

#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>

#include "common/qt.h"
#include "common/strings/formatting.h"
#include "common/strings/parsing.h"
#include "mkvtoolnix-gui/chapter_editor/generate_sub_chapters_parameters_dialog.h"
#include "mkvtoolnix-gui/chapter_editor/tool.h"
#include "mkvtoolnix-gui/forms/chapter_editor/generate_sub_chapters_parameters_dialog.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/util/settings.h"
#include "mkvtoolnix-gui/util/widget.h"

namespace mtx::gui::ChapterEditor {

using namespace mtx::gui;

GenerateSubChaptersParametersDialog::GenerateSubChaptersParametersDialog(QWidget *parent,
                                                                         int firstChapterNumber,
                                                                         uint64_t startTimestamp,
                                                                         QStringList const &additionalLanguages)
  : QDialog{parent}
  , m_ui{new Ui::GenerateSubChaptersParametersDialog}
{
  setupUi(firstChapterNumber, startTimestamp, additionalLanguages);
  retranslateUi();
}

GenerateSubChaptersParametersDialog::~GenerateSubChaptersParametersDialog() {
}

void
GenerateSubChaptersParametersDialog::setupUi(int firstChapterNumber,
                                             uint64_t startTimestamp,
                                             QStringList const &additionalLanguages) {
  auto &cfg = Util::Settings::get();

  m_ui->setupUi(this);

  m_ui->sbFirstChapterNumber->setValue(firstChapterNumber);
  m_ui->leStartTimestamp->setText(Q(mtx::string::format_timestamp(startTimestamp)));
  m_ui->leNameTemplate->setText(cfg.m_chapterNameTemplate);

  m_ui->ldwLanguage->setAdditionalLanguages(additionalLanguages);
  m_ui->ldwLanguage->setLanguage(cfg.m_defaultChapterLanguage);
  m_ui->ldwLanguage->enableClearingLanguage(true);
  m_ui->ldwLanguage->setClearTitle(QY("– Set to none –"));

  m_ui->sbNumberOfEntries->setFocus();

  m_ui->ldwLanguage->registerBuddyLabel(*m_ui->lLanguage);

  adjustSize();

  connect(m_ui->leStartTimestamp, &QLineEdit::textChanged,     this, &GenerateSubChaptersParametersDialog::verifyStartTimestamp);
  connect(m_ui->buttonBox,        &QDialogButtonBox::accepted, this, &GenerateSubChaptersParametersDialog::accept);
  connect(m_ui->buttonBox,        &QDialogButtonBox::rejected, this, &GenerateSubChaptersParametersDialog::reject);
}

void
GenerateSubChaptersParametersDialog::retranslateUi() {
  Util::setToolTip(m_ui->leStartTimestamp, QY("The format is either the form 'HH:MM:SS.nnnnnnnnn' or a number followed by one of the units 's', 'ms' or 'us'."));
  Util::setToolTip(m_ui->leNameTemplate, Tool::chapterNameTemplateToolTip());
}

int
GenerateSubChaptersParametersDialog::numberOfEntries()
  const {
  return m_ui->sbNumberOfEntries->value();
}

uint64_t
GenerateSubChaptersParametersDialog::durationInNs()
  const {
  return static_cast<uint64_t>(m_ui->sbDuration->value()) * 1000000000ull;
}

int
GenerateSubChaptersParametersDialog::firstChapterNumber()
  const {
  return m_ui->sbFirstChapterNumber->value();
}

uint64_t
GenerateSubChaptersParametersDialog::startTimestamp()
  const {
  int64_t timestamp = 0;
  mtx::string::parse_timestamp(to_utf8(m_ui->leStartTimestamp->text()), timestamp);

  return timestamp;
}

QString
GenerateSubChaptersParametersDialog::nameTemplate()
  const {
  return m_ui->leNameTemplate->text();
}

mtx::bcp47::language_c
GenerateSubChaptersParametersDialog::language()
  const {
  return m_ui->ldwLanguage->language();
}

void
GenerateSubChaptersParametersDialog::verifyStartTimestamp() {
  int64_t dummy = 0;
  Util::buttonForRole(m_ui->buttonBox)->setEnabled(mtx::string::parse_timestamp(to_utf8(m_ui->leStartTimestamp->text()), dummy));
}

}
