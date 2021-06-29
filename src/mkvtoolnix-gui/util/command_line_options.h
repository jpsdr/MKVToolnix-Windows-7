#pragma once

#include "common/common_pch.h"

#include <QString>
#include <QVector>

#include "mkvtoolnix-gui/util/string.h"

namespace mtx::gui::Util {

class CommandLineOption {
private:
  QString m_option;
  bool m_isFileName{};

public:
  CommandLineOption() = default;
  CommandLineOption(CommandLineOption const &) = default;
  CommandLineOption(CommandLineOption &&) = default;
  CommandLineOption(QString const &option, bool isFileName = false);

  CommandLineOption &operator =(CommandLineOption const &) = default;
  CommandLineOption &operator =(CommandLineOption &&) = default;

  QString effectiveOption(EscapeMode escapeMode) const;

  bool isEmpty() const;

public:
  static CommandLineOption fileName(QString const &name);
};

class CommandLineOptions {
private:
  QVector<CommandLineOption> m_options;
  QString m_executable;

public:
  CommandLineOptions &operator <<(QString const &option);
  CommandLineOptions &operator <<(CommandLineOption const &option);
  CommandLineOptions &operator <<(CommandLineOptions const &options);
  CommandLineOptions &operator <<(QStringList const &options);

  CommandLineOptions &setExecutable(QString const &executable);

  QStringList effectiveOptions(EscapeMode escapeMode) const;
  QString formatted(EscapeMode escapeMode) const;
};

} // namespace mtx::gui::Util
