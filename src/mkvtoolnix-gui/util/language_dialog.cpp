#include "common/common_pch.h"

#include <QDialogButtonBox>
#include <QPushButton>

#include "common/qt.h"
#include "mkvtoolnix-gui/forms/util/language_dialog.h"
#include "mkvtoolnix-gui/util/language_dialog.h"

namespace mtx::gui::Util {

// ------------------------------------------------------------

class LanguageDialogPrivate {
  friend class LanguageDialog;

  std::unique_ptr<Ui::LanguageDialog> ui{new Ui::LanguageDialog};
};

LanguageDialog::LanguageDialog(QWidget *parent)
  : QDialog{parent}
  , p_ptr{new LanguageDialogPrivate}
{
  auto p = p_func();

  p->ui->setupUi(this);

  setupConnections();
}

LanguageDialog::~LanguageDialog() {
}

void
LanguageDialog::retranslateUi() {
  p_func()->ui->retranslateUi(this);
}

void
LanguageDialog::setupConnections() {
  auto &p       = *p_func();
  auto okButton = p.ui->buttonBox->button(QDialogButtonBox::Ok);

  connect(p.ui->languageWidget, &LanguageWidget::tagValidityChanged, okButton, &QPushButton::setEnabled);
  connect(p.ui->buttonBox,      &QDialogButtonBox::accepted,         this,     &QDialog::accept);
  connect(p.ui->buttonBox,      &QDialogButtonBox::rejected,         this,     &QDialog::reject);
}

void
LanguageDialog::setAdditionalLanguages(QStringList const &additionalLanguages) {
  p_func()->ui->languageWidget->setAdditionalLanguages(additionalLanguages);
}

void
LanguageDialog::setLanguage(mtx::bcp47::language_c const &language) {
  p_func()->ui->languageWidget->setLanguage(language);
}

mtx::bcp47::language_c
LanguageDialog::language()
  const {
  return p_func()->ui->languageWidget->language();
}

}
