#include "common/common_pch.h"

#include <QRegularExpression>
#include <QThread>

#include "common/qt.h"
#include "common/version.h"
#include "mkvtoolnix-gui/app.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/util/installation_checker.h"
#include "mkvtoolnix-gui/util/process.h"
#include "mkvtoolnix-gui/util/settings.h"

namespace mtx { namespace gui { namespace Util {

InstallationChecker::InstallationChecker(QObject *parent)
  : QObject{parent}
{
}

InstallationChecker::~InstallationChecker() {
}

void
InstallationChecker::runChecks() {
  auto problems    = Problems{};
  auto mkvmergeExe = Util::Settings::get().actualMkvmergeExe();
  auto versionRE   = QRegularExpression{Q("^mkvmerge [[:space:]]+ v ( [[:digit:].]+ )"), QRegularExpression::ExtendedPatternSyntaxOption};
  auto guiVersion  = Q(get_current_version().to_string());

  if (mkvmergeExe.isEmpty() || !QFileInfo{mkvmergeExe}.exists())
    problems << Problem{ ProblemType::MkvmergeNotFound, {} };

  else {
    auto process = Process::execute(mkvmergeExe, { Q("--version") });

    if (process->hasError())
      problems << Problem{ ProblemType::MkvmergeCannotBeExecuted, {} };

    else {
      // mkvmerge v9.7.1 ('Pandemonium') 64bit
      auto output = process->output().join(QString{});
      auto match  = versionRE.match(output);

      if (!match.hasMatch())
        problems << Problem{ ProblemType::MkvmergeVersionNotRecognized, output };

      else if (guiVersion != match.captured(1))
        problems << Problem{ ProblemType::MkvmergeVersionDiffers, match.captured(1) };
    }
  }

#if defined(SYS_WINDOWS)
  auto magicFile = App::applicationDirPath() + "/data/magic.mgc";
  if (!QFileInfo{magicFile}.exists())
    problems << Problem{ ProblemType::FileNotFound, Q("data\\magic.mgc") };

#endif  // SYS_WINDOWS

  if (!problems.isEmpty())
    emit problemsFound(problems);

  emit finished();
}

void
InstallationChecker::checkInstallation() {
  auto thread  = new QThread{};
  auto checker = new InstallationChecker{};

  checker->moveToThread(thread);

  connect(thread,  &QThread::started,                   checker,           &InstallationChecker::runChecks);
  connect(checker, &InstallationChecker::problemsFound, MainWindow::get(), &MainWindow::displayInstallationProblems);
  connect(checker, &InstallationChecker::finished,      checker,           &InstallationChecker::deleteLater);
  connect(thread,  &QThread::finished,                  thread,            &QThread::deleteLater);

  thread->start();
}


}}}
