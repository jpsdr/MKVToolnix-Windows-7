#include "common/common_pch.h"

#include <QRegularExpression>
#include <QTemporaryFile>
#include <QThread>

#include "common/qt.h"
#include "common/version.h"
#include "mkvtoolnix-gui/app.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/util/installation_checker.h"
#include "mkvtoolnix-gui/util/option_file.h"
#include "mkvtoolnix-gui/util/process.h"
#include "mkvtoolnix-gui/util/settings.h"

namespace mtx::gui::Util {

InstallationChecker::InstallationChecker(QObject *parent)
  : QObject{parent}
{
}

InstallationChecker::~InstallationChecker() {
}

void
InstallationChecker::runChecks() {
  m_problems.clear();
  auto mkvmergeExe  = Util::Settings::get().actualMkvmergeExe();
  auto versionRE    = QRegularExpression{Q("^mkvmerge [[:space:]]+ v ( [[:digit:].]+ )"), QRegularExpression::ExtendedPatternSyntaxOption};
  auto guiVersion   = Q(get_current_version().to_string());
  auto testMkvmerge = true;


  if (mkvmergeExe.isEmpty() || !QFileInfo{mkvmergeExe}.exists()) {
    m_problems << Problem{ ProblemType::MkvmergeNotFound, {} };
    testMkvmerge = false;
  }

  try {
    auto optFile = OptionFile::createTemporary(Q("MKVToolNix-process"), {});

  } catch (ProcessX const &ex) {
    m_problems << Problem{ ProblemType::TemporaryDirectoryNotWritable, Q(ex.what()) };
    testMkvmerge = false;
  }

  if (testMkvmerge) {
    auto process = Process::execute(mkvmergeExe, { Q("--version") });

    if (process->hasError())
      m_problems << Problem{ ProblemType::MkvmergeCannotBeExecuted, {} };

    else {
      // mkvmerge v9.7.1 ('Pandemonium') 64bit
      auto output = process->output().join(QString{});
      auto match  = versionRE.match(output);

      if (!match.hasMatch())
        m_problems << Problem{ ProblemType::MkvmergeVersionNotRecognized, output };

      else {
        m_mkvmergeVersion = match.captured(1);
        if (guiVersion != match.captured(1))
          m_problems << Problem{ ProblemType::MkvmergeVersionDiffers, match.captured(1) };
      }
    }
  }

  if (!m_problems.isEmpty())
    Q_EMIT problemsFound(m_problems);

  Q_EMIT finished();
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

QString
InstallationChecker::mkvmergeVersion()
  const {
  return m_mkvmergeVersion;
}

InstallationChecker::Problems
InstallationChecker::problems()
  const {
  return m_problems;
}

}
