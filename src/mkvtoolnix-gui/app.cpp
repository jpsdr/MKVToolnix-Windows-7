#include "common/common_pch.h"

#include <QFile>
#include <QLibraryInfo>
#include <QLocalServer>
#include <QLocalSocket>
#include <QThread>
#include <QTranslator>

#include <boost/optional.hpp>

#include "common/command_line.h"
#include "common/extern_data.h"
#include "common/fs_sys_helpers.h"
#include "common/iso639.h"
#include "common/qt.h"
#include "common/random.h"
#include "common/unique_numbers.h"
#include "common/version.h"
#include "mkvtoolnix-gui/app.h"
#include "mkvtoolnix-gui/jobs/program_runner.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/merge/tool.h"
#include "mkvtoolnix-gui/util/container.h"
#include "mkvtoolnix-gui/util/media_player.h"
#include "mkvtoolnix-gui/util/network_access_manager.h"
#include "mkvtoolnix-gui/util/process.h"
#include "mkvtoolnix-gui/util/settings.h"

namespace mtx { namespace gui {

static Iso639LanguageList s_iso639Languages, s_commonIso639Languages;
static TopLevelDomainCountryCodeList s_topLevelDomainCountryCodes, s_commonTopLevelDomainCountryCodes;
static CharacterSetList s_characterSets, s_commonCharacterSets;
static QHash<QString, QString> s_iso639_2LanguageCodeToDescription, s_topLevelDomainCountryCodeToDescription;

static boost::optional<bool> s_is_installed;

class AppPrivate {
  friend class App;

  std::unique_ptr<QTranslator> m_currentTranslator;
  std::unique_ptr<GuiCliParser> m_cliParser;
  std::unique_ptr<QLocalServer> m_instanceCommunicator;
  std::unique_ptr<Jobs::ProgramRunner> m_programRunner;
  std::unique_ptr<Util::MediaPlayer> m_mediaPlayer;
  Util::NetworkAccessManager *m_networkAccessManager{new Util::NetworkAccessManager{}};
  QThread m_networkAccessManagerThread;
  bool m_otherInstanceRunning{};

