#include "common/common_pch.h"

#include <QLibraryInfo>
#include <QLocalServer>
#include <QLocalSocket>
#include <QLockFile>

#include <boost/optional.hpp>

#include "common/command_line.h"
#include "common/extern_data.h"
#include "common/fs_sys_helpers.h"
#include "common/iso639.h"
#include "common/qt.h"
#include "common/translation.h"
#include "common/unique_numbers.h"
#include "common/version.h"
#include "mkvtoolnix-gui/app.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/util/util.h"
#include "mkvtoolnix-gui/util/settings.h"

namespace mtx { namespace gui {

static Iso639LanguageList s_iso639Languages, s_commonIso639Languages;
static Iso3166CountryList s_iso3166_1Alpha2Countries, s_commonIso3166_1Alpha2Countries;
static CharacterSetList s_characterSets, s_commonCharacterSets;
static QHash<QString, QString> s_iso639_2LanguageCodeToDescription, s_iso3166_1Alpha2CountryCodeToDescription;

static boost::optional<bool> s_is_installed;

App::App(int &argc,
         char **argv)
  : QApplication{argc, argv}
{
  mtx_common_init("mkvtoolnix-gui", argv[0]);
  version_info = get_version_info("mkvtoolnix-gui", vif_full);

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

  QObject::connect(this, &App::aboutToQuit, this, &App::saveSettings);

  initializeLocale();
}

App::~App() {
}

QString
App::communicatorSocketName() {
  return Q("MKVToolNix-GUI-Instance-Communicator");
}

void
App::setupInstanceCommunicator() {
  auto socketName   = communicatorSocketName();
  auto lockFilePath = QDir{QDir::tempPath()}.filePath(Q("%1.lock").arg(socketName));
  m_instanceLock    = std::make_unique<QLockFile>(lockFilePath);

  m_instanceLock->setStaleLockTime(0);
  if (!m_instanceLock->tryLock(0)) {
    m_instanceLock.reset(nullptr);
    return;
  }

  QLocalServer::removeServer(socketName);

  m_instanceCommunicator = std::make_unique<QLocalServer>(this);

  if (m_instanceCommunicator->listen(socketName))
    connect(m_instanceCommunicator.get(), &QLocalServer::newConnection, this, &App::receiveInstanceCommunication);

  else
    m_instanceCommunicator.reset(nullptr);
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

void
App::retranslateUi() {
  if (MainWindow::get())
    MainWindow::get()->retranslateUi();
}

void
App::reinitializeLanguageLists() {
  s_iso639Languages.clear();
  s_commonIso639Languages.clear();
  s_iso3166_1Alpha2Countries.clear();
  s_commonIso3166_1Alpha2Countries.clear();
  s_iso639_2LanguageCodeToDescription.clear();
  s_iso3166_1Alpha2CountryCodeToDescription.clear();
  s_characterSets.clear();
  s_commonCharacterSets.clear();

  initializeLanguageLists();
}

void
App::initializeLanguageLists() {
  if (!s_iso639Languages.empty())
    return;

  initializeIso639Languages();
  initializeIso3166_1Alpha2Countries();
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
App::initializeIso3166_1Alpha2Countries() {
  auto &cfg = Util::Settings::get();

  s_iso3166_1Alpha2Countries.reserve(g_cctlds.size());
  s_commonIso3166_1Alpha2Countries.reserve(g_cctlds.size());

  for (auto const &country : g_cctlds) {
    auto countryCode = Q(country.code);
    auto description = Q("%1 (%2)").arg(Q(country.country)).arg(countryCode);
    auto isCommon    = cfg.m_oftenUsedCountries.indexOf(countryCode) != -1;

    s_iso3166_1Alpha2Countries.emplace_back(description, countryCode);
    if (isCommon)
      s_commonIso3166_1Alpha2Countries.emplace_back(description, countryCode);

    s_iso3166_1Alpha2CountryCodeToDescription[countryCode] = description;
  }

  brng::sort(s_iso3166_1Alpha2Countries);
  brng::sort(s_commonIso3166_1Alpha2Countries);
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

Iso3166CountryList const &
App::iso3166_1Alpha2Countries() {
  initializeLanguageLists();
  return s_iso3166_1Alpha2Countries;
}

Iso3166CountryList const &
App::commonIso3166_1Alpha2Countries() {
  initializeLanguageLists();
  return s_commonIso3166_1Alpha2Countries;
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
App::descriptionFromIso3166_1Alpha2CountryCode(QString const &code) {
  initializeLanguageLists();
  return s_iso3166_1Alpha2CountryCodeToDescription.contains(code) ? s_iso3166_1Alpha2CountryCodeToDescription[code] : code;
}

bool
App::isInstalled() {
  if (!s_is_installed)
    s_is_installed.reset(mtx::sys::is_installed());
  return *s_is_installed;
}

void
App::initializeLocale(QString const &requestedLocale) {
  auto locale = to_utf8(requestedLocale);

#if defined(HAVE_LIBINTL_H)
  auto &cfg = Util::Settings::get();

  if (m_currentTranslator)
    removeTranslator(m_currentTranslator.get());
  m_currentTranslator.reset();

  translation_c::initialize_available_translations();

  if (locale.empty())
    locale = to_utf8(cfg.m_uiLocale);

  if (-1 == translation_c::look_up_translation(locale))
    locale = "";

  if (locale.empty()) {
    locale = boost::regex_replace(translation_c::get_default_ui_locale(), boost::regex{"\\..*", boost::regex::perl}, "");
    if (-1 == translation_c::look_up_translation(locale))
      locale = "";
  }

  if (!locale.empty()) {
    auto translator = std::make_unique<QTranslator>();
    auto paths      = QStringList{} << Q("%1/locale/%2/LC_MESSAGES").arg(applicationDirPath()).arg(Q(locale))
                                    << QLibraryInfo::location(QLibraryInfo::TranslationsPath);

    for (auto const &path : paths)
      if (translator->load(Q("qtbase_%1").arg(Q(locale)), path))
        break;

    installTranslator(translator.get());

    m_currentTranslator = std::move(translator);
    cfg.m_uiLocale      = Q(locale);
  }

#endif  // HAVE_LIBINTL_H

  init_locales(locale);

  retranslateUi();
}

bool
App::isOtherInstanceRunning()
  const {
  return !m_instanceLock;
}

void
App::sendCommandLineArgumentsToRunningInstance() {
  auto args = m_cliParser->rebuildCommandLine();
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
  if (args.isEmpty()) {
    raiseAndActivateRunningInstance();
    return true;
  }

  m_cliParser.reset(new GuiCliParser{Util::toStdStringVector(args, 1)});
  m_cliParser->run();

  if (m_cliParser->exitAfterParsing())
    return false;

  if (!isOtherInstanceRunning())
    return true;

  raiseAndActivateRunningInstance();
  sendCommandLineArgumentsToRunningInstance();

  return false;
}

void
App::handleCommandLineArgumentsLocally() {
  if (!m_cliParser->m_configFiles.isEmpty())
    emit openConfigFilesRequested(m_cliParser->m_configFiles);

  if (!m_cliParser->m_addToMerge.isEmpty())
    emit addingFilesToMergeRequested(m_cliParser->m_addToMerge);

  if (!m_cliParser->m_editChapters.isEmpty())
    emit editingChaptersRequested(m_cliParser->m_editChapters);

  if (!m_cliParser->m_editHeaders.isEmpty())
    emit editingHeadersRequested(m_cliParser->m_editHeaders);
}

void
App::receiveInstanceCommunication() {
  auto socket = m_instanceCommunicator->nextPendingConnection();
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

  m_cliParser.reset(new GuiCliParser{Util::toStdStringVector(args)});
  m_cliParser->run();
  handleCommandLineArgumentsLocally();
}

QString
App::settingsBaseGroupName() {
  return Q("MKVToolNix GUI Settings");
}

}}
