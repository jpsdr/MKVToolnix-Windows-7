#pragma once

#include "common/common_pch.h"

#include <QtContainerFwd>
#include <QRegularExpression>

class QString;

namespace mtx::gui::Util {

enum EscapeMode {
  EscapeJSON,
  EscapeShellUnix,
  EscapeShellCmdExeArgument,
  EscapeShellCmdExeProgram,
  EscapeKeyboardShortcuts,
  DontEscape,
#if defined(SYS_WINDOWS)
  EscapeShellNative = EscapeShellCmdExeArgument,
#else
  EscapeShellNative = EscapeShellUnix,
#endif
};

QString escape(QString const &source, EscapeMode mode);
QStringList escape(QStringList const &source, EscapeMode mode);
QString unescape(QString const &source, EscapeMode mode);
QStringList unescape(QStringList const &source, EscapeMode mode);
QStringList unescapeSplit(QString const &source, EscapeMode mode);

QString joinSentences(QStringList const &sentences);

QString itemFlagsToString(Qt::ItemFlags const &flags);

QString replaceApplicationDirectoryWithMtxVariable(QString string);
QString replaceMtxVariableWithApplicationDirectory(QString string);

class DeferredRegularExpression {
private:
  std::unique_ptr<QRegularExpression> m_re;
  QString m_pattern;
  QRegularExpression::PatternOptions m_options;

public:
  DeferredRegularExpression(QString const &pattern, QRegularExpression::PatternOptions options = QRegularExpression::NoPatternOption);
  ~DeferredRegularExpression();
  QRegularExpression &operator *();
};

}
