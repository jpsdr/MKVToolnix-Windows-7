#pragma once

#include "common/common_pch.h"

#include <QObject>
#include <QVector>

class QString;

namespace mtx { namespace gui { namespace Util {

class InstallationChecker: public QObject {
  Q_OBJECT;

public:
  enum class ProblemType {
    FileNotFound,
    MkvmergeNotFound,
    MkvmergeCannotBeExecuted,
    MkvmergeVersionNotRecognized,
    MkvmergeVersionDiffers,
  };

  using Problem  = std::pair<ProblemType, QString>;
  using Problems = QVector<Problem>;

public:
  explicit InstallationChecker(QObject *parent = nullptr);
  virtual ~InstallationChecker();

signals:
  void problemsFound(Util::InstallationChecker::Problems const &results);
  void finished();

public slots:
  void runChecks();

public:
  static void checkInstallation();
};

}}}
