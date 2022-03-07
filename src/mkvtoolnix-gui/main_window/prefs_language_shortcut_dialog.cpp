#include "common/common_pch.h"

#include <QPushButton>

#include "common/qt.h"

#include "mkvtoolnix-gui/forms/main_window/prefs_language_shortcut_dialog.h"
#include "mkvtoolnix-gui/main_window/prefs_language_shortcut_dialog.h"
#include "mkvtoolnix-gui/util/widget.h"

namespace mtx::gui {

class PrefsLanguageShortcutDialogPrivate {
  friend class PrefsLanguageShortcutDialog;

  std::unique_ptr<Ui::PrefsLanguageShortcutDialog> ui{new Ui::PrefsLanguageShortcutDialog};
};

PrefsLanguageShortcutDialog::PrefsLanguageShortcutDialog(QWidget *parent,
                                                         bool isNew)
  : QDialog{parent}
  , p_ptr{new PrefsLanguageShortcutDialogPrivate}
{
  auto &p = *p_ptr;

  p.ui->setupUi(this);

  auto title = isNew ? QY("Add new language shortcut") : QY("Edit language shortcut");

  setWindowTitle(title);
  p.ui->lTitle->setText(title);

  setupConnections();
  enableControls();

  Util::restoreWidgetGeometry(this);
}

PrefsLanguageShortcutDialog::~PrefsLanguageShortcutDialog() {
  Util::saveWidgetGeometry(this);
}

void
PrefsLanguageShortcutDialog::setupConnections() {
  auto &p = *p_func();

  connect(p.ui->buttons,     &QDialogButtonBox::accepted,                   this, &QDialog::accept);
  connect(p.ui->buttons,     &QDialogButtonBox::rejected,                   this, &QDialog::reject);
  connect(p.ui->ldwLanguage, &Util::LanguageDisplayWidget::languageChanged, this, &PrefsLanguageShortcutDialog::enableControls);
}

void
PrefsLanguageShortcutDialog::enableControls() {
  auto &p     = *p_func();
  auto enable = p.ui->ldwLanguage->language().is_valid();

  Util::buttonForRole(p.ui->buttons, QDialogButtonBox::AcceptRole)->setEnabled(enable);
}

void
PrefsLanguageShortcutDialog::setLanguage(mtx::bcp47::language_c const &language) {
  p_func()->ui->ldwLanguage->setLanguage(language);
  enableControls();
}

void
PrefsLanguageShortcutDialog::setTrackName(QString const &trackName) {
  auto &p = *p_func();
  p.ui->leTrackName->setText(trackName);
}

mtx::bcp47::language_c
PrefsLanguageShortcutDialog::language()
  const {
  return p_func()->ui->ldwLanguage->language();
}

QString
PrefsLanguageShortcutDialog::trackName()
  const {
  return p_func()->ui->leTrackName->text();
}

}
