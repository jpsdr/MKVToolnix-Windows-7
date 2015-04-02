#include "common/common_pch.h"

#include <QApplication>
#include <QProcess>

#include "common/fs_sys_helpers.h"
#if defined(HAVE_CURL_EASY_H)
# include "common/version.h"
#endif  // HAVE_CURL_EASY_H
#include "mkvtoolnix-gui/app.h"
#include "mkvtoolnix-gui/jobs/job.h"
#include "mkvtoolnix-gui/mkvtoolnix_gui.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/main_window/preview_warning_dialog.h"
#include "mkvtoolnix-gui/main_window/update_check_thread.h"

static void
registerMetaTypes() {
  qRegisterMetaType<mtx::gui::Jobs::Job::LineType>("Job::LineType");
  qRegisterMetaType<mtx::gui::Jobs::Job::Status>("Job::Status");
  qRegisterMetaType<QProcess::ExitStatus>("QProcess::ExitStatus");
#if defined(HAVE_CURL_EASY_H)
  qRegisterMetaType<mtx_release_version_t>("mtx_release_version_t");
  qRegisterMetaType<std::shared_ptr<pugi::xml_document>>("std::shared_ptr<pugi::xml_document>");
  qRegisterMetaType<UpdateCheckStatus>("UpdateCheckStatus");
#endif  // HAVE_CURL_EASY_H
}

static void
showPreviewWarning(QWidget *parent) {
  if (get_environment_variable("NO_WARNING") == "1")
    return;

  auto dlg = std::make_unique<PreviewWarningDialog>(parent);
  dlg->exec();
}

int
main(int argc,
     char **argv) {
  registerMetaTypes();

  auto app        = std::make_unique<App>(argc, argv);
  auto mainWindow = std::make_unique<MainWindow>();

  showPreviewWarning(mainWindow.get());

  mainWindow->show();
  app->exec();

  mainWindow.reset();
  app.reset();

  mxexit();
}
