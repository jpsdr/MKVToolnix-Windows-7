#include "common/common_pch.h"

#if defined(SYS_APPLE)

#include "common/qt.h"
#include "mkvtoolnix-gui/jobs/program_runner/macos_program_runner.h"

namespace mtx { namespace gui { namespace Jobs {

MacOSProgramRunner::MacOSProgramRunner()
  : ProgramRunner{}
{
}

MacOSProgramRunner::~MacOSProgramRunner() {
}

QString
MacOSProgramRunner::defaultAudioFileName()
  const {
  return Q("<MTX_INSTALLATION_DIRECTORY>/sounds/finished-1.ogg");
}

}}}

#endif  // SYS_APPLE
