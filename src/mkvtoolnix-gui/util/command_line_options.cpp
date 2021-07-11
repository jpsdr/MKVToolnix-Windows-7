#include "mkvtoolnix-gui/util/command_line_options.h"

#include <QDir>

#include "common/list_utils.h"
#include "mkvtoolnix-gui/util/command_line_options.h"

namespace mtx::gui::Util {

CommandLineOption::CommandLineOption(QString const &option,
                                     bool isFileName)
  : m_option{option}
  , m_isFileName{isFileName}
{
}

QString
CommandLineOption::effectiveOption(Util::EscapeMode escape)
  const {
  if (!m_isFileName)
    return m_option;

  if (!mtx::included_in(escape, Util::EscapeShellUnix, Util::EscapeShellCmdExeArgument, Util::EscapeShellCmdExeProgram))
    return QDir::toNativeSeparators(m_option);

  auto copy = m_option;

  if (escape == Util::EscapeShellUnix)
    return copy.replace(QChar{'\\'}, QChar{'/'});

  return copy.replace(QChar{'/'}, QChar{'\\'});
}

bool
CommandLineOption::isEmpty()
  const {
  return m_option.isEmpty();
}

CommandLineOption
CommandLineOption::fileName(QString const &name) {
  return CommandLineOption{name, true};
}

// ------------------------------------------------------------

CommandLineOptions &
CommandLineOptions::setExecutable(QString const &executable) {
  m_executable = executable;
  return *this;
}

CommandLineOptions &
CommandLineOptions::operator <<(QString const &option) {
  m_options << CommandLineOption{option};
  return *this;
}

CommandLineOptions &
CommandLineOptions::operator <<(CommandLineOption const &option) {
  m_options << option;
  return *this;
}

CommandLineOptions &
CommandLineOptions::operator <<(CommandLineOptions const &options) {
  m_options += options.m_options;
  return *this;
}

CommandLineOptions &
CommandLineOptions::operator <<(QStringList const &options) {
  for (auto const &option : options)
    m_options << CommandLineOption{option};
  return *this;
}

QStringList
CommandLineOptions::effectiveOptions(Util::EscapeMode escapeMode)
  const {
  QStringList options;

  options.reserve(m_options.size());

  auto mode = escapeMode == EscapeShellCmdExeProgram ? EscapeShellCmdExeArgument : escapeMode;

  for (auto const &option : m_options)
    options << option.effectiveOption(mode);

  return options;
}

QString
CommandLineOptions::formatted(Util::EscapeMode escapeMode)
  const {
  QStringList options;

  options.reserve(m_options.size() + 1);

  if (escapeMode == EscapeShellCmdExeProgram)
    escapeMode = EscapeShellCmdExeArgument;

  for (auto const &option : m_options)
    options << option.effectiveOption(escapeMode);

  auto escapedOptions = Util::escape(options, escapeMode);

  if (escapeMode == EscapeJSON)
    return Util::escape(options, escapeMode).join(QString{});

  if (!m_executable.isEmpty()) {
    auto exeMode = escapeMode == EscapeShellCmdExeArgument ? EscapeShellCmdExeProgram : escapeMode;
    escapedOptions.prepend(Util::escape(Util::CommandLineOption::fileName(m_executable).effectiveOption(exeMode), exeMode));
  }

  return escapedOptions.join(QChar{' '});
}

} // namespace mtx::gui::Util
