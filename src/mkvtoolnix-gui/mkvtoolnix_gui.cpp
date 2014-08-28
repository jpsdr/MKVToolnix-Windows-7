#include "common/common_pch.h"

#include <QApplication>
#include <QProcess>

#include "mkvtoolnix-gui/app.h"
#include "mkvtoolnix-gui/job_widget/job.h"
#include "mkvtoolnix-gui/mkvtoolnix_gui.h"
#include "mkvtoolnix-gui/main_window/main_window.h"

static void
registerMetaTypes() {
  qRegisterMetaType<Job::Status>("Job::Status");
  qRegisterMetaType<QProcess::ExitStatus>("QProcess::ExitStatus");
}

int
main(int argc,
     char **argv) {
  registerMetaTypes();

  auto app        = std::unique_ptr<App>(new App{argc, argv});
  auto mainWindow = std::unique_ptr<MainWindow>(new MainWindow{});

  mainWindow->show();
  app->exec();

  mainWindow.reset();
  app.reset();

  mxexit(0);
}
