#include "common/common_pch.h"

#include <QDebug>
#include <QMutex>
#include <QMutexLocker>
#include <QThread>
#include <QWaitCondition>

#include "mkvtoolnix-gui/util/runnable.h"
#include "mkvtoolnix-gui/util/serial_worker_queue.h"

namespace mtx { namespace gui { namespace Util {

using namespace mtx::gui;

class SerialWorkerQueuePrivate {
public:
  QMutex m_mutex;
  QWaitCondition m_watch;
  QVector<Runnable *> m_workQueue;
  Runnable *m_currentWork{};
  QThread &m_thread;
  bool m_aborted{};

  SerialWorkerQueuePrivate(QThread &thread)
    : m_thread{thread}
  {
  }
};

SerialWorkerQueue::SerialWorkerQueue(QThread &thread)
  : p_ptr{new SerialWorkerQueuePrivate{thread}}
{
}

SerialWorkerQueue::~SerialWorkerQueue() {
}

void
SerialWorkerQueue::add(Runnable *work) {
  if (!work)
    return;

  auto p = p_func();

  QMutexLocker locker{&p->m_mutex};

  if (p->m_aborted)
    return;

  p->m_workQueue << work;
  p->m_watch.wakeOne();
}

void
SerialWorkerQueue::abort() {
  auto p = p_func();

  QMutexLocker locker{&p->m_mutex};

  if (p->m_aborted)
    return;

  p->m_aborted = true;

  for (auto work : p->m_workQueue)
    delete work;

  p->m_workQueue.clear();

  if (p->m_currentWork)
    p->m_currentWork->abort();

  p->m_watch.wakeOne();
}

void
SerialWorkerQueue::run() {
  auto p = p_func();

  p->m_mutex.lock();

  while (true) {
    if (!p->m_workQueue.isEmpty()) {
      p->m_currentWork = p->m_workQueue.takeFirst();
      p->m_mutex.unlock();

      p->m_currentWork->run();

      p->m_mutex.lock();

      if (p->m_currentWork->autoDelete())
        delete p->m_currentWork;

      p->m_currentWork = nullptr;

    } else if (p->m_aborted)
      break;

    else
      p->m_watch.wait(&p->m_mutex);
  }

  p->m_mutex.unlock();
}

std::pair<QThread *, SerialWorkerQueue *>
SerialWorkerQueue::create() {
  auto thread = new QThread;
  auto worker = new SerialWorkerQueue{*thread};

  worker->moveToThread(thread);

  connect(thread, &QThread::started,  worker, &SerialWorkerQueue::run);
  connect(thread, &QThread::finished, worker, &QObject::deleteLater);
  connect(thread, &QThread::finished, thread, &QObject::deleteLater);

  return { thread, worker };
}

}}}
