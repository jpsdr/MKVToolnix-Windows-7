#pragma once

#include "common/common_pch.h"

#include <QAction>
#include <QDialog>

#include "mkvtoolnix-gui/util/command_line_options.h"

namespace mtx::gui::Merge {

namespace Ui {
class CommandLineDialog;
}

class CommandLineDialog : public QDialog {
  Q_OBJECT

protected:
  // UI stuff:
  std::unique_ptr<Ui::CommandLineDialog> ui;
  Util::CommandLineOptions const m_options;

public:
  struct Mode {
    int index;
    QString title;
    Util::EscapeMode escapeMode;
  };

public:
  explicit CommandLineDialog(QWidget *parent, Util::CommandLineOptions const &options, QString const &title);
  ~CommandLineDialog();

public Q_SLOTS:
  void onEscapeModeChanged(int index);
  void copyToClipboard();

public:
  static QVector<Mode> supportedModes();
  static int platformDependentDefaultMode();
};

}
