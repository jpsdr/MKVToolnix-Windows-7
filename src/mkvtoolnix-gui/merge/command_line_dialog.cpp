#include "common/common_pch.h"

#include <QApplication>
#include <QClipboard>

#include "common/qt.h"
#include "mkvtoolnix-gui/forms/merge/command_line_dialog.h"
#include "mkvtoolnix-gui/merge/command_line_dialog.h"
#include "mkvtoolnix-gui/util/settings.h"
#include "mkvtoolnix-gui/util/string.h"
#include "mkvtoolnix-gui/util/widget.h"

namespace mtx::gui::Merge {

CommandLineDialog::CommandLineDialog(QWidget *parent,
                                     Util::CommandLineOptions const &options,
                                     QString const &title)
  : QDialog{parent, Qt::Dialog | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint}
  , ui{new Ui::CommandLineDialog}
  , m_options{options}
{
  // Setup UI controls.
  ui->setupUi(this);

  setWindowTitle(title);

  ui->escapeMode->clear();
  for (auto const &mode : supportedModes())
    ui->escapeMode->addItem(mode.title);

  // Set initial escaping mode according to platform's native mode.
  auto index = Util::Settings::get().m_mergeDefaultCommandLineEscapeMode;

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

QVector<CommandLineDialog::Mode>
CommandLineDialog::supportedModes() {
  QVector<Mode> modes;

  modes << Mode{0, QY("Windows (cmd.exe)"),                        Util::EscapeShellCmdExeArgument }
        << Mode{1, QY("Linux/Unix shells (bash, zsh etc.)"),       Util::EscapeShellUnix           }
        << Mode{2, QY("MKVToolNix option files (JSON-formatted)"), Util::EscapeJSON                }
        << Mode{3, QY("Don't escape"),                             Util::DontEscape                };

  return modes;
}

int
CommandLineDialog::platformDependentDefaultMode() {
#if defined(SYS_WINDOWS)
  return 0;
#else
  return 1;
#endif
}

void
CommandLineDialog::onEscapeModeChanged(int index) {
  auto mode = 0 == index ? Util::EscapeShellCmdExeArgument
            : 1 == index ? Util::EscapeShellUnix
            : 2 == index ? Util::EscapeJSON
            :              Util::DontEscape;

  ui->commandLine->setPlainText(m_options.formatted(mode));
}

void
CommandLineDialog::copyToClipboard() {
  QApplication::clipboard()->setText(ui->commandLine->toPlainText());
}

}