  explicit AppPrivate()
  {
  }
};

App::App(int &argc,
         char **argv)
  : QApplication{argc, argv}
  , d_ptr{new AppPrivate{}}
{
  mtx_common_init("mkvtoolnix-gui", argv[0]);
  mtx::cli::g_version_info = get_version_info("mkvtoolnix-gui", vif_full);

  qsrand(random_c::generate_64bits());

  // The routines for handling unique numbers cannot cope with
  // multiple chapters being worked on at the same time as they safe
  // already-used numbers in one static container. So just disable them.
  ignore_unique_numbers(UNIQUE_CHAPTER_IDS);
  ignore_unique_numbers(UNIQUE_EDITION_IDS);

  QCoreApplication::setOrganizationName("bunkus.org");
  QCoreApplication::setOrganizationDomain("bunkus.org");
  QCoreApplication::setApplicationName("mkvtoolnix-gui");

#ifdef SYS_WINDOWS
  QApplication::setStyle(Q("windowsvista"));
#endif

  Util::Settings::migrateFromRegistry();
  Util::Settings::get().load();

  setupInstanceCommunicator();
  setupNetworkAccessManager();

  QObject::connect(this, &App::aboutToQuit, this, &App::saveSettings);

  initializeLocale();
  App::setupUiFont();
}

App::~App() {
  Q_D(App);

  d->m_networkAccessManagerThread.quit();
  d->m_networkAccessManagerThread.wait();
}

Util::NetworkAccessManager &
App::networkAccessManager() {
  Q_D(App);

  return *d->m_networkAccessManager;
}

void
App::setupNetworkAccessManager() {
  Q_D(App);

  d->m_networkAccessManager->moveToThread(&d->m_networkAccessManagerThread);

  connect(&d->m_networkAccessManagerThread, &QThread::finished, d->m_networkAccessManager, &QObject::deleteLater);

  d->m_networkAccessManagerThread.start();
}

void
App::setupUiFont() {
  auto &cfg    = Util::Settings::get();
  auto newFont = App::font();

  newFont.setFamily(cfg.m_uiFontFamily);
  newFont.setPointSize(std::max(cfg.m_uiFontPointSize, 5));

  App::setFont(newFont);
}

QString
App::communicatorSocketName() {
  return Q("MKVToolNix-GUI-Instance-Communicator-%1").arg(Util::currentUserName());
}

void
App::setupInstanceCommunicator() {
  Q_D(App);

  auto socketName = communicatorSocketName();
  auto socket     = std::make_unique<QLocalSocket>(this);

  socket->connectToServer(socketName);

  if (socket->state() == QLocalSocket::ConnectedState) {
    socket->disconnect();

    d->m_otherInstanceRunning = true;

    return;
  }

  QLocalServer::removeServer(socketName);

  d->m_instanceCommunicator = std::make_unique<QLocalServer>(this);

  if (d->m_instanceCommunicator->listen(socketName))
    connect(d->m_instanceCommunicator.get(), &QLocalServer::newConnection, this, &App::receiveInstanceCommunication);

  else
    d->m_instanceCommunicator.reset(nullptr);
}

void
App::saveSettings()
  const {
  Util::Settings::get().save();
}

App *
App::instance() {
  return static_cast<App *>(QApplication::instance());
}

Jobs::ProgramRunner &
App::setupProgramRunner() {
  Q_D(App);

  if (!d->m_programRunner)
    d->m_programRunner = Jobs::ProgramRunner::create();
  return *d->m_programRunner;
}

Jobs::ProgramRunner &
App::programRunner() {
  return instance()->setupProgramRunner();
}

Util::MediaPlayer &
App::setupMediaPlayer() {
  Q_D(App);

  if (!d->m_mediaPlayer)
    d->m_mediaPlayer.reset(new Util::MediaPlayer);
  return *d->m_mediaPlayer;
}

Util::MediaPlayer &
App::mediaPlayer() {
  return instance()->setupMediaPlayer();
}

void
App::retranslateUi() {
  if (MainWindow::get())
    MainWindow::get()->retranslateUi();
}

void
App::reinitializeLanguageLists() {
  s_iso639Languages.clear();
  s_commonIso639Languages.clear();
  s_topLevelDomainCountryCodes.clear();
  s_commonTopLevelDomainCountryCodes.clear();
  s_iso639_2LanguageCodeToDescription.clear();
  s_topLevelDomainCountryCodeToDescription.clear();
  s_characterSets.clear();
  s_commonCharacterSets.clear();

  initializeLanguageLists();
}

void
App::initializeLanguageLists() {
  if (!s_iso639Languages.empty())
    return;

  initializeIso639Languages();
  initializeTopLevelDomainCountryCodes();
  initializeCharacterSets();
}

void
App::initializeIso639Languages() {
  auto &cfg = Util::Settings::get();

  s_iso639Languages.reserve(g_iso639_languages.size());
  s_commonIso639Languages.reserve(cfg.m_oftenUsedLanguages.size());

  for (auto const &language : g_iso639_languages) {
    auto languageCode = Q(language.iso639_2_code);
    auto description  = Q("%1 (%2)").arg(Q(language.english_name)).arg(languageCode);
    auto isCommon     = cfg.m_oftenUsedLanguages.indexOf(languageCode) != -1;

    s_iso639Languages.emplace_back(description, languageCode);
    if (isCommon)
      s_commonIso639Languages.emplace_back(description, languageCode);

    s_iso639_2LanguageCodeToDescription[languageCode] = description;
  }

  brng::sort(s_iso639Languages);
  brng::sort(s_commonIso639Languages);
}

void
App::initializeTopLevelDomainCountryCodes() {
  auto &cfg = Util::Settings::get();

  s_topLevelDomainCountryCodes.reserve(g_cctlds.size());
  s_commonTopLevelDomainCountryCodes.reserve(g_cctlds.size());

  for (auto const &country : g_cctlds) {
    auto countryCode = Q(country.code);
    auto description = Q("%1 (%2)").arg(Q(country.country)).arg(countryCode);
    auto isCommon    = cfg.m_oftenUsedCountries.indexOf(countryCode) != -1;

    s_topLevelDomainCountryCodes.emplace_back(description, countryCode);
    if (isCommon)
      s_commonTopLevelDomainCountryCodes.emplace_back(description, countryCode);

    s_topLevelDomainCountryCodeToDescription[countryCode] = description;
  }

  brng::sort(s_topLevelDomainCountryCodes);
  brng::sort(s_commonTopLevelDomainCountryCodes);
}

void
App::initializeCharacterSets() {
  auto &cfg = Util::Settings::get();

  s_characterSets.reserve(sub_charsets.size());
  s_commonCharacterSets.reserve(cfg.m_oftenUsedCharacterSets.size());

  for (auto const &characterSet : sub_charsets) {
    auto qCharacterSet = Q(characterSet);

    s_characterSets.emplace_back(qCharacterSet);
    if (cfg.m_oftenUsedCharacterSets.contains(qCharacterSet))
      s_commonCharacterSets.emplace_back(qCharacterSet);
  }

  brng::sort(s_characterSets);
  brng::sort(s_commonCharacterSets);
}

Iso639LanguageList const &
App::iso639Languages() {
  initializeLanguageLists();
  return s_iso639Languages;
}

Iso639LanguageList const &
App::commonIso639Languages() {
  initializeLanguageLists();
  return s_commonIso639Languages;
}

TopLevelDomainCountryCodeList const &
App::topLevelDomainCountryCodes() {
  initializeLanguageLists();
  return s_topLevelDomainCountryCodes;
}

TopLevelDomainCountryCodeList const &
App::commonTopLevelDomainCountryCodes() {
  initializeLanguageLists();
  return s_commonTopLevelDomainCountryCodes;
}

CharacterSetList const &
App::characterSets() {
  initializeLanguageLists();
  return s_characterSets;
}

CharacterSetList const &
App::commonCharacterSets() {
  initializeLanguageLists();
  return s_commonCharacterSets;
}

QString const &
App::descriptionFromIso639_2LanguageCode(QString const &code) {
  initializeLanguageLists();
  return s_iso639_2LanguageCodeToDescription.contains(code) ? s_iso639_2LanguageCodeToDescription[code] : code;
}

QString const &
App::descriptionFromTopLevelDomainCountryCode(QString const &code) {
  initializeLanguageLists();
  return s_topLevelDomainCountryCodeToDescription.contains(code) ? s_topLevelDomainCountryCodeToDescription[code] : code;
}

bool
App::isInstalled() {
  if (!s_is_installed)
    s_is_installed.reset(mtx::sys::is_installed());
  return *s_is_installed;
}

void
App::initializeLocale(QString const &requestedLocale) {
  auto locale = Util::Settings::get().localeToUse(requestedLocale);

#if defined(HAVE_LIBINTL_H)
  Q_D(App);

  if (!locale.isEmpty()) {
    if (d->m_currentTranslator)
      removeTranslator(d->m_currentTranslator.get());
    d->m_currentTranslator.reset();

    auto translator = std::make_unique<QTranslator>();
    auto paths      = QStringList{} << Q("%1/locale/libqt").arg(applicationDirPath())
                                    << QLibraryInfo::location(QLibraryInfo::TranslationsPath);

    for (auto const &path : paths)
      if (translator->load(Q("qt_%1").arg(locale), path))
        break;

    installTranslator(translator.get());

    d->m_currentTranslator           = std::move(translator);
    Util::Settings::get().m_uiLocale = locale;
  }

#endif  // HAVE_LIBINTL_H

  init_locales(to_utf8(locale));

  retranslateUi();
}

bool
App::isOtherInstanceRunning()
  const {
  Q_D(const App);

  return d->m_otherInstanceRunning;
}

void
App::sendCommandLineArgumentsToRunningInstance() {
  Q_D(App);

  auto args = d->m_cliParser->rebuildCommandLine();
  if (!args.isEmpty())
    sendArgumentsToRunningInstance(args);
}

void
App::raiseAndActivateRunningInstance() {
  if (isOtherInstanceRunning())
    sendArgumentsToRunningInstance(QStringList{} << "--activate");
}

void
App::sendArgumentsToRunningInstance(QStringList const &args) {
  auto size  = 0;
  auto block = QByteArray{};
  QDataStream out{&block, QIODevice::WriteOnly};

  out.setVersion(QDataStream::Qt_5_0);
  out << size;
  out << args;

  size = block.size() - sizeof(int);
  out.device()->seek(0);
  out << size;

  auto socket = std::make_unique<QLocalSocket>(this);
  socket->connectToServer(communicatorSocketName());

  if (socket->state() != QLocalSocket::ConnectedState)
    return;

  socket->write(block);
  socket->flush();

  if (!socket->waitForReadyRead(10000))
    return;

  bool ok = false;
  socket->read(reinterpret_cast<char *>(&ok), sizeof(bool));
}

bool
App::parseCommandLineArguments(QStringList const &args) {
  Q_D(App);

  if (args.isEmpty()) {
    raiseAndActivateRunningInstance();
    return true;
  }

  d->m_cliParser.reset(new GuiCliParser{Util::toStdStringVector(args, 1)});
  d->m_cliParser->run();

  if (d->m_cliParser->exitAfterParsing())
    return false;

  if (!isOtherInstanceRunning())
    return true;

  raiseAndActivateRunningInstance();
  sendCommandLineArgumentsToRunningInstance();

  return false;
}

void
App::handleCommandLineArgumentsLocally() {
  Q_D(App);

  if (!d->m_cliParser->configFiles().isEmpty())
    emit openConfigFilesRequested(d->m_cliParser->configFiles());

  if (!d->m_cliParser->addToMerge().isEmpty())
    emit addingFilesToMergeRequested(d->m_cliParser->addToMerge());

  if (!d->m_cliParser->editChapters().isEmpty())
    emit editingChaptersRequested(d->m_cliParser->editChapters());

  if (!d->m_cliParser->editHeaders().isEmpty())
    emit editingHeadersRequested(d->m_cliParser->editHeaders());
}

void
App::receiveInstanceCommunication() {
  Q_D(App);

  auto socket = d->m_instanceCommunicator->nextPendingConnection();
  connect(socket, &QLocalSocket::disconnected, socket, &QLocalSocket::deleteLater);

  QDataStream in{socket};
  in.setVersion(QDataStream::Qt_5_0);

  auto start = QDateTime::currentDateTime();
  auto size  = 0;
  auto args  = QStringList{};
  auto ok    = false;

  while (true) {
    if (!socket->waitForReadyRead(10000))
      break;

    if (!size && (socket->bytesAvailable() >= static_cast<int>(sizeof(int))))
      in >> size;

    if (size && (socket->bytesAvailable()) >= size) {
      in >> args;
      ok = true;
      break;
    }

    auto elapsed = start.msecsTo(QDateTime::currentDateTime());
    if (elapsed >= 10000)
      break;
  }

  if (!ok) {
    socket->disconnectFromServer();
    return;
  }

  socket->write(reinterpret_cast<char *>(&ok), sizeof(bool));
  socket->flush();

  d->m_cliParser.reset(new GuiCliParser{Util::toStdStringVector(args)});
  d->m_cliParser->run();
  handleCommandLineArgumentsLocally();
}

QString
App::settingsBaseGroupName() {
  return Q("MKVToolNix GUI Settings");
}

void
App::run() {
  if (!parseCommandLineArguments(App::arguments()))
    return;

  // Change directory after processing the command line arguments so
  // that relative file names are resolved correctly.
  QDir::setCurrent(QDir::homePath());

  auto mainWindow = std::make_unique<MainWindow>();
  mainWindow->show();

  handleCommandLineArgumentsLocally();

  MainWindow::mergeTool()->addMergeTabIfNoneOpen();

  exec();
}

}}
