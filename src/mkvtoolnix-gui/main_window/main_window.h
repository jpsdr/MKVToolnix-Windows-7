#ifndef MTX_MKVTOOLNIX_GUI_MAIN_WINDOW_MAIN_WINDOW_H
#define MTX_MKVTOOLNIX_GUI_MAIN_WINDOW_MAIN_WINDOW_H

#include "common/common_pch.h"

#include <QAction>
#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class JobWidget;
class MergeWidget;
class StatusBarProgressWidget;
class WatchJobContainerWidget;
class WatchJobWidget;

class MainWindow : public QMainWindow {
  Q_OBJECT;

protected:
  // UI stuff:
  std::unique_ptr<Ui::MainWindow> ui;
  StatusBarProgressWidget *m_statusBarProgress;
  MergeWidget *m_toolMerge;
  JobWidget *m_toolJobs;
  WatchJobContainerWidget *m_watchJobContainer;

protected:                      // static
  static MainWindow *ms_mainWindow;

public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

  virtual void setStatusBarMessage(QString const &message);
  virtual Ui::MainWindow *getUi();

public:                         // static
  static MainWindow *get();
  static MergeWidget *getMergeWidget();
  static JobWidget *getJobWidget();
  static WatchJobWidget *getWatchCurrentJobWidget();
  static WatchJobContainerWidget *getWatchJobContainerWidget();

protected:
  virtual void setupToolSelector();
  virtual QWidget *createNotImplementedWidget();
  virtual void retranslateUI();

  virtual void closeEvent(QCloseEvent *event);
};

#endif // MTX_MKVTOOLNIX_GUI_MAIN_WINDOW_MAIN_WINDOW_H
