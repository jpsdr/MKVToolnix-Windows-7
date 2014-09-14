#include "common/common_pch.h"

#include "common/qt.h"
#include "common/version.h"
#include "mkvtoolnix-gui/forms/main_window.h"
#include "mkvtoolnix-gui/job_widget/job_widget.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/main_window/status_bar_progress_widget.h"
#include "mkvtoolnix-gui/merge_widget/merge_widget.h"
#include "mkvtoolnix-gui/util/settings.h"
#include "mkvtoolnix-gui/util/util.h"
#include "mkvtoolnix-gui/watch_job_container_widget/watch_job_container_widget.h"

#include <QCloseEvent>
#include <QIcon>
#include <QLabel>
#include <QStaticText>
#include <QVBoxLayout>

MainWindow *MainWindow::ms_mainWindow = nullptr;

MainWindow::MainWindow(QWidget *parent)
  : QMainWindow{parent}
  , ui{new Ui::MainWindow}
  , m_toolMerge{}
  , m_toolJobs{}
{
  ms_mainWindow = this;

  // Setup UI controls.
  ui->setupUi(this);

  m_statusBarProgress = new StatusBarProgressWidget{this};
  ui->statusBar->addPermanentWidget(m_statusBarProgress);

  setupToolSelector();

  // Setup window properties.
  setWindowIcon(Util::loadIcon(Q("mkvmergeGUI.png"), QList<int>{} << 32 << 48 << 64 << 128 << 256));

  retranslateUI();
}

MainWindow::~MainWindow() {
}

void
MainWindow::setStatusBarMessage(QString const &message) {
  ui->statusBar->showMessage(message, 3000);
}

Ui::MainWindow *
MainWindow::getUi() {
  return ui.get();
}

QWidget *
MainWindow::createNotImplementedWidget() {
  auto widget   = new QWidget{ui->tool};
  auto vlayout  = new QVBoxLayout{widget};
  auto hlayout  = new QHBoxLayout;
  auto text     = new QLabel{widget};

  text->setText(QY("This has not been implemented yet."));

  hlayout->addItem(new QSpacerItem{1, 1, QSizePolicy::Expanding, QSizePolicy::Minimum});
  hlayout->addWidget(text);
  hlayout->addItem(new QSpacerItem{1, 1, QSizePolicy::Expanding, QSizePolicy::Minimum});

  vlayout->addItem(new QSpacerItem{1, 1, QSizePolicy::Minimum,   QSizePolicy::Expanding});
  vlayout->addItem(hlayout);
  vlayout->addItem(new QSpacerItem{1, 1, QSizePolicy::Minimum,   QSizePolicy::Expanding});

  return widget;
}

void
MainWindow::setupToolSelector() {
  // ui->tool->setIconSize(QSize{48, 48});

  m_toolMerge = new MergeWidget{ui->tool};
  m_toolJobs  = new JobWidget{ui->tool};
  m_watchJobContainer = new WatchJobContainerWidget{ui->tool};

  auto numTabs = 0u;
  ui->tool->insertTab(numTabs++, m_toolMerge,                  QIcon{":/icons/48x48/merge.png"},                      QY("merge"));
  ui->tool->insertTab(numTabs++, createNotImplementedWidget(), QIcon{":/icons/48x48/split.png"},                      QY("extract"));
  ui->tool->insertTab(numTabs++, createNotImplementedWidget(), QIcon{":/icons/48x48/document-preview-archive.png"},   QY("info"));
  ui->tool->insertTab(numTabs++, createNotImplementedWidget(), QIcon{":/icons/48x48/document-edit.png"},              QY("edit headers"));
  ui->tool->insertTab(numTabs++, createNotImplementedWidget(), QIcon{":/icons/48x48/story-editor.png"},               QY("edit chapters"));
  ui->tool->insertTab(numTabs++, createNotImplementedWidget(), QIcon{":/icons/48x48/document-edit-sign-encrypt.png"}, QY("edit tags"));
  ui->tool->insertTab(numTabs++, m_toolJobs,                   QIcon{":/icons/48x48/view-task.png"},                  QY("job queue"));
  ui->tool->insertTab(numTabs++, m_watchJobContainer,          QIcon{":/icons/48x48/system-run.png"},                 QY("job output"));

  for (auto idx = 0u; idx < numTabs; ++idx)
    ui->tool->setTabEnabled(idx, true);

  ui->tool->setCurrentIndex(0);

  connect(m_toolJobs->getModel(), SIGNAL(progressChanged(unsigned int,unsigned int)), m_statusBarProgress, SLOT(setProgress(unsigned int,unsigned int)));
}

MainWindow *
MainWindow::get() {
  return ms_mainWindow;
}

MergeWidget *
MainWindow::getMergeWidget() {
  return get()->m_toolMerge;
}

JobWidget *
MainWindow::getJobWidget() {
  return get()->m_toolJobs;
}

WatchJobWidget *
MainWindow::getWatchCurrentJobWidget() {
  return getWatchJobContainerWidget()->currentJobWidget();
}

WatchJobContainerWidget *
MainWindow::getWatchJobContainerWidget() {
  return get()->m_watchJobContainer;
}

void
MainWindow::retranslateUI() {
  setWindowTitle(Q(get_version_info("MKVToolNix GUI")));
}

void
MainWindow::closeEvent(QCloseEvent *event) {
  QSettings reg;

  auto jobWidget = getJobWidget();
  if (jobWidget)
    jobWidget->getModel()->saveJobs(reg);

  event->accept();
}
