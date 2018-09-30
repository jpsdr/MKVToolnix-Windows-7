#include "common/common_pch.h"

#include "common/markdown.h"
#include "common/qt.h"
#include "mkvtoolnix-gui/forms/util/text_display_dialog.h"
#include "mkvtoolnix-gui/util/text_display_dialog.h"

namespace mtx { namespace gui { namespace Util {

TextDisplayDialog::TextDisplayDialog(QWidget *parent)
  : QDialog{parent, Qt::Dialog | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint}
  , ui{new Ui::TextDisplayDialog}
{
  // Setup UI controls.
  ui->setupUi(this);

  connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &TextDisplayDialog::accept);
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
  switch (format) {
    case Format::Plain:
      ui->text->setText(text.toHtmlEscaped());
      break;

    case Format::HTML:
      ui->text->setText(text);
      break;

    case Format::Markdown:
      ui->text->setText(Q(mtx::markdown::to_html(to_utf8(text))));
      break;
  }

  return *this;
}

}}}
