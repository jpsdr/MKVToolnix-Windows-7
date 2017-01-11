#include "common/common_pch.h"

#include <QtGlobal>
#include <QRegExp>
#include <QTemporaryFile>

#include "common/qt.h"
#include "mkvtoolnix-gui/util/option_file.h"
#include "mkvtoolnix-gui/util/process.h"
#include "mkvtoolnix-gui/util/string.h"

namespace mtx { namespace gui { namespace Util {

Process::Process(QString const &command,
                 QStringList const &args)
  : m_command{command}
  , m_args{args}
  , m_hasError{}
{
  connect(&m_process, &QProcess::readyReadStandardOutput,                                        this, &Process::dataAvailable);
  connect(&m_process, static_cast<void (QProcess::*)(QProcess::ProcessError)>(&QProcess::error), this, &Process::onError);
}

Process::~Process() {
}

void
Process::run() {
  m_process.start(m_command, m_args);
  m_process.waitForFinished(-1);
  dataAvailable();
}

QStringList
Process::output()
  const {
  return m_output.split(QRegExp{ "\r?\n" });
}

QProcess const &
Process::process()
  const {
  return m_process;
}

bool
Process::hasError()
  const {
  return m_hasError;
}

void
Process::onError() {
  m_hasError = true;
}

void
Process::dataAvailable() {
  auto output = m_process.readAllStandardOutput();
  m_output += QString::fromUtf8(output);
}

ProcessPtr
Process::execute(QString const &command,
                 QStringList const &args,
                 bool useTempFile) {
  auto runner = [](QString const &commandToUse, QStringList const &argsToUse) -> std::shared_ptr<Process> {
    auto pr = std::make_shared<Process>( commandToUse, argsToUse );
    pr->run();
    return pr;
  };

  if (!useTempFile)
    return runner(command, args);

  auto optFile = OptionFile::createTemporary(Q("MKVToolNix-process"), args);

  return runner(command, { QString{"@%1"}.arg(optFile->fileName()) });
}

// ----------------------------------------------------------------------

QString
currentUserName() {
  auto userName = QString::fromLocal8Bit(qgetenv("USERNAME"));
  if (userName.isEmpty())
    userName = QString::fromLocal8Bit(qgetenv("USER"));

  return userName;
}

}}}
