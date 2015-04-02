#ifndef MTX_MKVTOOLNIX_GUI_MAIN_WINDOW_MAIN_WINDOW_H
#define MTX_MKVTOOLNIX_GUI_MAIN_WINDOW_MAIN_WINDOW_H

#include "common/common_pch.h"

#if defined(HAVE_CURL_EASY_H)
# include "mkvtoolnix-gui/main_window/update_check_thread.h"
#endif  // HAVE_CURL_EASY_H

#include <QAction>
#include <QMainWindow>

namespace Ui {
class MainWindow;
}

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
}}}

class StatusBarProgressWidget;

class MainWindow : public QMainWindow {
  Q_OBJECT;

protected:
  // UI stuff:
  std::unique_ptr<Ui::MainWindow> ui;
  StatusBarProgressWidget *m_statusBarProgress{};
  mtx::gui::Merge::Tool *m_toolMerge{};
  mtx::gui::Jobs::Tool *m_toolJobs{};
  mtx::gui::HeaderEditor::Tool *m_toolHeaderEditor{};
  mtx::gui::WatchJobs::Tool *m_watchJobTool{};

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
  virtual void toolChanged(int index);
  virtual void editPreferences();

#if defined(HAVE_CURL_EASY_H)
  virtual void updateCheckFinished(UpdateCheckStatus status, mtx_release_version_t release);
  virtual void checkForUpdates();
#endif  // HAVE_CURL_EASY_H

public:                         // static
  static MainWindow *get();
  static mtx::gui::Merge::Tool *getMergeTool();
  static mtx::gui::HeaderEditor::Tool *getHeaderEditorTool();
  static mtx::gui::Jobs::Tool *getJobTool();
  static mtx::gui::WatchJobs::Tab *getWatchCurrentJobTab();
  static mtx::gui::WatchJobs::Tool *getWatchJobTool();
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

#endif // MTX_MKVTOOLNIX_GUI_MAIN_WINDOW_MAIN_WINDOW_H
