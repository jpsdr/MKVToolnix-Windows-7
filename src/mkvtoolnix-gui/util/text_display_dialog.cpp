#include "common/common_pch.h"

#include <QClipboard>

#include "common/markdown.h"
#include "common/qt.h"
#include "mkvtoolnix-gui/forms/util/text_display_dialog.h"
#include "mkvtoolnix-gui/util/text_display_dialog.h"

namespace mtx { namespace gui { namespace Util {

class TextDisplayDialogPrivate {
  friend class TextDisplayDialog;

  std::unique_ptr<Ui::TextDisplayDialog> ui;
  QString m_text;

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

  connect(p->ui->pbClose,           &QPushButton::clicked, this, &TextDisplayDialog::accept);
  connect(p->ui->pbCopyToClipboard, &QPushButton::clicked, this, &TextDisplayDialog::copyToClipboard);
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

  p->m_text = text;

  return *this;
}

void
TextDisplayDialog::copyToClipboard() {
  QApplication::clipboard()->setText(p_func()->m_text);
}

}}}
