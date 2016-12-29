#ifndef MTX_MKVTOOLNIX_GUI_GUI_CLI_PARSER_H
#define MTX_MKVTOOLNIX_GUI_GUI_CLI_PARSER_H

#include "common/common_pch.h"

#include "common/cli_parser.h"

namespace mtx { namespace gui {

class GuiCliParser: public cli_parser_c {
public:
  QStringList m_configFiles, m_addToMerge, m_editChapters, m_editHeaders;

protected:
  QStringList *m_toProcess{};
  bool m_exitAfterParsing{};

public:
  GuiCliParser(std::vector<std::string> const &args);

  void run();
  bool exitAfterParsing() const;

  QStringList rebuildCommandLine() const;

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

#endif // MTX_MKVTOOLNIX_GUI_GUI_CLI_PARSER_H
