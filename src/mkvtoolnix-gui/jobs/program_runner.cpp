#include "common/common_pch.h"

#include <QProcess>
#include <QRegularExpression>

#include "common/qt.h"
#include "mkvtoolnix-gui/jobs/program_runner.h"

namespace mtx { namespace gui { namespace Jobs {

void
ProgramRunner::run(Util::Settings::RunProgramForEvent forEvent,
                   std::function<void(VariableMap &)> const &setupVariables) {
  auto &cfg             = Util::Settings::get();
  auto generalVariables = VariableMap{};

  setupGeneralVariables(generalVariables);

  for (auto const &runConfig : cfg.m_runProgramConfigurations) {
    if (!(runConfig->m_forEvents & forEvent))
      continue;

    auto commandLine = runConfig->m_commandLine;
    auto variables   = generalVariables;

    setupVariables(variables);

    commandLine = replaceVariables(commandLine, variables);
    auto exe    = commandLine.value(0);

    if (exe.isEmpty())
      continue;

    commandLine.removeFirst();

    QProcess::startDetached(exe, commandLine);
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
}

}}}
