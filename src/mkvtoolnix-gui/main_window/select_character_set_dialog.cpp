#include "common/common_pch.h"

#include <QVariant>

#include "common/locale.h"
#include "common/qt.h"
#include "mkvtoolnix-gui/forms/main_window/select_character_set_dialog.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/main_window/select_character_set_dialog.h"
#include "mkvtoolnix-gui/util/widget.h"

namespace mtx { namespace gui {

class SelectCharacterSetDialogPrivate {
  friend class SelectCharacterSetDialog;

  std::unique_ptr<Ui::SelectCharacterSetDialog> m_ui{new Ui::SelectCharacterSetDialog};
  QByteArray m_content;
  QVariant m_userData;
};

SelectCharacterSetDialog::SelectCharacterSetDialog(QWidget *parent,
                                                   QString const &fileName,
                                                   QString const &initialCharacterSet,
                                                   QStringList const &additionalCharacterSets)
  : QDialog{parent, Qt::Dialog | Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint}
  , d_ptr{new SelectCharacterSetDialogPrivate}
{
  Q_D(SelectCharacterSetDialog);

  auto characterSetToSelect = initialCharacterSet.isEmpty() ? Q(g_cc_local_utf8->get_charset()) : initialCharacterSet;

  QFile file{fileName};
  if (file.open(QIODevice::ReadOnly))
    d->m_content = file.readAll();

  d->m_ui->setupUi(this);

  d->m_ui->fileName->setText(fileName);

  auto characterSets = additionalCharacterSets;
  characterSets << characterSetToSelect;

  d->m_ui->characterSet->setAdditionalItems(characterSets).setup().setCurrentByData(characterSetToSelect);

  d->m_ui->content->setPlaceholderText(QY("File not found"));

  connect(MainWindow::get(),     &MainWindow::preferencesChanged,                                        this, &SelectCharacterSetDialog::retranslateUi);
  connect(d->m_ui->characterSet, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &SelectCharacterSetDialog::updatePreview);
  connect(d->m_ui->buttonBox,    &QDialogButtonBox::accepted,                                            this, &SelectCharacterSetDialog::accept);
  connect(d->m_ui->buttonBox,    &QDialogButtonBox::rejected,                                            this, &SelectCharacterSetDialog::reject);
  connect(this,                  &SelectCharacterSetDialog::accepted,                                    this, &SelectCharacterSetDialog::emitResult);
  connect(this,                  &SelectCharacterSetDialog::finished,                                    this, &SelectCharacterSetDialog::deleteLater);

  Util::restoreWidgetGeometry(this);

  updatePreview();
}

SelectCharacterSetDialog::~SelectCharacterSetDialog() {
  Util::saveWidgetGeometry(this);
}

void
SelectCharacterSetDialog::retranslateUi() {
  Q_D(SelectCharacterSetDialog);

  d->m_ui->retranslateUi(this);
}

void
SelectCharacterSetDialog::updatePreview() {
  Q_D(SelectCharacterSetDialog);

  if (d->m_content.isEmpty())
    return;

  auto converter = charset_converter_c::init(to_utf8(selectedCharacterSet()));
  if (converter)
    d->m_ui->content->setPlainText(Q(converter->utf8(d->m_content.data())));
}

void
SelectCharacterSetDialog::emitResult() {
  emit characterSetSelected(selectedCharacterSet());
}

QString
SelectCharacterSetDialog::selectedCharacterSet()
  const {
  Q_D(const SelectCharacterSetDialog);

  return d->m_ui->characterSet->currentData().toString();
}

void
SelectCharacterSetDialog::setUserData(QVariant const &data) {
  Q_D(SelectCharacterSetDialog);

  d->m_userData = data;
}

QVariant const &
SelectCharacterSetDialog::userData()
  const {
  Q_D(const SelectCharacterSetDialog);

  return d->m_userData;
}

}}
