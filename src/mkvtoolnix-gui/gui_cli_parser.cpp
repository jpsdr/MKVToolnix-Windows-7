#include "common/common_pch.h"

#include <QDir>
#include <QFileInfo>
#include <QStringList>

#include "common/command_line.h"
#include "common/qt.h"
#include "mkvtoolnix-gui/gui_cli_parser.h"
#include "mkvtoolnix-gui/main_window/main_window.h"

namespace mtx { namespace gui {

GuiCliParser::GuiCliParser(std::vector<std::string> const &args)
  : cli_parser_c{args}
{
}

#define OPT(spec, func, description) add_option(spec, std::bind(&GuiCliParser::func, this), description)

void
GuiCliParser::initParser() {
  add_information(YT("mkvtoolnix-gui [options] [file names]"));

  add_section_header(YT("Usage"));
  add_information(YT("mkvtoolnix-gui <configuration file names>"));
  add_information(YT("mkvtoolnix-gui [--merge|--edit-chapters|--edit-headers] <file names>"));

  add_separator();

  add_information((boost::format("%1% %2% %3% %4%")
                   % YT("Listing configuration file names with the extension .mtxcfg causes the GUI to load the those configuration files in the appropriate tool.")
                   % YT("Any other file name is added as a source file for merging, opened in the chapter editor or in the header editor depending on the current mode.")
                   % YT("The current mode can be changed with --merge, --edit-chapters or --edit-headers.")
                   % YT("The default mode is adding files for merging.")).str());

  add_section_header(YT("Options"));

  OPT("merge",         setMergeMode,    (boost::format("%1% %2%") % YT("All following file names will be added as source files to the current merge job.") % YT("This is the default mode.")).str());
  OPT("edit-chapters", setChaptersMode, YT("All following file names will be opened in the chapter editor."));
  OPT("edit-headers",  setHeadersMode,  YT("All following file names will be opened in the header editor."));
  OPT("activate",      raiseAndActivate, "");

  add_section_header(YT("Global options"));

  OPT("h|help",        displayHelp,     YT("Show this help."));
  OPT("V|version",     displayVersion,  YT("Show version information."));

  add_hook(cli_parser_c::ht_unknown_option, std::bind(&GuiCliParser::handleFileNameArg, this));
}

#undef OPT

void
GuiCliParser::raiseAndActivate() {
  auto mw = MainWindow::get();
  if (mw)
    mw->raiseAndActivate();
}

void
GuiCliParser::handleFileNameArg() {
  auto arg = QDir::toNativeSeparators(QFileInfo{Q(m_current_arg)}.absoluteFilePath());

  if (arg.endsWith(Q(".mtxcfg")))
    m_configFiles << arg;

  else
    *m_toProcess << arg;
}

void
GuiCliParser::setChaptersMode() {
  m_toProcess = &m_editChapters;
}

void
GuiCliParser::setHeadersMode() {
  m_toProcess = &m_editHeaders;
}

void
GuiCliParser::setMergeMode() {
  m_toProcess = &m_addToMerge;
}

void
GuiCliParser::run() {
  m_toProcess          = &m_addToMerge;
  m_no_common_cli_args = true;

  initParser();
  parse_args();
}

void
GuiCliParser::displayHelp() {
  mxinfo(boost::format("%1%\n") % usage_text);
  m_exitAfterParsing = true;
}

void
GuiCliParser::displayVersion() {
  mxinfo(boost::format("%1%\n") % version_info);
  m_exitAfterParsing = true;
}

bool
GuiCliParser::exitAfterParsing()
  const {
  return m_exitAfterParsing;
}

QStringList
GuiCliParser::rebuildCommandLine()
  const {
  auto args = QStringList{} << m_configFiles << m_addToMerge;

  if (!m_editChapters.isEmpty())
    args << Q("--edit-chapters") << m_editChapters;

  if (!m_editHeaders.isEmpty())
    args << Q("--edit-headers") << m_editHeaders;

  return args;
}

}}
