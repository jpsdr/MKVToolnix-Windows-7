#pragma once

#include "common/common_pch.h"

#include <QProcess>
#include <QString>

#include "mkvtoolnix-gui/util/process_x.h"

namespace mtx::gui::Util {

class Process;
using ProcessPtr = std::shared_ptr<Process>;

class Process: public QObject {
  Q_OBJECT
private:
  QProcess m_process;
  QString m_command, m_output;
  QStringList m_args;
  bool m_hasError;

public:
  Process(QString const &command, QStringList const &args);
  virtual ~Process();

  virtual QStringList output() const;
  virtual QProcess const &process() const;
  virtual bool hasError() const;
  virtual void run();

public Q_SLOTS:
  virtual void dataAvailable();
  virtual void onError();

public:
  static ProcessPtr execute(QString const &command, QStringList const &args, bool useTempFile = true);
};

QString currentUserName();

}
