#include "common/common_pch.h"

#include <QDebug>
#include <QDir>
#include <QProcess>
#include <QRegularExpression>
#include <QTimer>

#include "common/fs_sys_helpers.h"
#include "common/list_utils.h"
#include "common/qt.h"
#include "mkvtoolnix-gui/app.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/jobs/model.h"
#include "mkvtoolnix-gui/jobs/program_runner.h"
#if defined(SYS_LINUX)
#include "mkvtoolnix-gui/jobs/program_runner/linux_program_runner.h"
#elif defined(SYS_APPLE)
#include "mkvtoolnix-gui/jobs/program_runner/macos_program_runner.h"
#elif defined(SYS_WINDOWS)
#include "mkvtoolnix-gui/jobs/program_runner/windows_program_runner.h"
#endif // SYS_WINDOWS
#include "mkvtoolnix-gui/jobs/tool.h"
#include "mkvtoolnix-gui/util/media_player.h"
#include "mkvtoolnix-gui/util/message_box.h"
#include "mkvtoolnix-gui/util/string.h"

namespace mtx::gui::Jobs {

ProgramRunner::ProgramRunner()
  : QObject{}
{
}

ProgramRunner::~ProgramRunner() {
}

void
ProgramRunner::enableActionToExecute(Util::Settings::RunProgramConfig &config,
                                     ExecuteActionCondition condition,
                                     bool enable) {
  if (enable)
    m_actionsToExecute[condition] << &config;
  else
    m_actionsToExecute[condition].remove(&config);
}

bool
ProgramRunner::isActionToExecuteEnabled(Util::Settings::RunProgramConfig &config,
                                        ExecuteActionCondition condition) {
  return m_actionsToExecute[condition].contains(&config);
}

void
ProgramRunner::setup() {
  connect(MainWindow::jobTool()->model(), &Jobs::Model::queueStatusChanged, this, &ProgramRunner::executeActionsAfterQueueFinishes);
}

void
ProgramRunner::executeActionsAfterJobFinishes(Job const &job) {
  executeActions(ExecuteActionCondition::AfterJobFinishes, &job);
}

void
ProgramRunner::executeActionsAfterQueueFinishes(QueueStatus status) {
  if (Jobs::QueueStatus::Stopped == status)
    executeActions(ExecuteActionCondition::AfterQueueFinishes);
}

void
ProgramRunner::executeActions(ExecuteActionCondition condition,
                              Job const *job) {
  for (auto const &config : Util::Settings::get().m_runProgramConfigurations)
    if (isActionToExecuteEnabled(*config, condition)) {
      // The event doesn't really matter as we're forcing a specific configuration to run.
      run(Util::Settings::RunAfterJobQueueFinishes, [job](VariableMap &variables) {
        if (job)
          job->runProgramSetupVariables(variables);
      }, config);
    }

  m_actionsToExecute[condition].clear();
}

void
ProgramRunner::run(Util::Settings::RunProgramForEvent forEvent,
                   std::function<void(VariableMap &)> const &setupVariables,
                   Util::Settings::RunProgramConfigPtr const &forceRunThis) {
  auto &cfg             = Util::Settings::get();
  auto generalVariables = VariableMap{};
  auto configsToRun     = forceRunThis ? Util::Settings::RunProgramConfigList{forceRunThis}
                        :                cfg.m_runProgramConfigurations;

  setupGeneralVariables(generalVariables);

  for (auto const &runConfig : configsToRun) {
    if (!(runConfig->m_active && (runConfig->m_forEvents & forEvent)) && !forceRunThis)
      continue;

    auto variables = generalVariables;
    setupVariables(variables);

    if (runConfig->m_type == Util::Settings::RunProgramType::ExecuteProgram)
      executeProgram(*runConfig, variables);

    else if (runConfig->m_type == Util::Settings::RunProgramType::DeleteSourceFiles)
      deleteSourceFiles(variables);

    else if (runConfig->m_type == Util::Settings::RunProgramType::PlayAudioFile)
      playAudioFile(*runConfig);

    else if (runConfig->m_type == Util::Settings::RunProgramType::ShutDownComputer)
      shutDownComputer(*runConfig);

    else if (runConfig->m_type == Util::Settings::RunProgramType::HibernateComputer)
      hibernateComputer(*runConfig);

    else if (runConfig->m_type == Util::Settings::RunProgramType::SleepComputer)
      sleepComputer(*runConfig);

    else if (runConfig->m_type == Util::Settings::RunProgramType::ShowDesktopNotification)
      showDesktopNotification(forEvent, variables);

    else if (runConfig->m_type == Util::Settings::RunProgramType::QuitMKVToolNix)
      QTimer::singleShot(0, MainWindow::get(), []() { MainWindow::get()->forceClose(); });
  }
}

QStringList
ProgramRunner::replaceVariables(QStringList const &commandLine,
                                VariableMap const &variables) {
  auto variableNameRE = QRegularExpression{Q("<MTX_[A-Z0-9_]+>")};
  auto newCommandLine = QStringList{};

  for (auto const &argument: commandLine) {
    auto replacedFully = false;
    auto newArgument   = argument;

    for (auto const &variable : variables.keys()) {
      auto placeholder = Q("<MTX_%1>").arg(variable);

      if (argument == placeholder) {
        newCommandLine += variables[variable];
        replacedFully = true;
        break;

      } else
        newArgument.replace(placeholder, variables[variable].join(Q(' ')));
    }

    if (replacedFully)
      continue;

    newArgument.replace(variableNameRE, Q(""));
    newCommandLine << newArgument;
  }

  return newCommandLine;
}

void
ProgramRunner::setupGeneralVariables(QMap<QString, QStringList> &variables) {
  variables[Q("CURRENT_TIME")] << QDateTime::currentDateTime().toString(Qt::ISODate);
  variables[Q("INSTALLATION_DIRECTORY")] << QDir::toNativeSeparators(App::applicationDirPath());
}

std::unique_ptr<ProgramRunner>
ProgramRunner::create() {
  std::unique_ptr<ProgramRunner> runner;

#if defined(SYS_LINUX)
  runner.reset(new LinuxProgramRunner{});
#elif defined(SYS_APPLE)
  runner.reset(new MacOSProgramRunner{});
#elif defined(SYS_WINDOWS)
  runner.reset(new WindowsProgramRunner{});
#endif // SYS_WINDOWS

  if (!runner)
    runner.reset(new ProgramRunner{});

  return runner;
}

bool
ProgramRunner::isRunProgramTypeSupported(Util::Settings::RunProgramType type) {
  return mtx::included_in(type, Util::Settings::RunProgramType::ExecuteProgram, Util::Settings::RunProgramType::PlayAudioFile, Util::Settings::RunProgramType::DeleteSourceFiles, Util::Settings::RunProgramType::QuitMKVToolNix);
}

bool
ProgramRunner::isJobType(VariableMap const &variables,
                         QString const &type) {
  return variables.contains(Q("JOB_TYPE")) && (variables[Q("JOB_TYPE")][0] == type);
}

void
ProgramRunner::executeProgram(Util::Settings::RunProgramConfig &config,
                              VariableMap const &variables) {
  auto commandLine = replaceVariables(config.m_commandLine, variables);
  auto exe         = commandLine.value(0);

  if (exe.isEmpty())
    return;

  commandLine.removeFirst();

  if (QProcess::startDetached(exe, commandLine))
    return;

  Util::MessageBox::critical(MainWindow::get())
    ->title(QY("Program execution failed"))
    .text(Q("%1\n%2")
          .arg(QY("The following program could not be executed: %1").arg(exe))
          .arg(QY("Possible causes are that the program does not exist or that you're not allowed to access it or its directory.")))
    .exec();
}

void
ProgramRunner::deleteSourceFiles(VariableMap const &variables) {
  if (!isJobType(variables, Q("multiplexer")))
    return;

  QStringList toRemove;
  auto vobsubRE = QRegularExpression{Q("(idx|sub)$"), QRegularExpression::PatternOption::CaseInsensitiveOption};

  for (auto const &sourceFileName : variables[Q("SOURCE_FILE_NAMES")]) {
    toRemove << sourceFileName;

    if (sourceFileName.contains(vobsubRE)) {
      auto other = sourceFileName.last(3).toLower() == Q("idx") ? Q("sub") : Q("idx");
      toRemove << sourceFileName.first(sourceFileName.length() - 3) + other;
    }
  }

  for (auto const &sourceFileName : toRemove) {
    auto succeeded = QFile::remove(sourceFileName);
    qDebug() << Q("deleteSourceFiles: file removal %1 (%2)").arg(succeeded ? "succeeded" : "failed").arg(sourceFileName);
  }
}

void
ProgramRunner::playAudioFile(Util::Settings::RunProgramConfig &config) {
  App::mediaPlayer().playFile(Util::replaceMtxVariableWithApplicationDirectory(config.m_audioFile), config.m_volume);
}

QString
ProgramRunner::defaultAudioFileName()
  const {
  return Q("%1/sounds/finished-1.webm").arg(Q(mtx::sys::get_package_data_folder()));
}

void
ProgramRunner::showDesktopNotification(Util::Settings::RunProgramForEvent const /* forEvent */,
                                       VariableMap const &/* variables */) {
  // Not supported in an OS-agnostic way.
}

void
ProgramRunner::shutDownComputer(Util::Settings::RunProgramConfig &/* config */) {
  // Not supported in an OS-agnostic way.
}

void
ProgramRunner::hibernateComputer(Util::Settings::RunProgramConfig &/* config */) {
  // Not supported in an OS-agnostic way.
}

void
ProgramRunner::sleepComputer(Util::Settings::RunProgramConfig &/* config */) {
  // Not supported in an OS-agnostic way.
}

}
