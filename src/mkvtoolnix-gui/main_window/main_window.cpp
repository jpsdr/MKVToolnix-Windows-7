#include "common/common_pch.h"

#include "common/fs_sys_helpers.h"
#include "common/qt.h"
#include "common/version.h"
#include "mkvtoolnix-gui/forms/main_window.h"
#include "mkvtoolnix-gui/header_editor/tool.h"
#include "mkvtoolnix-gui/jobs/tool.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/main_window/status_bar_progress_widget.h"
#include "mkvtoolnix-gui/merge/tool.h"
#include "mkvtoolnix-gui/util/settings.h"
#include "mkvtoolnix-gui/util/util.h"
#include "mkvtoolnix-gui/watch_jobs/tool.h"

#if defined(HAVE_CURL_EASY_H)
# include "mkvtoolnix-gui/main_window/available_update_info_dialog.h"
#endif  // HAVE_CURL_EASY_H

#include <QCloseEvent>
#include <QIcon>
#include <QLabel>
#include <QStaticText>
#include <QVBoxLayout>

MainWindow *MainWindow::ms_mainWindow = nullptr;

MainWindow::MainWindow(QWidget *parent)
  : QMainWindow{parent}
  , ui{new Ui::MainWindow}
{
  ms_mainWindow = this;

  // Setup UI controls.
  ui->setupUi(this);

  m_statusBarProgress = new StatusBarProgressWidget{this};
  ui->statusBar->addPermanentWidget(m_statusBarProgress);

  setupMenu();
  setupToolSelector();

  // Setup window properties.
  setWindowIcon(Util::loadIcon(Q("mkvmergeGUI.png"), QList<int>{} << 32 << 48 << 64 << 128 << 256));

  retranslateUi();

#if defined(HAVE_CURL_EASY_H)
  silentlyCheckForUpdates();
#endif  // HAVE_CURL_EASY_H
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
MainWindow::setupMenu() {
  connect(ui->actionGUIExit,            SIGNAL(triggered()), this, SLOT(close()));
  connect(ui->actionGUIPreferences,     SIGNAL(triggered()), this, SLOT(editPreferences()));

#if defined(HAVE_CURL_EASY_H)
  connect(ui->actionGUICheckForUpdates, SIGNAL(triggered()), this, SLOT(checkForUpdates()));
#else
  ui->actionGUICheckForUpdates->setVisible(false);
#endif  // HAVE_CURL_EASY_H
}

void
MainWindow::setupToolSelector() {
  // ui->tool->setIconSize(QSize{48, 48});

  m_toolMerge         = new mtx::gui::Merge::Tool{ui->tool, ui->menuMerge};
  m_toolJobs          = new mtx::gui::Jobs::Tool{ui->tool};
  m_toolHeaderEditor  = new mtx::gui::HeaderEditor::Tool{ui->tool, ui->menuHeaderEditor};
  m_watchJobTool      = new mtx::gui::WatchJobs::Tool{ui->tool};

  ui->tool->appendTab(m_toolMerge,                  QIcon{":/icons/48x48/merge.png"},                      QY("merge"));
  ui->tool->appendTab(createNotImplementedWidget(), QIcon{":/icons/48x48/split.png"},                      QY("extract"));
  ui->tool->appendTab(createNotImplementedWidget(), QIcon{":/icons/48x48/document-preview-archive.png"},   QY("info"));
  ui->tool->appendTab(m_toolHeaderEditor,           QIcon{":/icons/48x48/document-edit.png"},              QY("edit headers"));
  ui->tool->appendTab(createNotImplementedWidget(), QIcon{":/icons/48x48/story-editor.png"},               QY("edit chapters"));
  ui->tool->appendTab(createNotImplementedWidget(), QIcon{":/icons/48x48/document-edit-sign-encrypt.png"}, QY("edit tags"));
  ui->tool->appendTab(m_toolJobs,                   QIcon{":/icons/48x48/view-task.png"},                  QY("job queue"));
  ui->tool->appendTab(m_watchJobTool,               QIcon{":/icons/48x48/system-run.png"},                 QY("job output"));

  for (auto idx = 0, numTabs = ui->tool->count(); idx < numTabs; ++idx)
    ui->tool->setTabEnabled(idx, true);

  ui->tool->setCurrentIndex(0);
  m_toolMerge->toolShown();

  connect(ui->tool,               SIGNAL(currentChanged(int)),                        this,                SLOT(toolChanged(int)));
  connect(m_toolJobs->getModel(), SIGNAL(progressChanged(unsigned int,unsigned int)), m_statusBarProgress, SLOT(setProgress(unsigned int,unsigned int)));
}

void
MainWindow::showAndEnableMenu(QMenu &menu,
                              bool show) {
  auto &action = *menu.menuAction();
  action.setVisible(show);
  action.setEnabled(show);
  for (auto const &action : menu.actions())
    action->setEnabled(show);
}

void
MainWindow::showTheseMenusOnly(QList<QMenu *> const &menus) {
  showAndEnableMenu(*ui->menuMerge,        menus.contains(ui->menuMerge));
  showAndEnableMenu(*ui->menuHeaderEditor, menus.contains(ui->menuHeaderEditor));
}

void
MainWindow::toolChanged(int index) {
  auto widget   = ui->tool->widget(index);
  auto toolBase = dynamic_cast<ToolBase *>(widget);

  if (toolBase)
    toolBase->toolShown();
}

MainWindow *
MainWindow::get() {
  return ms_mainWindow;
}

mtx::gui::Merge::Tool *
MainWindow::getMergeTool() {
  return get()->m_toolMerge;
}

mtx::gui::HeaderEditor::Tool *
MainWindow::getHeaderEditorTool() {
  return get()->m_toolHeaderEditor;
}

mtx::gui::Jobs::Tool *
MainWindow::getJobTool() {
  return get()->m_toolJobs;
}

mtx::gui::WatchJobs::Tab *
MainWindow::getWatchCurrentJobTab() {
  return getWatchJobTool()->currentJobTab();
}

mtx::gui::WatchJobs::Tool *
MainWindow::getWatchJobTool() {
  return get()->m_watchJobTool;
}

void
MainWindow::retranslateUi() {
  setWindowTitle(Q(get_version_info("MKVToolNix GUI")));

  for (auto idx = 0, count = ui->tool->count(); idx < count; ++idx) {
    auto toolBase = dynamic_cast<ToolBase *>(ui->tool->widget(idx));
    if (toolBase)
      toolBase->retranslateUi();
  }
}

void
MainWindow::closeEvent(QCloseEvent *event) {
  QSettings reg;

  auto jobTool = getJobTool();
  if (jobTool)
    jobTool->getModel()->saveJobs(reg);

  event->accept();
}

void
MainWindow::editPreferences() {
  // TODO: MainWindow::editPreferences
  setStatusBarMessage(Q("TODO: MainWindow::editPreferences"));
}

#if defined(HAVE_CURL_EASY_H)
void
MainWindow::checkForUpdates() {
  AvailableUpdateInfoDialog dlg{this};
  dlg.exec();
}

void
MainWindow::silentlyCheckForUpdates() {
  auto forceUpdateCheck = get_environment_variable("FORCE_UPDATE_CHECK") == "1";

  if (!forceUpdateCheck && !Settings::get().m_checkForUpdates)
    return;

  auto lastCheck = Settings::get().m_lastUpdateCheck;
  if (!forceUpdateCheck && lastCheck.isValid() && (lastCheck.addDays(1) >= QDateTime::currentDateTime()))
    return;

  auto thread = new UpdateCheckThread(this);

  connect(thread, SIGNAL(checkFinished(UpdateCheckStatus, mtx_release_version_t)), this, SLOT(updateCheckFinished(UpdateCheckStatus, mtx_release_version_t)));

  thread->start();
}

QString
MainWindow::versionStringForSettings(version_number_t const &version) {
  return Q("version_%1").arg(to_qs(boost::regex_replace(version.to_string(), boost::regex("[^\\d]+", boost::regex::perl), "_")));
}

void
MainWindow::updateCheckFinished(UpdateCheckStatus status,
                                mtx_release_version_t release) {
  if ((status == UpdateCheckStatus::Failed) || !release.valid)
    return;

  auto &settings             = Settings::get();
  settings.m_lastUpdateCheck = QDateTime::currentDateTime();
  auto forceUpdateCheck      = get_environment_variable("FORCE_UPDATE_CHECK") == "1";

  if (!forceUpdateCheck && !(release.current_version < release.latest_source))
    return;

  auto settingsVersionString = versionStringForSettings(release.latest_source);
  auto wasVersionDisplayed   = settings.value(Q("settings/updates"), settingsVersionString, false).toBool();

  if (!forceUpdateCheck && wasVersionDisplayed)
    return;

  settings.setValue(Q("settings/updates"), settingsVersionString, true);

  AvailableUpdateInfoDialog dlg{this};
  dlg.exec();
}
#endif  // HAVE_CURL_EASY_H
