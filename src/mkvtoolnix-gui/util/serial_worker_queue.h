#pragma once

#include "common/common_pch.h"

#include <QObject>

class QThread;

namespace mtx::gui::Util {

class Runnable;

class SerialWorkerQueuePrivate;
class SerialWorkerQueue : public QObject {
  Q_OBJECT

protected:
  MTX_DECLARE_PRIVATE(SerialWorkerQueuePrivate)

  std::unique_ptr<SerialWorkerQueuePrivate> const p_ptr;

public:
  SerialWorkerQueue(QThread &thread);
  virtual ~SerialWorkerQueue();

  virtual void add(Runnable *work);

public slots:
  virtual void run();
  virtual void abort();

public:
  static std::pair<QThread *, SerialWorkerQueue *> create();
};

}
