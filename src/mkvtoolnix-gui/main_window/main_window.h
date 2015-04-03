#ifndef MTX_MKVTOOLNIX_GUI_MAIN_WINDOW_MAIN_WINDOW_H
#define MTX_MKVTOOLNIX_GUI_MAIN_WINDOW_MAIN_WINDOW_H

#include "common/common_pch.h"

#if defined(HAVE_CURL_EASY_H)
# include "mkvtoolnix-gui/main_window/update_check_thread.h"
#endif  // HAVE_CURL_EASY_H

#include <QAction>
#include <QMainWindow>

namespace mtx { namespace gui {

namespace HeaderEditor {
class Tool;
}
namespace Jobs {
class Tool;
}
namespace Merge {
class Tool;
}
namespace WatchJobs {
class Tab;
class Tool;
}

namespace Ui {
class MainWindow;
}

class StatusBarProgressWidget;

class MainWindow : public QMainWindow {
  Q_OBJECT;

protected:
  // UI stuff:
  std::unique_ptr<Ui::MainWindow> ui;
  StatusBarProgressWidget *m_statusBarProgress{};
  Merge::Tool *m_toolMerge{};
  Jobs::Tool *m_toolJobs{};
  HeaderEditor::Tool *m_toolHeaderEditor{};
  WatchJobs::Tool *m_watchJobTool{};
  QList<QAction *> m_toolSelectionActions;

protected:                      // static
  static MainWindow *ms_mainWindow;

public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

  virtual void setStatusBarMessage(QString const &message);
  virtual Ui::MainWindow *getUi();

  virtual void showTheseMenusOnly(QList<QMenu *> const &menus);
  virtual void showAndEnableMenu(QMenu &menu, bool show);

public slots:
  virtual void changeTool();
  virtual void toolChanged(int index);
  virtual void editPreferences();

#if defined(HAVE_CURL_EASY_H)
  virtual void updateCheckFinished(UpdateCheckStatus status, mtx_release_version_t release);
  virtual void checkForUpdates();
#endif  // HAVE_CURL_EASY_H

public:                         // static
  static MainWindow *get();
  static Merge::Tool *getMergeTool();
  static HeaderEditor::Tool *getHeaderEditorTool();
  static Jobs::Tool *getJobTool();
  static WatchJobs::Tab *getWatchCurrentJobTab();
  static WatchJobs::Tool *getWatchJobTool();
#if defined(HAVE_CURL_EASY_H)
  static QString versionStringForSettings(version_number_t const &version);
#endif  // HAVE_CURL_EASY_H


protected:
  virtual void setupMenu();
  virtual void setupToolSelector();
  virtual QWidget *createNotImplementedWidget();
  virtual void retranslateUi();

  virtual void closeEvent(QCloseEvent *event);

#if defined(HAVE_CURL_EASY_H)
  virtual void silentlyCheckForUpdates();
#endif  // HAVE_CURL_EASY_H
};

}}

#endif // MTX_MKVTOOLNIX_GUI_MAIN_WINDOW_MAIN_WINDOW_H
