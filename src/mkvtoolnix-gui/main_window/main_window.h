#pragma once

#include "common/common_pch.h"

#include <QMainWindow>

#include "mkvtoolnix-gui/main_window/preferences_dialog.h"
#include "mkvtoolnix-gui/main_window/update_checker.h"
#include "mkvtoolnix-gui/util/installation_checker.h"

class QEvent;
class QResizeEvent;
class QShowEvent;

namespace mtx { namespace gui {

class ToolBase;

namespace ChapterEditor {
class Tool;
}
namespace HeaderEditor {
class Tool;
}
namespace Jobs {
class Tool;
}
namespace Merge {
class Tool;
}
namespace Ui {
class MainWindow;
}
namespace Util {
class MovingPixmapOverlay;
}
namespace WatchJobs {
class Tab;
class Tool;
}

class MainWindowPrivate;
class MainWindow : public QMainWindow {
  Q_OBJECT;

protected:
  Q_DECLARE_PRIVATE(MainWindow);

  QScopedPointer<MainWindowPrivate> const d_ptr;

  explicit MainWindow(MainWindowPrivate &d, QWidget *parent);

public:
  explicit MainWindow(QWidget *parent = nullptr);
  virtual ~MainWindow();

  virtual void setStatusBarMessage(QString const &message);

  virtual void showTheseMenusOnly(QList<QMenu *> const &menus);
  virtual void showAndEnableMenu(QMenu &menu, bool show);
  virtual void retranslateUi();

  virtual void showIconMovingToTool(QString const &pixmapName, ToolBase const &tool);

  virtual void resizeEvent(QResizeEvent *event) override;

  virtual void switchToTool(ToolBase *tool);

  virtual bool eventFilter(QObject *watched, QEvent *event) override;

  virtual void editPreferencesAndShowPage(PreferencesDialog::Page page);

  virtual void registerSubWindowWidget(ToolBase &toolBase, QTabWidget &tabWidget);

signals:
  void windowShown();
  void preferencesChanged();
  void aboutToClose();

public slots:
  virtual void changeToolToSender();
  virtual void toolChanged(int index);
  virtual void editPreferences();
  virtual void editRunProgramConfigurations();
  virtual void visitHelpURL();
  virtual void visitMkvmergeDocumentation();
  virtual void setToolSelectorVisibility();
  virtual void raiseAndActivate();

#if defined(HAVE_UPDATE_CHECK)
  virtual void updateCheckFinished(UpdateCheckStatus status, mtx_release_version_t release);
  virtual void checkForUpdates();
#endif  // HAVE_UPDATE_CHECK

  virtual void displayInstallationProblems(Util::InstallationChecker::Problems const &problems);

  virtual void setupWindowMenu();
  virtual void showNextOrPreviousSubWindow(int delta);
  virtual void showSubWindow(unsigned int tabIdx);

public:                         // static
  static MainWindow *get();
  static Ui::MainWindow *getUi();
  static Merge::Tool *mergeTool();
  static HeaderEditor::Tool *headerEditorTool();
  static ChapterEditor::Tool *chapterEditorTool();
  static Jobs::Tool *jobTool();
  static WatchJobs::Tab *watchCurrentJobTab();
  static WatchJobs::Tool *watchJobTool();
#if defined(HAVE_UPDATE_CHECK)
  static QString versionStringForSettings(version_number_t const &version);
#endif  // HAVE_UPDATE_CHECK

  static QIcon const & yesIcon();
  static QIcon const & noIcon();

protected:
  virtual void setupMenu();
  virtual void setupToolSelector();
  virtual void setupHelpURLs();
  virtual QWidget *createNotImplementedWidget();

  virtual void showEvent(QShowEvent *event);
  virtual void closeEvent(QCloseEvent *event);
  virtual bool beforeCloseCheckRunningJobs();

  virtual boost::optional<bool> filterWheelEventForStrongFocus(QObject *watched, QEvent *event);

#if defined(HAVE_UPDATE_CHECK)
  virtual void silentlyCheckForUpdates();
#endif  // HAVE_UPDATE_CHECK
  virtual void runCacheCleanupOncePerVersion() const;

  virtual std::pair<ToolBase *, QTabWidget *> currentSubWindowWidget();
};

}}
