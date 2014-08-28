#include "common/common_pch.h"

#include "common/qt.h"
#include "mkvtoolnix-gui/forms/command_line_dialog.h"
#include "mkvtoolnix-gui/main_window/command_line_dialog.h"
#include "mkvtoolnix-gui/util/util.h"

CommandLineDialog::CommandLineDialog(QWidget *parent,
                                     QStringList const &options,
                                     QString const &title)
  : QDialog{parent}
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
}

CommandLineDialog::~CommandLineDialog() {
}

void
CommandLineDialog::onEscapeModeChanged(int index) {
  auto mode = 0 == index ? Util::EscapeShellWindows
            : 1 == index ? Util::EscapeShellUnix
            : 2 == index ? Util::EscapeMkvtoolnix
            :              Util::DontEscape;

  auto sep  = Util::EscapeMkvtoolnix == mode ? "\n" : " ";

  ui->commandLine->setPlainText(Util::escape(m_options, mode).join(Q(sep)));
}
