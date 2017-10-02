#pragma once

#include "common/common_pch.h"

#include <QApplication>
#include <QStringList>

#include "mkvtoolnix-gui/gui_cli_parser.h"

class QLocalServer;
class QThread;

namespace mtx { namespace gui {

namespace Util {
class MediaPlayer;
class NetworkAccessManager;
}

namespace Jobs {
class ProgramRunner;
}

using Iso639Language                = std::pair<QString, QString>;
using Iso639LanguageList            = std::vector<Iso639Language>;
using TopLevelDomainCountryCode     = std::pair<QString, QString>;
using TopLevelDomainCountryCodeList = std::vector<TopLevelDomainCountryCode>;
using CharacterSetList              = std::vector<QString>;

class AppPrivate;
class App : public QApplication {
  Q_OBJECT;

protected:
  Q_DECLARE_PRIVATE(App);

  QScopedPointer<AppPrivate> const d_ptr;

  explicit App(AppPrivate &d, QWidget *parent);

protected:
  enum class CliCommand {
    OpenConfigFiles,
    AddToMerge,
    EditChapters,
    EditHeaders,
  };

public:
  App(int &argc, char **argv);
  virtual ~App();

  void retranslateUi();
  void initializeLocale(QString const &requestedLocale = QString{});

  bool parseCommandLineArguments(QStringList const &args);
  void handleCommandLineArgumentsLocally();

  bool isOtherInstanceRunning() const;
  void sendCommandLineArgumentsToRunningInstance();
  void sendArgumentsToRunningInstance(QStringList const &args);
  void raiseAndActivateRunningInstance();

  void run();

  Util::NetworkAccessManager &networkAccessManager();

signals:
  void addingFilesToMergeRequested(QStringList const &fileNames);
  void editingChaptersRequested(QStringList const &fileNames);
  void editingHeadersRequested(QStringList const &fileNames);
  void openConfigFilesRequested(QStringList const &fileNames);

public slots:
  void saveSettings() const;
  void receiveInstanceCommunication();

protected:
  void setupInstanceCommunicator();
  void setupNetworkAccessManager();
  Util::MediaPlayer &setupMediaPlayer();
  Jobs::ProgramRunner &setupProgramRunner();

public:
  static App *instance();
  static Util::MediaPlayer &mediaPlayer();
  static Jobs::ProgramRunner &programRunner();

  static Iso639LanguageList const &iso639Languages();
  static Iso639LanguageList const &commonIso639Languages();
  static TopLevelDomainCountryCodeList const &topLevelDomainCountryCodes();
  static TopLevelDomainCountryCodeList const &commonTopLevelDomainCountryCodes();
  static QString const &descriptionFromIso639_2LanguageCode(QString const &code);
  static QString const &descriptionFromTopLevelDomainCountryCode(QString const &code);
  static CharacterSetList const &characterSets();
  static CharacterSetList const &commonCharacterSets();

  static void reinitializeLanguageLists();
  static void initializeLanguageLists();
  static void initializeTopLevelDomainCountryCodes();
  static void initializeIso639Languages();
  static void initializeCharacterSets();

  static bool isInstalled();

  static QString communicatorSocketName();
  static QString settingsBaseGroupName();

  static void fixLockFileHostName(QString const &lockFilePath);
  static void setupUiFont();
};

}}
