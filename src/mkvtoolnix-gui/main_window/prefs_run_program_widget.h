#ifndef MTX_MKVTOOLNIX_GUI_MAIN_WINDOW_PREFS_RUN_PROGRAM_WIDGET_H
#define MTX_MKVTOOLNIX_GUI_MAIN_WINDOW_PREFS_RUN_PROGRAM_WIDGET_H

#include "common/common_pch.h"

#include <QWidget>

#include "mkvtoolnix-gui/util/settings.h"

namespace mtx { namespace gui {

class PrefsRunProgramWidgetPrivate;
class PrefsRunProgramWidget : public QWidget {
  Q_OBJECT;

protected:
  Q_DECLARE_PRIVATE(PrefsRunProgramWidget);

  QScopedPointer<PrefsRunProgramWidgetPrivate> const d_ptr;

  explicit PrefsRunProgramWidget(PrefsRunProgramWidgetPrivate &d, QWidget *parent, Util::Settings::RunProgramConfig const &cfg);

public:
  explicit PrefsRunProgramWidget(QWidget *parent, Util::Settings::RunProgramConfig const &cfg);
  ~PrefsRunProgramWidget();

  bool isValid() const;
  Util::Settings::RunProgramConfigPtr config() const;

signals:
  void nameOrExecutableChanged(QString const &name, QString const &newExecutable);

protected slots:
  void selectVariableToAdd();
  void changeExecutable();
  void commandLineEdited(QString const &commandLine);
  void nameEdited(QString const &name);
  void executeNow();
  void enableControls();

protected:
  void changeArguments(std::function<void(QStringList &)> const &worker);
  void addVariable(QString const &variable);

  void setupUi(Util::Settings::RunProgramConfig const &cfg);
  void setupToolTips();
  void setupMenu();
  void setupConnections();
};

}}

#endif // MTX_MKVTOOLNIX_GUI_MAIN_WINDOW_PREFS_RUN_PROGRAM_WIDGET_H
