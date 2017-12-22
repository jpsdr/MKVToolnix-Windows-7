#pragma once

#include "common/common_pch.h"

#include <QWidget>

#include "common/qt.h"
#include "mkvtoolnix-gui/util/settings.h"

namespace mtx { namespace gui {

class PrefsRunProgramWidgetPrivate;
class PrefsRunProgramWidget : public QWidget {
  Q_OBJECT;

protected:
  MTX_DECLARE_PRIVATE(PrefsRunProgramWidget);

  std::unique_ptr<PrefsRunProgramWidgetPrivate> const p_ptr;

  explicit PrefsRunProgramWidget(PrefsRunProgramWidgetPrivate &p, QWidget *parent, Util::Settings::RunProgramConfig const &cfg);

public:
  explicit PrefsRunProgramWidget(QWidget *parent, Util::Settings::RunProgramConfig const &cfg);
  ~PrefsRunProgramWidget();

  bool isValid() const;
  Util::Settings::RunProgramConfigPtr config() const;
  QString validate() const;

signals:
  void titleChanged();

protected slots:
  void typeChanged(int index);

  void selectVariableToAdd();
  void changeExecutable();
  void commandLineEdited(QString const &commandLine);
  void nameEdited();
  void executeNow();
  void enableControls();

  void changeAudioFile();
  void audioFileEdited();

protected:
  void changeArguments(std::function<void(QStringList &)> const &worker);
  void addVariable(QString const &variable);

  void setupUi(Util::Settings::RunProgramConfig const &cfg);
  void setupTypeControl(Util::Settings::RunProgramConfig const &cfg);
  void setupToolTips();
  void setupMenu();
  void setupConnections();

  void showPageForType(Util::Settings::RunProgramType type);
};

}}
