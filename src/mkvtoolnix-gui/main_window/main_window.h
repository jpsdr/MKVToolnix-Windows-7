#ifndef MTX_MKVTOOLNIX_GUI_MAIN_WINDOW_MAIN_WINDOW_H
#define MTX_MKVTOOLNIX_GUI_MAIN_WINDOW_MAIN_WINDOW_H

#include "common/common_pch.h"

#include <QAction>
#include <QMainWindow>

#include "mkvtoolnix-gui/main_window/update_checker.h"

class QEvent;
class QResizeEvent;
class QShowEvent;

namespace mtx { namespace gui {

class StatusBarProgressWidget;
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
namespace Util {
class MovingPixmapOverlay;
}
namespace WatchJobs {
class Tab;
class Tool;
}

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
  Q_OBJECT;

protected:
  // UI stuff:
  std::unique_ptr<Ui::MainWindow> ui;
  StatusBarProgressWidget *m_statusBarProgress{};
  Merge::Tool *m_toolMerge{};
  Jobs::Tool *m_toolJobs{};
  HeaderEditor::Tool *m_toolHeaderEditor{};
  ChapterEditor::Tool *m_toolChapterEditor{};
  WatchJobs::Tool *m_watchJobTool{};
  QList<QAction *> m_toolSelectionActions;
  std::unique_ptr<Util::MovingPixmapOverlay> m_movingPixmapOverlay;

  QHash<QObject *, QString> m_helpURLs;

protected:                      // static
  static MainWindow *ms_mainWindow;

public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

  virtual void setStatusBarMessage(QString const &message);

  virtual void showTheseMenusOnly(QList<QMenu *> const &menus);
  virtual void showAndEnableMenu(QMenu &menu, bool show);
  virtual void retranslateUi();

  virtual void showIconMovingToTool(QString const &pixmapName, ToolBase const &tool);

  virtual void resizeEvent(QResizeEvent *event) override;

  virtual void switchToTool(ToolBase *tool);

  virtual bool eventFilter(QObject *watched, QEvent *event) override;

signals:
  void windowShown();
  void preferencesChanged();
  void aboutToClose();

public slots:
  virtual void changeToolToSender();
  virtual void toolChanged(int index);
  virtual void editPreferences();
  virtual void visitHelpURL();
  virtual void visitMkvmergeDocumentation();
  virtual void setToolSelectorVisibility();
  virtual void raiseAndActivate();

  virtual void updateCheckFinished(UpdateCheckStatus status, mtx_release_version_t release);
  virtual void checkForUpdates();

public:                         // static
  static MainWindow *get();
  static Ui::MainWindow *getUi();
  static Merge::Tool *mergeTool();
  static HeaderEditor::Tool *headerEditorTool();
  static ChapterEditor::Tool *chapterEditorTool();
  static Jobs::Tool *jobTool();
  static WatchJobs::Tab *watchCurrentJobTab();
  static WatchJobs::Tool *watchJobTool();
  static QString versionStringForSettings(version_number_t const &version);

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

  virtual void silentlyCheckForUpdates();
};

}}

#endif // MTX_MKVTOOLNIX_GUI_MAIN_WINDOW_MAIN_WINDOW_H
