#include "common/common_pch.h"

#if defined(SYS_APPLE)

#include "common/qt.h"
#include "mkvtoolnix-gui/jobs/program_runner/macos_program_runner.h"

namespace mtx::gui::Jobs {

MacOSProgramRunner::MacOSProgramRunner()
  : ProgramRunner{}
{
}

MacOSProgramRunner::~MacOSProgramRunner() {
}

QString
MacOSProgramRunner::defaultAudioFileName()
  const {
  return Q("/System/Library/Sounds/Glass.aiff");
}

}

#endif  // SYS_APPLE
