#pragma once

#include "common/common_pch.h"

#include <QProcess>
#include <QString>

namespace mtx { namespace gui { namespace Util {

class ProcessX : public mtx::exception {
protected:
  std::string m_message;
public:
  explicit ProcessX(std::string const &message)  : m_message(message)       { }
  explicit ProcessX(boost::format const &message): m_message(message.str()) { }
  virtual ~ProcessX() throw() { }

  virtual char const *what() const throw() {
    return m_message.c_str();
  }
};

class Process;
using ProcessPtr = std::shared_ptr<Process>;

class Process: public QObject {
  Q_OBJECT;
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

public slots:
  virtual void dataAvailable();
  virtual void onError();

public:
  static ProcessPtr execute(QString const &command, QStringList const &args, bool useTempFile = true);
};

QString currentUserName();

}}}
