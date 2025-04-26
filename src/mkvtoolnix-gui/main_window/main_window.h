#pragma once

#include "common/common_pch.h"

#include <QMainWindow>
#include <QMediaPlayer>

#include "common/qt.h"
#include "mkvtoolnix-gui/main_window/preferences_dialog.h"
#include "mkvtoolnix-gui/main_window/update_checker.h"
#include "mkvtoolnix-gui/util/installation_checker.h"

class QEvent;
class QResizeEvent;
class QShowEvent;

namespace mtx::gui {

class ToolBase;

namespace ChapterEditor {
class Tool;
}
namespace HeaderEditor {
class Tool;
}
namespace Info {
class Tool;
}
namespace Jobs {
class Tool;
enum class QueueStatus;
}
namespace Merge {
class Tool;
}
namespace Ui {
class MainWindow;
}
namespace Util {
class LanguageDialog;
}
namespace WatchJobs {
class Tab;
class Tool;
}

class MainWindowPrivate;
class MainWindow : public QMainWindow {
  Q_OBJECT

protected:
  MTX_DECLARE_PRIVATE(MainWindowPrivate)

  std::unique_ptr<MainWindowPrivate> const p_ptr;

public:
  explicit MainWindow(QWidget *parent = nullptr);
  virtual ~MainWindow();

  virtual void setStatusBarMessage(QString const &message);

  virtual void showTheseMenusOnly(QList<QMenu *> const &menus);
  virtual void showAndEnableMenu(QMenu &menu, bool show);
  virtual void retranslateUi();

  virtual void switchToTool(ToolBase *tool);

  virtual bool eventFilter(QObject *watched, QEvent *event) override;

  virtual void editPreferencesAndShowPage(PreferencesDialog::Page page);

  virtual void registerSubWindowWidget(ToolBase &toolBase, QTabWidget &tabWidget);

Q_SIGNALS:
  void windowShown();
  void preferencesChanged();
  void aboutToClose();

public Q_SLOTS:
  virtual void changeToolToSender();
  virtual void toolChanged(int index);
  virtual void editPreferences();
  virtual void editRunProgramConfigurations();
  virtual void visitHelpURL();
  virtual void visitMkvmergeDocumentation();
  virtual void showCodeOfConduct();
  virtual void showSystemInformation();
  virtual void setToolSelectorVisibility();
  virtual void setStayOnTopStatus();
  virtual void showOrHideDebuggingMenu();
  virtual void raiseAndActivate();

#if defined(HAVE_UPDATE_CHECK)
  virtual void updateCheckFinished(UpdateCheckStatus status, mtx_release_version_t release);
  virtual void checkForUpdates();
#endif  // HAVE_UPDATE_CHECK

  virtual void handleMediaPlaybackError(QMediaPlayer::Error error, QString const &fileName);

  virtual void displayInstallationProblems(Util::InstallationChecker::Problems const &problems);

  virtual void setupWindowMenu();
  virtual void showNextOrPreviousSubWindow(int delta);
  virtual void showSubWindow(unsigned int tabIdx);
  virtual void startStopQueueSpinnerForQueue(Jobs::QueueStatus status);
  virtual void inhibitSleepWhileQueueIsRunning(Jobs::QueueStatus status);
  virtual void startStopQueueSpinner(bool start);
  virtual void startQueueSpinner();
  virtual void stopQueueSpinner();

  virtual void forceClose();

public:                         // static
  static MainWindow *get();
  static Ui::MainWindow *getUi();
  static Merge::Tool *mergeTool();
  static Info::Tool *infoTool();
  static HeaderEditor::Tool *headerEditorTool();
  static ChapterEditor::Tool *chapterEditorTool();
  static Jobs::Tool *jobTool();
  static WatchJobs::Tab *watchCurrentJobTab();
  static WatchJobs::Tool *watchJobTool();
  static Util::LanguageDialog &languageDialog();
#if defined(HAVE_UPDATE_CHECK)
  static QString versionStringForSettings(version_number_t const &version);
#endif  // HAVE_UPDATE_CHECK

  static QIcon yesIcon();
  static QIcon noIcon();

protected:
  virtual void setupConnections();
  virtual void setupToolSelector();
  virtual void setupHelpURLs();
  virtual void setupAuxiliaryWidgets();
  virtual void setupDebuggingMenu();
  virtual QWidget *createNotImplementedWidget();
  Util::LanguageDialog &setupLanguageDialog();

  virtual void resizeToDefaultSize();

  virtual void showEvent(QShowEvent *event);
  virtual void closeEvent(QCloseEvent *event);
  virtual bool beforeCloseCheckRunningJobs();

  virtual std::optional<bool> filterWheelEventForStrongFocus(QObject *watched, QEvent *event);

#if defined(HAVE_UPDATE_CHECK)
  virtual void silentlyCheckForUpdates();
#endif  // HAVE_UPDATE_CHECK
  virtual void runCacheCleanupOncePerVersion() const;

  virtual std::pair<ToolBase *, QTabWidget *> currentSubWindowWidget();
};

}
