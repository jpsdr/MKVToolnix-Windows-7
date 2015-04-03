#include "common/common_pch.h"

#include "common/common.h"
#include "common/qt.h"
#include "common/version.h"
#include "mkvtoolnix-gui/app.h"
#include "mkvtoolnix-gui/util/settings.h"

namespace mtx { namespace gui {

App::App(int &argc,
         char **argv)
  : QApplication{argc, argv}
{
  mtx_common_init("mkvtoolnix-gui", argv[0]);

  QCoreApplication::setOrganizationName("bunkus.org");
  QCoreApplication::setOrganizationDomain("bunkus.org");
  QCoreApplication::setApplicationName("mkvtoolnix-gui");

#ifdef SYS_WINDOWS
  QApplication::setStyle(Q("windows"));
#endif

  Util::Settings::get().load();

  QObject::connect(this, SIGNAL(aboutToQuit()), this, SLOT(saveSettings()));

  retranslateUi();
}

App::~App() {
}

void
App::saveSettings()
  const {
  Util::Settings::get().save();
}

App *
App::instance() {
  return static_cast<App *>(QApplication::instance());
}

void
App::retranslateUi() {
  setApplicationDisplayName(Q(get_version_info("MKVToolNix GUI")));
}

}}
