#ifndef MTX_MKVTOOLNIX_GUI_JOBS_PROGRAM_RUNNER_H
#define MTX_MKVTOOLNIX_GUI_JOBS_PROGRAM_RUNNER_H

#include "common/common_pch.h"

#include "mkvtoolnix-gui/util/settings.h"

namespace mtx { namespace gui {

namespace Jobs {

class Job;
enum class QueueStatus;

class ProgramRunner: public QObject {
  Q_OBJECT;

public:
  using VariableMap = QMap<QString, QStringList>;

  enum class ExecuteActionCondition {
    AfterJobFinishes,
    AfterQueueFinishes,
  };

protected:
  QMap<ExecuteActionCondition, QSet<Util::Settings::RunProgramConfig *>> m_actionsToExecute;

public:
  explicit ProgramRunner();
  virtual ~ProgramRunner();

  void setup();

  void enableActionToExecute(Util::Settings::RunProgramConfig *config, ExecuteActionCondition condition, bool enable);
  bool isActionToExecuteEnabled(Util::Settings::RunProgramConfig *config, ExecuteActionCondition condition);

public slots:
  void executeActionsAfterJobFinishes(Job const &job);
  void executeActionsAfterQueueFinishes(Jobs::QueueStatus status);

protected:
  void executeActions(ExecuteActionCondition condition, Job const *job = nullptr);

public:
  static void run(Util::Settings::RunProgramForEvent forEvent, std::function<void(VariableMap &)> const &setupVariables, Util::Settings::RunProgramConfigPtr const &forceRunThis = Util::Settings::RunProgramConfigPtr{});

protected:
  static void setupGeneralVariables(VariableMap &variables);
  static QStringList replaceVariables(QStringList const &commandLine, VariableMap const &variables);
};

}}}

#endif  // MTX_MKVTOOLNIX_GUI_JOBS_PROGRAM_RUNNER_H
