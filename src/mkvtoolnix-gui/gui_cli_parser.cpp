#include "common/common_pch.h"

#include <QDir>
#include <QFileInfo>
#include <QStringList>

#include "common/command_line.h"
#include "common/hacks.h"
#include "common/qt.h"
#include "mkvtoolnix-gui/gui_cli_parser.h"
#include "mkvtoolnix-gui/main_window/main_window.h"

namespace mtx { namespace gui {

class GuiCliParserPrivate {
  friend class GuiCliParser;

  QStringList configFiles, addToMerge, runInfoOn, editChapters, editHeaders;
  QStringList *toProcess{};
  bool exitAfterParsing{};

  explicit GuiCliParserPrivate() {
  }
};

GuiCliParser::GuiCliParser(std::vector<std::string> const &args)
  : mtx::cli::parser_c{args}
  , p_ptr{new GuiCliParserPrivate}
{
}

GuiCliParser::~GuiCliParser() {
}

#define OPT(spec, func, description) add_option(spec, std::bind(&GuiCliParser::func, this), description)

void
GuiCliParser::initParser() {
  add_information(YT("mkvtoolnix-gui [options] [file names]"));

  add_section_header(YT("Usage"));
  add_information(YT("mkvtoolnix-gui <configuration file names>"));
  add_information(YT("mkvtoolnix-gui [--multiplex|--info|--edit-chapters|--edit-headers] <file names>"));

  add_separator();

  add_information((boost::format("%1% %2% %3% %4%")
                   % YT("Listing configuration file names with the extension .mtxcfg causes the GUI to load the those configuration files in the appropriate tool.")
                   % YT("Any other file name is added as a source file for multiplexing, opened in the chapter editor or in the header editor depending on the current mode.")
                   % YT("The current mode can be changed with --multiplex, --edit-chapters or --edit-headers.")
                   % YT("The default mode is adding files for multiplexing.")).str());

  add_section_header(YT("Options"));

  OPT("multiplex|merge", setMergeMode,    (boost::format("%1% %2%") % YT("All following file names will be added as source files to the current multiplex settings.") % YT("This is the default mode.")).str());
  OPT("info",            setInfoMode,     YT("All following file names will be opened in the info tool."));
  OPT("edit-chapters",   setChaptersMode, YT("All following file names will be opened in the chapter editor."));
  OPT("edit-headers",    setHeadersMode,  YT("All following file names will be opened in the header editor."));
  OPT("activate",        raiseAndActivate, "");

  add_section_header(YT("Global options"));

  OPT("h|help",        displayHelp,     YT("Show this help."));
  OPT("V|version",     displayVersion,  YT("Show version information."));
  OPT("debug=option",  enableDebugging, {});
  OPT("engage=hack",   enableHack,      {});

  add_hook(mtx::cli::parser_c::ht_unknown_option, std::bind(&GuiCliParser::handleFileNameArg, this));
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
  auto p   = p_func();
  auto arg = QDir::toNativeSeparators(QFileInfo{Q(m_current_arg)}.absoluteFilePath());

  if (arg.endsWith(Q(".mtxcfg")))
    p->configFiles << arg;

  else
    *p->toProcess << arg;
}

void
GuiCliParser::setChaptersMode() {
  auto p       = p_func();
  p->toProcess = &p->editChapters;
}

void
GuiCliParser::setHeadersMode() {
  auto         p = p_func();
  p->toProcess = &p->editHeaders;
}

void
GuiCliParser::setInfoMode() {
  auto p       = p_func();
  p->toProcess = &p->runInfoOn;
}

void
GuiCliParser::setMergeMode() {
  auto         p = p_func();
  p->toProcess = &p->addToMerge;
}

void
GuiCliParser::run() {
  auto p               = p_func();
  p->toProcess         = &p->addToMerge;
  m_no_common_cli_args = true;

  initParser();
  parse_args();
}

void
GuiCliParser::displayHelp() {
  auto p = p_func();

  mxinfo(boost::format("%1%\n") % mtx::cli::g_usage_text);
  p->exitAfterParsing = true;
}

void
GuiCliParser::displayVersion() {
  auto p = p_func();

  mxinfo(boost::format("%1%\n") % mtx::cli::g_version_info);
  p->exitAfterParsing = true;
}

bool
GuiCliParser::exitAfterParsing()
  const {
  auto p = p_func();

  return p->exitAfterParsing;
}

QStringList
GuiCliParser::rebuildCommandLine()
  const {
  auto p    = p_func();
  auto args = QStringList{} << p->configFiles << p->addToMerge;

  if (!p->editChapters.isEmpty())
    args << Q("--edit-chapters") << p->editChapters;

  if (!p->editHeaders.isEmpty())
    args << Q("--edit-headers") << p->editHeaders;

  return args;
}

void
GuiCliParser::enableDebugging() {
  debugging_c::request(m_next_arg);
}

void
GuiCliParser::enableHack() {
  engage_hacks(m_next_arg);
}

QStringList const &
GuiCliParser::configFiles()
  const {
  return p_func()->configFiles;
}

QStringList const &
GuiCliParser::addToMerge()
  const {
  return p_func()->addToMerge;
}

QStringList const &
GuiCliParser::runInfoOn()
  const {
  return p_func()->runInfoOn;
}

QStringList const &
GuiCliParser::editChapters()
  const {
  return p_func()->editChapters;
}

QStringList const &
GuiCliParser::editHeaders()
  const {
  return p_func()->editHeaders;
}

}}
