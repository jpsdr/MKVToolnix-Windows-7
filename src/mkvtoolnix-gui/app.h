#pragma once

#include "common/common_pch.h"

#include <QApplication>
#include <QDateTime>
#include <QStringList>

#include "mkvtoolnix-gui/gui_cli_parser.h"

class QEvent;
class QLocalServer;
class QThread;

namespace mtx::gui {

namespace Util {
class MediaPlayer;
class NetworkAccessManager;
}

namespace Jobs {
class ProgramRunner;
}

using Iso639Language     = std::pair<QString, QString>;
using Iso639LanguageList = std::vector<Iso639Language>;
using Region             = std::pair<QString, QString>;
using RegionList         = std::vector<Region>;
using CharacterSetList   = std::vector<QString>;

class AppPrivate;
class App : public QApplication {
  Q_OBJECT

protected:
  MTX_DECLARE_PRIVATE(AppPrivate)

  std::unique_ptr<AppPrivate> const p_ptr;

  explicit App(AppPrivate &p);


protected:

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

  QDateTime appStartTimestamp();

#if defined(SYS_APPLE)
  virtual bool event(QEvent *event) override;
#endif

Q_SIGNALS:
  void addingFilesToMergeRequested(QStringList const &fileNames);
  void editingChaptersRequested(QStringList const &fileNames);
  void editingHeadersRequested(QStringList const &fileNames);
  void openConfigFilesRequested(QStringList const &fileNames);
  void runningInfoOnRequested(QStringList const &fileNames);
  void toolRequested(ToolBase *tool);
  void systemColorSchemeChanged();

public Q_SLOTS:
  void saveSettings() const;
  void receiveInstanceCommunication();
  void setupAppearance();
#if defined(SYS_WINDOWS)
  void setupPalette();
#endif
  void setupUiFont();

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
  static Iso639LanguageList const &iso639_2Languages();
  static Iso639LanguageList const &commonIso639Languages();
  static RegionList const &regions();
  static RegionList const &commonRegions();
  static QString const &descriptionForRegion(QString const &code);
  static CharacterSetList const &characterSets();
  static CharacterSetList const &commonCharacterSets();

  static void reinitializeLanguageLists();
  static void initializeLanguageLists();
  static void initializeRegions();
  static void initializeIso639Languages();
  static void initializeCharacterSets();

  static bool isInstalled();
#if defined(SYS_WINDOWS)
  static bool isWindows11OrLater();
  static QPalette systemPalette(bool light);
#endif

  static QString communicatorSocketName();
  static QString settingsBaseGroupName();

  static void fixLockFileHostName(QString const &lockFilePath);

  static void registerOriginalCLIArgs(int argc, char **argv);
  static QStringList const &originalCLIArgs();
};

}
