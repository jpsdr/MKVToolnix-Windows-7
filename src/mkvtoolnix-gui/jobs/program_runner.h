#ifndef MTX_MKVTOOLNIX_GUI_JOBS_PROGRAM_RUNNER_H
#define MTX_MKVTOOLNIX_GUI_JOBS_PROGRAM_RUNNER_H

#include "common/common_pch.h"

#include "mkvtoolnix-gui/util/settings.h"

namespace mtx { namespace gui {

namespace Jobs {

class ProgramRunner {
public:
  using VariableMap = QMap<QString, QStringList>;

  static void run(Util::Settings::RunProgramForEvent forEvent, std::function<void(VariableMap &)> const &setupVariables, Util::Settings::RunProgramConfigPtr const &forceRunThis = Util::Settings::RunProgramConfigPtr{});

protected:
  static void setupGeneralVariables(VariableMap &variables);
  static QStringList replaceVariables(QStringList const &commandLine, VariableMap const &variables);
};

}}}

#endif  // MTX_MKVTOOLNIX_GUI_JOBS_PROGRAM_RUNNER_H
