#include "common/common_pch.h"

#include <QApplication>
#include <QProcess>

#include "common/fs_sys_helpers.h"
#include "mkvtoolnix-gui/app.h"
#include "mkvtoolnix-gui/job_widget/job.h"
#include "mkvtoolnix-gui/mkvtoolnix_gui.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/main_window/preview_warning_dialog.h"

static void
registerMetaTypes() {
  qRegisterMetaType<Job::LineType>("Job::LineType");
  qRegisterMetaType<Job::Status>("Job::Status");
  qRegisterMetaType<QProcess::ExitStatus>("QProcess::ExitStatus");
}

static void
showPreviewWarning(QWidget *parent) {
  if (get_environment_variable("NO_WARNING") == "1")
    return;

  auto dlg = std::unique_ptr<PreviewWarningDialog>(new PreviewWarningDialog{parent});
  dlg->exec();
}

int
main(int argc,
     char **argv) {
  registerMetaTypes();

  auto app        = std::unique_ptr<App>(new App{argc, argv});
  auto mainWindow = std::unique_ptr<MainWindow>(new MainWindow{});

  showPreviewWarning(mainWindow.get());

  mainWindow->show();
  app->exec();

  mainWindow.reset();
  app.reset();

  mxexit(0);
}
