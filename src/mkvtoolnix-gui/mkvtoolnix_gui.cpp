#include "common/common_pch.h"

#include <QApplication>
#include <QDir>
#include <QProcess>

#include "common/fs_sys_helpers.h"
#if defined(HAVE_CURL_EASY_H)
# include "common/version.h"
#endif  // HAVE_CURL_EASY_H
#include "mkvtoolnix-gui/app.h"
#include "mkvtoolnix-gui/jobs/job.h"
#include "mkvtoolnix-gui/main_window/update_check_thread.h"
#include "mkvtoolnix-gui/merge/source_file.h"

#if defined(HAVE_STATIC_QT)
# if defined(SYS_APPLE)
#  include <QtPlugin>
Q_IMPORT_PLUGIN(QCocoaIntegrationPlugin);

# elif defined(SYS_WINDOWS)
#  include <QtPlugin>
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin);

# endif  // SYS_APPLE, SYS_WINDOWS
#endif   // HAVE_STATIC_QT
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
#if defined(HAVE_CURL_EASY_H)
  qRegisterMetaType<mtx_release_version_t>("mtx_release_version_t");
  qRegisterMetaType<std::shared_ptr<pugi::xml_document>>("std::shared_ptr<pugi::xml_document>");
  qRegisterMetaType<UpdateCheckStatus>("UpdateCheckStatus");
#endif  // HAVE_CURL_EASY_H
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
