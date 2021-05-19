#include "common/common_pch.h"

#include <QDir>
#include <QFileInfo>
#include <QStringList>

#include "common/command_line.h"
#include "common/hacks.h"
#include "common/qt.h"
#include "common/version.h"
#include "mkvtoolnix-gui/chapter_editor/tool.h"
#include "mkvtoolnix-gui/gui_cli_parser.h"
#include "mkvtoolnix-gui/header_editor/tool.h"
#include "mkvtoolnix-gui/info/tool.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/merge/tool.h"

namespace mtx::gui {

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

void
GuiCliParser::initParser() {
  add_information(YT("mkvtoolnix-gui [options] [file names]"));

  add_section_header(YT("Usage"));
  add_information(YT("mkvtoolnix-gui <configuration file names>"));
  add_information(YT("mkvtoolnix-gui [--multiplex|--info|--edit-chapters|--edit-headers] <file names>"));

  add_separator();

  add_information(fmt::format("{0} {1} {2} {3}",
                              YT("Listing configuration file names with the extension .mtxcfg causes the GUI to load the those configuration files in the appropriate tool."),
                              YT("Any other file name is added as a source file for multiplexing, opened in the chapter editor or in the header editor depending on the current mode."),
                              YT("The current mode can be changed with --multiplex, --edit-chapters or --edit-headers."),
                              YT("The default mode is adding files for multiplexing.")));

  add_section_header(YT("Options"));

  add_option("multiplex|merge", std::bind(&GuiCliParser::setMergeMode,     this), fmt::format("{0} {1}", YT("All following file names will be added as source files to the current multiplex settings."), YT("This is the default mode.")));
  add_option("info",            std::bind(&GuiCliParser::setInfoMode,      this), YT("All following file names will be opened in the info tool."));
  add_option("edit-chapters",   std::bind(&GuiCliParser::setChaptersMode,  this), YT("All following file names will be opened in the chapter editor."));
  add_option("edit-headers",    std::bind(&GuiCliParser::setHeadersMode,   this), YT("All following file names will be opened in the header editor."));
  add_option("activate",        std::bind(&GuiCliParser::raiseAndActivate, this), {});

  add_section_header(YT("Global options"));

  add_option("h|help",       std::bind(&GuiCliParser::displayHelp,     this), YT("Show this help."));
  add_option("V|version",    std::bind(&GuiCliParser::displayVersion,  this), YT("Show version information."));
  add_option("debug=option", std::bind(&GuiCliParser::enableDebugging, this), {});
  add_option("engage=hack",  std::bind(&GuiCliParser::enableHack,      this), {});

  add_hook(mtx::cli::parser_c::ht_unknown_option, std::bind(&GuiCliParser::handleFileNameArg, this));
}

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
  auto p       = p_func();
  p->toProcess = &p->editHeaders;
}

void
GuiCliParser::setInfoMode() {
  auto p       = p_func();
  p->toProcess = &p->runInfoOn;
}

void
GuiCliParser::setMergeMode() {
  auto p       = p_func();
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

  mxinfo(fmt::format("{0}\n", mtx::cli::g_usage_text));
  p->exitAfterParsing = true;
}

void
GuiCliParser::displayVersion() {
  auto p = p_func();

  mxinfo(fmt::format("{0}\n", get_version_info("mkvtoolnix-gui", vif_full)));
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

  args << (  p->toProcess == &p->runInfoOn    ? Q("--info")
           : p->toProcess == &p->editHeaders  ? Q("--edit-headers")
           : p->toProcess == &p->editChapters ? Q("--edit-chapters")
           :                                    Q("--multiplex"));

  return args;
}

void
GuiCliParser::enableDebugging() {
  debugging_c::request(m_next_arg);
}

void
GuiCliParser::enableHack() {
  mtx::hacks::engage(m_next_arg);
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

ToolBase *
GuiCliParser::requestedTool()
  const {
  auto p = p_func();

  return   p->toProcess == &p->runInfoOn    ? static_cast<ToolBase *>(MainWindow::infoTool())
         : p->toProcess == &p->editHeaders  ? static_cast<ToolBase *>(MainWindow::headerEditorTool())
         : p->toProcess == &p->editChapters ? static_cast<ToolBase *>(MainWindow::chapterEditorTool())
         :                                    static_cast<ToolBase *>(MainWindow::mergeTool());
}

}
