#pragma once

#include "common/common_pch.h"

#include "common/cli_parser.h"

namespace mtx { namespace gui {

class GuiCliParserPrivate;
class GuiCliParser: public mtx::cli::parser_c {
protected:
  Q_DECLARE_PRIVATE(GuiCliParser);

  QScopedPointer<GuiCliParserPrivate> const d_ptr;

  explicit GuiCliParser(GuiCliParserPrivate &d, QWidget *parent);

public:
  GuiCliParser(std::vector<std::string> const &args);
  virtual ~GuiCliParser();

  void run();
  bool exitAfterParsing() const;

  QStringList rebuildCommandLine() const;

  QStringList const &configFiles() const;
  QStringList const &addToMerge() const;
  QStringList const &editChapters() const;
  QStringList const &editHeaders() const;

protected:
  void initParser();
  void handleFileNameArg();

  void displayHelp();
  void displayVersion();
  void setChaptersMode();
  void setHeadersMode();
  void setMergeMode();
  void raiseAndActivate();
  void enableDebugging();
  void enableHack();
};

}}
