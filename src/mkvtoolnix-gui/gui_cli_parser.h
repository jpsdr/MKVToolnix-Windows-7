#pragma once

#include "common/common_pch.h"

#include "common/cli_parser.h"
#include "common/qt.h"

namespace mtx { namespace gui {

class GuiCliParserPrivate;
class GuiCliParser: public mtx::cli::parser_c {
protected:
  MTX_DECLARE_PRIVATE(GuiCliParserPrivate)

  std::unique_ptr<GuiCliParserPrivate> const p_ptr;

  explicit GuiCliParser(GuiCliParserPrivate &p, QWidget *parent);

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
  QStringList const &runInfoOn() const;

protected:
  void initParser();
  void handleFileNameArg();

  void displayHelp();
  void displayVersion();
  void setChaptersMode();
  void setHeadersMode();
  void setInfoMode();
  void setMergeMode();
  void raiseAndActivate();
  void enableDebugging();
  void enableHack();
};

}}
