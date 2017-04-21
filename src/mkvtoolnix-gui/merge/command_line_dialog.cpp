#include "common/common_pch.h"

#include <QApplication>
#include <QClipboard>

#include "common/list_utils.h"
#include "common/qt.h"
#include "mkvtoolnix-gui/forms/merge/command_line_dialog.h"
#include "mkvtoolnix-gui/merge/command_line_dialog.h"
#include "mkvtoolnix-gui/util/string.h"
#include "mkvtoolnix-gui/util/widget.h"

namespace mtx { namespace gui { namespace Merge {

CommandLineDialog::CommandLineDialog(QWidget *parent,
                                     QStringList const &options,
                                     QString const &title)
  : QDialog{parent, Qt::Dialog | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint}
  , ui{new Ui::CommandLineDialog}
  , m_options{options}
{
  // Setup UI controls.
  ui->setupUi(this);

  setWindowTitle(title);

  // Set initial escaping mode according to platform's native mode.
#if defined(SYS_WINDOWS)
  int index = 0;
#else
  int index = 1;
#endif

  ui->escapeMode->setCurrentIndex(index);
  onEscapeModeChanged(index);

  ui->commandLine->setFocus();

  Util::restoreWidgetGeometry(this);

  connect(ui->escapeMode,        static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &CommandLineDialog::onEscapeModeChanged);
  connect(ui->pbClose,           &QPushButton::clicked,                                                  this, &CommandLineDialog::accept);
  connect(ui->pbCopyToClipboard, &QPushButton::clicked,                                                  this, &CommandLineDialog::copyToClipboard);
}

CommandLineDialog::~CommandLineDialog() {
  Util::saveWidgetGeometry(this);
}

void
CommandLineDialog::onEscapeModeChanged(int index) {
  auto mode = 0 == index ? Util::EscapeShellCmdExeArgument
            : 1 == index ? Util::EscapeShellUnix
            : 2 == index ? Util::EscapeJSON
            : 3 == index ? Util::EscapeMkvtoolnix
            :              Util::DontEscape;

  auto sep  = Util::EscapeMkvtoolnix == mode ? "\n" : " ";
  auto opts = m_options;

  if (mtx::included_in(mode, Util::EscapeJSON, Util::EscapeMkvtoolnix))
    opts.removeFirst();

  ui->commandLine->setPlainText(Util::escape(opts, mode).join(Q(sep)));
}

void
CommandLineDialog::copyToClipboard() {
  QApplication::clipboard()->setText(ui->commandLine->toPlainText());
}

}}}
