#include "common/common_pch.h"

#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>

#include "common/qt.h"
#include "common/strings/formatting.h"
#include "common/strings/parsing.h"
#include "mkvtoolnix-gui/chapter_editor/renumber_sub_chapters_parameters_dialog.h"
#include "mkvtoolnix-gui/chapter_editor/tool.h"
#include "mkvtoolnix-gui/forms/chapter_editor/renumber_sub_chapters_parameters_dialog.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/util/settings.h"
#include "mkvtoolnix-gui/util/widget.h"

namespace mtx { namespace gui { namespace ChapterEditor {

using namespace mtx::gui;

RenumberSubChaptersParametersDialog::RenumberSubChaptersParametersDialog(QWidget *parent,
                                                                         int firstChapterNumber,
                                                                         QStringList const &existingSubChapters,
                                                                         QStringList const &additionalLanguages)
  : QDialog{parent}
  , m_ui{new Ui::RenumberSubChaptersParametersDialog}
{
  setupUi(firstChapterNumber, existingSubChapters, additionalLanguages);
}

RenumberSubChaptersParametersDialog::~RenumberSubChaptersParametersDialog() {
}

void
RenumberSubChaptersParametersDialog::setupUi(int firstChapterNumber,
                                             QStringList const &existingSubChapters,
                                             QStringList const &additionalLanguages) {
  auto &cfg = Util::Settings::get();

  m_ui->setupUi(this);

  m_ui->cbFirstEntryToRenumber->addItems(existingSubChapters);
  m_ui->sbNumberOfEntries->setMaximum(existingSubChapters.count());
  m_ui->sbFirstChapterNumber->setValue(firstChapterNumber);
  m_ui->leNameTemplate->setText(cfg.m_chapterNameTemplate);

  m_ui->cbLanguageOfNamesToReplace->setAdditionalItems(additionalLanguages).setup();
  m_ui->cbLanguageOfNamesToReplace->insertItem(0, QY("– First chapter name regardless of its language –"),  static_cast<int>(NameMatch::First));
  m_ui->cbLanguageOfNamesToReplace->insertItem(1, QY("– All chapter names regardless of their language –"), static_cast<int>(NameMatch::All));
  m_ui->cbLanguageOfNamesToReplace->insertSeparator(2);

  m_ui->cbLanguageOfNamesToReplace->setCurrentIndex(0);

  Util::setToolTip(m_ui->leNameTemplate, Tool::chapterNameTemplateToolTip());

  m_ui->cbFirstEntryToRenumber->setFocus();

  adjustSize();

  connect(m_ui->buttonBox, &QDialogButtonBox::accepted, this, &RenumberSubChaptersParametersDialog::accept);
  connect(m_ui->buttonBox, &QDialogButtonBox::rejected, this, &RenumberSubChaptersParametersDialog::reject);
}

int
RenumberSubChaptersParametersDialog::firstEntryToRenumber()
  const {
  return m_ui->cbFirstEntryToRenumber->currentIndex();
}

int
RenumberSubChaptersParametersDialog::numberOfEntries()
  const {
  return m_ui->sbNumberOfEntries->value();
}

int
RenumberSubChaptersParametersDialog::firstChapterNumber()
  const {
  return m_ui->sbFirstChapterNumber->value();
}

QString
RenumberSubChaptersParametersDialog::nameTemplate()
  const {
  return m_ui->leNameTemplate->text();
}

RenumberSubChaptersParametersDialog::NameMatch
RenumberSubChaptersParametersDialog::nameMatchingMode()
  const {
  auto data = m_ui->cbLanguageOfNamesToReplace->currentData();

  if (data.type() == static_cast<QVariant::Type>(QMetaType::Int))
    return static_cast<NameMatch>(data.toInt());
  return NameMatch::ByLanguage;
}

QString
RenumberSubChaptersParametersDialog::languageOfNamesToReplace()
  const {
  if (nameMatchingMode() != NameMatch::ByLanguage)
    return Q("");
  return m_ui->cbLanguageOfNamesToReplace->currentData().toString();
}

bool
RenumberSubChaptersParametersDialog::skipHidden()
  const {
  return m_ui->cbSkipHidden->isChecked();
}

}}}
