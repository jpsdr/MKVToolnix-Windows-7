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

public:
  struct MuxSettings {
    QString title;
    Util::CommandLineOptions options;
  };

  struct Mode {
    int index;
    QString title;
    Util::EscapeMode escapeMode;
  };

protected:
  // UI stuff:
  std::unique_ptr<Ui::CommandLineDialog> ui;
  QVector<MuxSettings> m_muxSettings;

public:
  explicit CommandLineDialog(QWidget *parent, QVector<MuxSettings> const &muxSettings, int initialMuxSettings, QString const &title);
  ~CommandLineDialog();

public Q_SLOTS:
  void onControlsChanged();
  void copyToClipboard();

public:
  static QVector<Mode> supportedModes();
  static int platformDependentDefaultMode();
};

}
