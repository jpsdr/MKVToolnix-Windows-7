#include "common/common_pch.h"

#include <QClipboard>
#include <QEvent>
#include <QKeyEvent>
#include <QKeySequence>

#include "common/markdown.h"
#include "common/qt.h"
#include "mkvtoolnix-gui/forms/util/text_display_dialog.h"
#include "mkvtoolnix-gui/util/file.h"
#include "mkvtoolnix-gui/util/file_dialog.h"
#include "mkvtoolnix-gui/util/settings.h"
#include "mkvtoolnix-gui/util/text_display_dialog.h"

namespace mtx::gui::Util {

class TextDisplayDialogPrivate {
  friend class TextDisplayDialog;

  std::unique_ptr<Ui::TextDisplayDialog> ui;
  QString m_text, m_saveDefaultFileName, m_saveFilter, m_saveDefaultSuffix;
  TextDisplayDialog::Format m_format{TextDisplayDialog::Format::Plain};

  TextDisplayDialogPrivate()
    : ui{new Ui::TextDisplayDialog}
  {
  }
};

TextDisplayDialog::TextDisplayDialog(QWidget *parent)
  : QDialog{parent, Qt::Dialog | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint}
  , p_ptr{new TextDisplayDialogPrivate}
{
  auto p = p_func();

  // Setup UI controls.
  p->ui->setupUi(this);

  p->ui->text->installEventFilter(this);
  p->ui->pbSave->hide();

  connect(p->ui->pbClose,           &QPushButton::clicked, this, &TextDisplayDialog::accept);
  connect(p->ui->pbCopyToClipboard, &QPushButton::clicked, this, &TextDisplayDialog::copyToClipboard);
  connect(p->ui->pbSave,            &QPushButton::clicked, this, &TextDisplayDialog::save);
}

TextDisplayDialog::~TextDisplayDialog() {
}

TextDisplayDialog &
TextDisplayDialog::setTitle(QString const &title) {
  setWindowTitle(title);
  return *this;
}

TextDisplayDialog &
TextDisplayDialog::setText(QString const &text,
                           Format format) {
  auto p = p_func();

  switch (format) {
    case Format::Plain:
      p->ui->text->setText(text.toHtmlEscaped());
      break;

    case Format::HTML:
      p->ui->text->setText(text);
      break;

    case Format::Markdown:
      p->ui->text->setText(Q(mtx::markdown::to_html(to_utf8(text))));
      break;
  }

  p->m_text   = text;
  p->m_format = format;

  return *this;
}

TextDisplayDialog &
TextDisplayDialog::setSaveInfo(QString const &defaultFileName,
                               QString const &filter,
                               QString const &defaultSuffix) {
  auto &p                 = *p_func();
  p.m_saveDefaultFileName = defaultFileName;
  p.m_saveFilter          = filter;
  p.m_saveDefaultSuffix   = defaultSuffix;

  p.ui->pbSave->show();

  return *this;
}

void
TextDisplayDialog::copyToClipboard() {
  QApplication::clipboard()->setText(p_func()->m_text);
}

void
TextDisplayDialog::save() {
  auto &p       = *p_func();
  auto &cfg     = Util::Settings::get();
  auto filter   = Q("%1 (*.%2);;%3 (*)").arg(p.m_saveFilter).arg(p.m_saveDefaultSuffix).arg(QY("All files"));
  auto fileName = Util::getSaveFileName(this, QY("Save"), cfg.lastOpenDirPath(), p.m_saveDefaultFileName, filter, p.m_saveDefaultSuffix);

  if (!fileName.isEmpty())
    Util::saveTextToFile(fileName, p.m_text, true);
}

bool
TextDisplayDialog::eventFilter(QObject *o,
                               QEvent *e) {
  auto p = p_func();

  if (   (p->m_format == Format::Markdown)
      && (o           == p->ui->text)
      && (e->type()   == QEvent::KeyPress)
      && (static_cast<QKeyEvent *>(e)->matches(QKeySequence::Copy))) {
    copyToClipboard();
    return true;
  }

  return QDialog::eventFilter(o, e);
}

}
