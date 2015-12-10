#ifndef MTX_MKVTOOLNIX_GUI_UTIL_STRING_H
#define MTX_MKVTOOLNIX_GUI_UTIL_STRING_H

#include "common/common_pch.h"

class QString;
class QStringList;

namespace mtx { namespace gui { namespace Util {

enum EscapeMode {
  EscapeMkvtoolnix,
  EscapeShellUnix,
  EscapeShellCmdExeArgument,
  EscapeShellCmdExeProgram,
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

}}}

#endif  // MTX_MKVTOOLNIX_GUI_UTIL_STRING_H
