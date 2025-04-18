#include "common/common_pch.h"

#include <QDebug>
#include <QDir>
#include <QRegularExpression>
#include <QString>
#include <QStringList>

#include "common/json.h"
#include "common/list_utils.h"
#include "common/qt.h"
#include "common/strings/editing.h"
#include "mkvtoolnix-gui/app.h"
#include "mkvtoolnix-gui/util/json.h"
#include "mkvtoolnix-gui/util/string.h"

namespace mtx::gui::Util {

DeferredRegularExpression::DeferredRegularExpression(QString const &pattern,
                                                     QRegularExpression::PatternOptions options)
  : m_pattern{pattern}
  , m_options{options}
{
}

DeferredRegularExpression::~DeferredRegularExpression() {
}

QRegularExpression &
DeferredRegularExpression::operator *() {
  if (!m_re) {
    m_re.reset(new QRegularExpression{m_pattern, m_options});
    if (!m_re->isValid())
      qDebug() << "mtxgui::DeferredRegularExpression: compilation failed for pattern" << m_pattern << "at position" << m_re->patternErrorOffset() << ":" << m_re->errorString();
  }

  return *m_re;
}

// ----------------------------------------------------------------------

static QString
escapeShellUnix(QString const &source) {
  if (source.isEmpty())
    return Q("\"\"");

  if (!source.contains(QRegularExpression{"[^\\w%+,\\-./:=]"}))
    return source;

  auto copy = source;

  if (copy.startsWith(QChar{'@'}))
    copy.insert(0, QChar{'@'});

  // ' -> '\''
  copy.replace(QRegularExpression{"'"}, Q("'\\''"));

  copy = Q("'%1'").arg(copy);
  copy.replace(QRegularExpression{"^''"}, Q(""));
  copy.replace(QRegularExpression{"''$"}, Q(""));

  return copy;
}

static QStringList
unescapeSplitShellUnix(QString const &source) {
  QStringList arguments;

  auto *currentArgument = static_cast<QString *>(nullptr);
  auto escapeNext       = false;
  auto singleQuoted     = false;
  auto doubleQuoted     = false;
  auto ensureHaveArg    = [&currentArgument, &arguments]() {
    if (!currentArgument) {
      arguments << Q("");
      currentArgument = &arguments.last();
    }
  };
  auto appendChar = [&currentArgument, &ensureHaveArg](QChar const &c) {
    ensureHaveArg();
    *currentArgument += c;
  };

  for (auto const &c : source) {
    if (escapeNext) {
      escapeNext = false;
      appendChar(c);

    } else if (singleQuoted) {
      if (c == Q('\''))
        singleQuoted = false;
      else
        appendChar(c);

    } else if (c == Q('\\'))
      escapeNext = true;

    else if (doubleQuoted) {
      if (c == Q('"'))
        doubleQuoted = false;
      else
        appendChar(c);

    } else if (c == Q('\'')) {
      ensureHaveArg();
      singleQuoted = true;

    } else if (c == Q('"')) {
      ensureHaveArg();
      doubleQuoted = true;

    } else if (c == Q(' '))
      currentArgument = nullptr;

    else
      appendChar(c);
  }

  return arguments;
}

static QString
escapeShellWindows(QString const &source) {
  if (source.isEmpty())
    return Q("^\"^\"");

  if (!source.contains(QRegularExpression{"[^\\w+,\\-./:=]"}))
    return source;

  auto copy = QString{'"'};

  if (source.startsWith(QChar{'@'}))
    copy.append(QChar{'@'});

  for (auto it = source.begin(), end = source.end() ; ; ++it) {
    QString::size_type numBackslashes = 0;

    while ((it != end) && (*it == QChar{'\\'})) {
      ++it;
      ++numBackslashes;
    }

    if (it == end) {
      copy += QString{numBackslashes * 2, QChar{'\\'}};
      break;

    } else if (*it == QChar{'"'})
      copy += QString{numBackslashes * 2 + 1, QChar{'\\'}} + *it;

    else
      copy += QString{numBackslashes, QChar{'\\'}} + *it;
  }

  copy += QChar{'"'};

  copy.replace(QRegularExpression{"([()%!^\"<>&|])"}, Q("^\\1"));

  return copy;
}

static QString
escapeShellWindowsProgram(QString const &source) {
  if (source.contains(QRegularExpression{"[&<>[\\]{}^=;!'+,`~ ]"}))
    return Q("\"%1\"").arg(source);

  return source;
}

static QString
escapeKeyboardShortcuts(QString const &text) {
  auto copy = text;
  return copy.replace(Q("&"), Q("&&"));
}

static QString
unescapeKeyboardShortcuts(QString const &text) {
  auto copy = text;
  return copy.replace(Q("&&"), Q("&"));
}

QString
escape(QString const &source,
       EscapeMode mode) {
  return EscapeShellUnix           == mode ? escapeShellUnix(source)
       : EscapeShellCmdExeArgument == mode ? escapeShellWindows(source)
       : EscapeShellCmdExeProgram  == mode ? escapeShellWindowsProgram(source)
       : EscapeKeyboardShortcuts   == mode ? escapeKeyboardShortcuts(source)
       :                                     source;
}

QString
unescape(QString const &source,
         EscapeMode mode) {
  Q_ASSERT(mtx::included_in(mode, EscapeKeyboardShortcuts));

  return unescapeKeyboardShortcuts(source);
}

static QStringList
escapeJson(QStringList source) {
  // QStrings can be null in addition to being empty. A NULL string
  // converts to a null QVariant, which in turn converts to null JSON
  // object. mkvmerge expects strings, though, so replace all null
  // strings with empty ones instead.

  for (auto &string : source)
    if (string.isNull())
      string = Q("");

  return { Q(mtx::json::dump(variantToNlohmannJson(source), 2)) };
}

QStringList
escape(QStringList const &source,
       EscapeMode mode) {
  if (EscapeJSON == mode)
    return escapeJson(source);

  auto escaped = QStringList{};

  for (auto const &string : source)
    escaped << escape(string, mode);

  return escaped;
}

QStringList
unescape(QStringList const &source,
         EscapeMode mode) {
  auto unescaped = QStringList{};
  for (auto const &string : source)
    unescaped << unescape(string, mode);

  return unescaped;
}

QStringList
unescapeSplit(QString const &source,
              EscapeMode mode) {
  Q_ASSERT(EscapeShellUnix == mode);

  return unescapeSplitShellUnix(source);
}

QString
joinSentences(QStringList const &sentences) {
  // TODO: act differently depending on the UI locale. Some languages,
  // e.g. Japanese, don't join sentences with spaces.
  return sentences.join(" ");
}

QString
itemFlagsToString(Qt::ItemFlags const &flags) {
  auto items = QStringList{};

  if (flags & Qt::ItemIsSelectable)     items << "IsSelectable";
  if (flags & Qt::ItemIsEditable)       items << "IsEditable";
  if (flags & Qt::ItemIsDragEnabled)    items << "IsDragEnabled";
  if (flags & Qt::ItemIsDropEnabled)    items << "IsDropEnabled";
  if (flags & Qt::ItemIsUserCheckable)  items << "IsUserCheckable";
  if (flags & Qt::ItemIsEnabled)        items << "IsEnabled";
  if (flags & Qt::ItemNeverHasChildren) items << "NeverHasChildren";

  return items.join(Q("|"));
}

QString
replaceApplicationDirectoryWithMtxVariable(QString string) {
  auto applicationDirectory = App::applicationDirPath().replace(Q("\\"), Q("/"));

  return string.replace(Q("\\"), Q("/")).replace(applicationDirectory, Q("<MTX_INSTALLATION_DIRECTORY>"), Qt::CaseInsensitive);
}

QString
replaceMtxVariableWithApplicationDirectory(QString string) {
  auto applicationDirectory = QDir::toNativeSeparators(App::applicationDirPath());

  return string.replace(Q("<MTX_INSTALLATION_DIRECTORY>"), applicationDirectory);
}

}
