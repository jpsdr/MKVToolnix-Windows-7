#include "common/common_pch.h"

#include <QFile>

#include "common/markdown.h"
#include "common/qt.h"
#include "mkvtoolnix-gui/forms/main_window/code_of_conduct_dialog.h"
#include "mkvtoolnix-gui/main_window/code_of_conduct_dialog.h"

namespace mtx { namespace gui {

CodeOfConductDialog::CodeOfConductDialog(QWidget *parent)
  : QDialog{parent, Qt::Dialog | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint}
  , ui{new Ui::CodeOfConductDialog}
{
  // Setup UI controls.
  ui->setupUi(this);

  QFile coc{Q(":/CODE_OF_CONDUCT.md")};
  if (coc.open(QIODevice::ReadOnly))
    ui->codeOfConduct->setText(Q(mtx::markdown::to_html(coc.readAll().toStdString())));

  connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &CodeOfConductDialog::accept);
}

CodeOfConductDialog::~CodeOfConductDialog() {
}

}}
