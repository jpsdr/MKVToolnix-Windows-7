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

namespace mtx { namespace gui { namespace ChapterEditor {

using namespace mtx::gui;

GenerateSubChaptersParametersDialog::GenerateSubChaptersParametersDialog(QWidget *parent,
                                                                         int firstChapterNumber,
                                                                         uint64_t startTimestamp,
                                                                         QStringList const &additionalLanguages,
                                                                         QStringList const &additionalCountryCodes)
  : QDialog{parent}
  , m_ui{new Ui::GenerateSubChaptersParametersDialog}
{
  setupUi(firstChapterNumber, startTimestamp, additionalLanguages, additionalCountryCodes);
  retranslateUi();
}

GenerateSubChaptersParametersDialog::~GenerateSubChaptersParametersDialog() {
}

void
GenerateSubChaptersParametersDialog::setupUi(int firstChapterNumber,
                                             uint64_t startTimestamp,
                                             QStringList const &additionalLanguages,
                                             QStringList const &additionalCountryCodes) {
  auto &cfg = Util::Settings::get();

  m_ui->setupUi(this);

  m_ui->sbFirstChapterNumber->setValue(firstChapterNumber);
  m_ui->leStartTimestamp->setText(Q(format_timestamp(startTimestamp)));
  m_ui->leNameTemplate->setText(cfg.m_chapterNameTemplate);

  m_ui->cbLanguage->setAdditionalItems(additionalLanguages).setup().setCurrentByData(cfg.m_defaultChapterLanguage);
  m_ui->cbCountry->setAdditionalItems(additionalCountryCodes).setup(true, QY("– Set to none –")).setCurrentByData(cfg.m_defaultChapterCountry);

  m_ui->sbNumberOfEntries->setFocus();

  adjustSize();

  auto mw = MainWindow::get();
  connect(m_ui->leStartTimestamp, &QLineEdit::textChanged,         this,             &GenerateSubChaptersParametersDialog::verifyStartTimestamp);
  connect(m_ui->buttonBox,        &QDialogButtonBox::accepted,     this,             &GenerateSubChaptersParametersDialog::accept);
  connect(m_ui->buttonBox,        &QDialogButtonBox::rejected,     this,             &GenerateSubChaptersParametersDialog::reject);
  connect(mw,                     &MainWindow::preferencesChanged, m_ui->cbLanguage, &Util::ComboBoxBase::reInitialize);
  connect(mw,                     &MainWindow::preferencesChanged, m_ui->cbCountry,  &Util::ComboBoxBase::reInitialize);
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
  parse_timestamp(to_utf8(m_ui->leStartTimestamp->text()), timestamp);

  return timestamp;
}

QString
GenerateSubChaptersParametersDialog::nameTemplate()
  const {
  return m_ui->leNameTemplate->text();
}

QString
GenerateSubChaptersParametersDialog::language()
  const {
  return m_ui->cbLanguage->currentData().toString();
}

OptQString
GenerateSubChaptersParametersDialog::country()
  const {
  auto countryStr = m_ui->cbCountry->currentData().toString();
  return countryStr.isEmpty() ? OptQString{} : OptQString{ countryStr };
}

void
GenerateSubChaptersParametersDialog::verifyStartTimestamp() {
  int64_t dummy = 0;
  Util::buttonForRole(m_ui->buttonBox)->setEnabled(parse_timestamp(to_utf8(m_ui->leStartTimestamp->text()), dummy));
}

}}}
