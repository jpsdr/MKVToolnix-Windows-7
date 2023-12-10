#pragma once

#include "common/common_pch.h"

namespace mtx::gui::Jobs {

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

  QRecursiveMutex mutex{};

public:
  explicit JobPrivate(Job::Status pStatus);
  virtual ~JobPrivate() = default;
};

}
