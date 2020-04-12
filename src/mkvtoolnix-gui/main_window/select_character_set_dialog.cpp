#include "common/common_pch.h"

#include <QScrollBar>
#include <QVariant>

#include "common/locale.h"
#include "common/qt.h"
#include "mkvtoolnix-gui/forms/main_window/select_character_set_dialog.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/main_window/select_character_set_dialog.h"
#include "mkvtoolnix-gui/util/widget.h"

namespace mtx::gui {

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
  , p_ptr{new SelectCharacterSetDialogPrivate}
{
  auto p                    = p_func();
  auto characterSetToSelect = initialCharacterSet.isEmpty() ? Q(g_cc_local_utf8->get_charset()) : initialCharacterSet;

  QFile file{fileName};
  if (file.open(QIODevice::ReadOnly))
    p->m_content = file.readAll();

  p->m_ui->setupUi(this);

  p->m_ui->fileName->setText(fileName);

  auto characterSets = additionalCharacterSets;
  characterSets << characterSetToSelect;

  p->m_ui->characterSet->setAdditionalItems(characterSets).setup().setCurrentByData(characterSetToSelect);

  p->m_ui->content->setPlaceholderText(QY("File not found"));

  connect(MainWindow::get(),     &MainWindow::preferencesChanged,                                        this, &SelectCharacterSetDialog::retranslateUi);
  connect(p->m_ui->characterSet, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &SelectCharacterSetDialog::updatePreview);
  connect(p->m_ui->buttonBox,    &QDialogButtonBox::accepted,                                            this, &SelectCharacterSetDialog::accept);
  connect(p->m_ui->buttonBox,    &QDialogButtonBox::rejected,                                            this, &SelectCharacterSetDialog::reject);
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
  auto p = p_func();

  p->m_ui->retranslateUi(this);
}

void
SelectCharacterSetDialog::updatePreview() {
  auto p = p_func();

  if (p->m_content.isEmpty())
    return;

  auto converter = charset_converter_c::init(to_utf8(selectedCharacterSet()));
  if (!converter)
    return;

  auto horizontalScrollBar = p->m_ui->content->horizontalScrollBar();
  auto verticalScrollBar   = p->m_ui->content->verticalScrollBar();
  auto horizontalPosition  = horizontalScrollBar->value();
  auto verticalPosition    = verticalScrollBar->value();

  p->m_ui->content->setPlainText(Q(converter->utf8(p->m_content.data())));

  horizontalScrollBar->setValue(std::min(horizontalPosition, horizontalScrollBar->maximum()));
  verticalScrollBar  ->setValue(std::min(verticalPosition,   verticalScrollBar->maximum()));
}

void
SelectCharacterSetDialog::emitResult() {
  Q_EMIT characterSetSelected(selectedCharacterSet());
}

QString
SelectCharacterSetDialog::selectedCharacterSet()
  const {
  return p_func()->m_ui->characterSet->currentData().toString();
}

void
SelectCharacterSetDialog::setUserData(QVariant const &data) {
  p_func()->m_userData = data;
}

QVariant const &
SelectCharacterSetDialog::userData()
  const {
  return p_func()->m_userData;
}

}
