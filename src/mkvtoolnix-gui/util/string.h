#pragma once

#include "common/common_pch.h"

#include <QRegularExpression>

class QString;
class QStringList;

namespace mtx { namespace gui { namespace Util {

enum EscapeMode {
  EscapeMkvtoolnix,
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

QString displayableDate(QDateTime const &date);

QString itemFlagsToString(Qt::ItemFlags const &flags);

QString mapToTopLevelCountryCode(QString const &countryCode);

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

}}}
