#include "common/common_pch.h"

#include <QApplication>
#include <QDir>
#include <QProcess>

#include "common/fs_sys_helpers.h"
#include "common/version.h"
#include "mkvtoolnix-gui/app.h"
#include "mkvtoolnix-gui/jobs/job.h"
#include "mkvtoolnix-gui/main_window/update_checker.h"
#include "mkvtoolnix-gui/merge/source_file.h"
#include "mkvtoolnix-gui/util/installation_checker.h"

using namespace mtx::gui;

Q_DECLARE_METATYPE(std::shared_ptr<Merge::SourceFile>);
Q_DECLARE_METATYPE(QList<std::shared_ptr<Merge::SourceFile>>);

static void
registerMetaTypes() {
  qRegisterMetaType<Jobs::Job::LineType>("Job::LineType");
  qRegisterMetaType<Jobs::Job::Status>("Job::Status");
  qRegisterMetaType<QProcess::ExitStatus>("QProcess::ExitStatus");
  qRegisterMetaType<std::shared_ptr<Merge::SourceFile>>("std::shared_ptr<SourceFile>");
  qRegisterMetaType<QList<std::shared_ptr<Merge::SourceFile>>>("QList<std::shared_ptr<SourceFile>>");
  qRegisterMetaType<QFileInfoList>("QFileInfoList");
  qRegisterMetaType<mtx_release_version_t>("mtx_release_version_t");
  qRegisterMetaType<std::shared_ptr<pugi::xml_document>>("std::shared_ptr<pugi::xml_document>");
#if defined(HAVE_UPDATE_CHECK)
  qRegisterMetaType<UpdateCheckStatus>("UpdateCheckStatus");
#endif  // HAVE_UPDATE_CHECK
  qRegisterMetaType<Util::InstallationChecker::Problems>("Util::InstallationChecker::Problems");
}

int
main(int argc,
     char **argv) {
  registerMetaTypes();

  auto app = std::make_unique<App>(argc, argv);

  app->run();

  app.reset();

  mxexit();
}
