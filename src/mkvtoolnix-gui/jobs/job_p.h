#pragma once

#include "common/common_pch.h"

namespace mtx { namespace gui { namespace Jobs {

class JobPrivate {
public:
  QUuid uuid;
  uint64_t id{};
  Job::Status status;
  QString description;
  QStringList output, warnings, errors, fullOutput;
  unsigned int progress{}, exitCode{std::numeric_limits<unsigned int>::max()};
  int warningsAcknowledged{}, errorsAcknowledged{};
  QDateTime dateAdded, dateStarted, dateFinished;
  bool quitAfterFinished{}, modified{true};

  QMutex mutex{QMutex::Recursive};

public:
  explicit JobPrivate(Job::Status pStatus);
};

}}}
