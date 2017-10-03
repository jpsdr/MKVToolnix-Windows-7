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

  QStringList configFiles, addToMerge, editChapters, editHeaders;
  QStringList *toProcess{};
  bool exitAfterParsing{};

  explicit GuiCliParserPrivate() {
  }
};

GuiCliParser::GuiCliParser(std::vector<std::string> const &args)
  : mtx::cli::parser_c{args}
  , d_ptr{new GuiCliParserPrivate}
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
  add_information(YT("mkvtoolnix-gui [--multiplex|--edit-chapters|--edit-headers] <file names>"));

  add_separator();

  add_information((boost::format("%1% %2% %3% %4%")
                   % YT("Listing configuration file names with the extension .mtxcfg causes the GUI to load the those configuration files in the appropriate tool.")
                   % YT("Any other file name is added as a source file for multiplexing, opened in the chapter editor or in the header editor depending on the current mode.")
                   % YT("The current mode can be changed with --multiplex, --edit-chapters or --edit-headers.")
                   % YT("The default mode is adding files for multiplexing.")).str());

  add_section_header(YT("Options"));

  OPT("multiplex|merge", setMergeMode,    (boost::format("%1% %2%") % YT("All following file names will be added as source files to the current multiplex settings.") % YT("This is the default mode.")).str());
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
  Q_D(GuiCliParser);

  auto arg = QDir::toNativeSeparators(QFileInfo{Q(m_current_arg)}.absoluteFilePath());

  if (arg.endsWith(Q(".mtxcfg")))
    d->configFiles << arg;

  else
    *d->toProcess << arg;
}

void
GuiCliParser::setChaptersMode() {
  Q_D(GuiCliParser);

  d->toProcess = &d->editChapters;
}

void
GuiCliParser::setHeadersMode() {
  Q_D(GuiCliParser);

  d->toProcess = &d->editHeaders;
}

void
GuiCliParser::setMergeMode() {
  Q_D(GuiCliParser);

  d->toProcess = &d->addToMerge;
}

void
GuiCliParser::run() {
  Q_D(GuiCliParser);

  d->toProcess         = &d->addToMerge;
  m_no_common_cli_args = true;

  initParser();
  parse_args();
}

void
GuiCliParser::displayHelp() {
  Q_D(GuiCliParser);

  mxinfo(boost::format("%1%\n") % mtx::cli::g_usage_text);
  d->exitAfterParsing = true;
}

void
GuiCliParser::displayVersion() {
  Q_D(GuiCliParser);

  mxinfo(boost::format("%1%\n") % mtx::cli::g_version_info);
  d->exitAfterParsing = true;
}

bool
GuiCliParser::exitAfterParsing()
  const {
  Q_D(const GuiCliParser);

  return d->exitAfterParsing;
}

QStringList
GuiCliParser::rebuildCommandLine()
  const {
  Q_D(const GuiCliParser);

  auto args = QStringList{} << d->configFiles << d->addToMerge;

  if (!d->editChapters.isEmpty())
    args << Q("--edit-chapters") << d->editChapters;

  if (!d->editHeaders.isEmpty())
    args << Q("--edit-headers") << d->editHeaders;

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
  Q_D(const GuiCliParser);

  return d->configFiles;
}

QStringList const &
GuiCliParser::addToMerge()
  const {
  Q_D(const GuiCliParser);

  return d->addToMerge;
}

QStringList const &
GuiCliParser::editChapters()
  const {
  Q_D(const GuiCliParser);

  return d->editChapters;
}

QStringList const &
GuiCliParser::editHeaders()
  const {
  Q_D(const GuiCliParser);

  return d->editHeaders;
}

}}
