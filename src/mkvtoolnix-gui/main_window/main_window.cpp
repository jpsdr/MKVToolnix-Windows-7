#include "common/common_pch.h"

#include <QCloseEvent>
#include <QIcon>
#include <QLabel>
#include <QSettings>
#include <QStaticText>
#include <QVBoxLayout>

#include "common/fs_sys_helpers.h"
#include "common/qt.h"
#include "common/version.h"
#include "mkvtoolnix-gui/app.h"
#include "mkvtoolnix-gui/chapter_editor/tool.h"
#include "mkvtoolnix-gui/forms/main_window/main_window.h"
#include "mkvtoolnix-gui/header_editor/tool.h"
#include "mkvtoolnix-gui/jobs/tool.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/main_window/preferences_dialog.h"
#include "mkvtoolnix-gui/main_window/status_bar_progress_widget.h"
#include "mkvtoolnix-gui/merge/tool.h"
#include "mkvtoolnix-gui/util/settings.h"
#include "mkvtoolnix-gui/util/util.h"
#include "mkvtoolnix-gui/watch_jobs/tool.h"

#if defined(HAVE_CURL_EASY_H)
# include "mkvtoolnix-gui/main_window/available_update_info_dialog.h"
#endif  // HAVE_CURL_EASY_H

namespace mtx { namespace gui {

MainWindow *MainWindow::ms_mainWindow = nullptr;

MainWindow::MainWindow(QWidget *parent)
  : QMainWindow{parent}
  , ui{new Ui::MainWindow}
  , m_geometrySaver{this, Q("MainWindow")}
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

  m_geometrySaver.restore();

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

  m_toolMerge         = new Merge::Tool{ui->tool, ui->menuMerge};
  m_toolJobs          = new Jobs::Tool{ui->tool};
  m_toolHeaderEditor  = new HeaderEditor::Tool{ui->tool, ui->menuHeaderEditor};
  m_toolChapterEditor = new ChapterEditor::Tool{ui->tool, ui->menuChapterEditor};
  m_watchJobTool      = new WatchJobs::Tool{ui->tool};

  ui->tool->appendTab(m_toolMerge,                  QIcon{":/icons/48x48/merge.png"},                      QY("merge"));
  ui->tool->appendTab(createNotImplementedWidget(), QIcon{":/icons/48x48/split.png"},                      QY("extract"));
  ui->tool->appendTab(createNotImplementedWidget(), QIcon{":/icons/48x48/document-preview-archive.png"},   QY("info"));
  ui->tool->appendTab(m_toolHeaderEditor,           QIcon{":/icons/48x48/document-edit.png"},              QY("edit headers"));
  ui->tool->appendTab(m_toolChapterEditor,          QIcon{":/icons/48x48/story-editor.png"},               QY("edit chapters"));
  ui->tool->appendTab(createNotImplementedWidget(), QIcon{":/icons/48x48/document-edit-sign-encrypt.png"}, QY("edit tags"));
  ui->tool->appendTab(m_toolJobs,                   QIcon{":/icons/48x48/view-task.png"},                  QY("job queue"));
  ui->tool->appendTab(m_watchJobTool,               QIcon{":/icons/48x48/system-run.png"},                 QY("job output"));

  for (auto idx = 0, numTabs = ui->tool->count(); idx < numTabs; ++idx)
    ui->tool->setTabEnabled(idx, true);

  ui->tool->setCurrentIndex(0);
  m_toolMerge->toolShown();

  m_toolSelectionActions << ui->actionGUIMergeTool    << ui->actionGUIExtractionTool << ui->actionGUIInfoTool
                         << ui->actionGUIHeaderEditor << ui->actionGUIChapterEditor  << ui->actionGUITagEditor
                         << ui->actionGUIJobQueue     << ui->actionGUIJobOutput;

  connect(ui->actionGUIMergeTool,      SIGNAL(triggered()),                                this,                SLOT(changeTool()));
  connect(ui->actionGUIExtractionTool, SIGNAL(triggered()),                                this,                SLOT(changeTool()));
  connect(ui->actionGUIInfoTool,       SIGNAL(triggered()),                                this,                SLOT(changeTool()));
  connect(ui->actionGUIHeaderEditor,   SIGNAL(triggered()),                                this,                SLOT(changeTool()));
  connect(ui->actionGUIChapterEditor,  SIGNAL(triggered()),                                this,                SLOT(changeTool()));
  connect(ui->actionGUITagEditor,      SIGNAL(triggered()),                                this,                SLOT(changeTool()));
  connect(ui->actionGUIJobQueue,       SIGNAL(triggered()),                                this,                SLOT(changeTool()));
  connect(ui->actionGUIJobOutput,      SIGNAL(triggered()),                                this,                SLOT(changeTool()));

  connect(ui->tool,                    SIGNAL(currentChanged(int)),                        this,                SLOT(toolChanged(int)));
  connect(m_toolJobs->getModel(),      SIGNAL(progressChanged(unsigned int,unsigned int)), m_statusBarProgress, SLOT(setProgress(unsigned int,unsigned int)));
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
  showAndEnableMenu(*ui->menuMerge,         menus.contains(ui->menuMerge));
  showAndEnableMenu(*ui->menuHeaderEditor,  menus.contains(ui->menuHeaderEditor));
  showAndEnableMenu(*ui->menuChapterEditor, menus.contains(ui->menuChapterEditor));
}

void
MainWindow::changeTool() {
  auto toolIndex = m_toolSelectionActions.indexOf(static_cast<QAction *>(sender()));
  if (-1 != toolIndex)
    ui->tool->setCurrentIndex(toolIndex);
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

Merge::Tool *
MainWindow::getMergeTool() {
  return get()->m_toolMerge;
}

HeaderEditor::Tool *
MainWindow::getHeaderEditorTool() {
  return get()->m_toolHeaderEditor;
}

ChapterEditor::Tool *
MainWindow::getChapterEditorTool() {
  return get()->m_toolChapterEditor;
}

Jobs::Tool *
MainWindow::getJobTool() {
  return get()->m_toolJobs;
}

WatchJobs::Tab *
MainWindow::getWatchCurrentJobTab() {
  return getWatchJobTool()->currentJobTab();
}

WatchJobs::Tool *
MainWindow::getWatchJobTool() {
  return get()->m_watchJobTool;
}

void
MainWindow::retranslateUi() {
  ui->retranslateUi(this);
  m_statusBarProgress->retranslateUi();

  setWindowTitle(Q(get_version_info("MKVToolNix GUI")));

  ui->tool->setUpdatesEnabled(false);

  auto toolTitles = QStringList{} << QY("merge") << QY("extract") << QY("info") << QY("edit headers") << QY("edit chapters") << QY("edit tags") << QY("job queue") << QY("job output");

  for (auto idx = 0, count = ui->tool->count(); idx < count; ++idx) {
    ui->tool->setTabText(idx, toolTitles[idx]);
    auto toolBase = dynamic_cast<ToolBase *>(ui->tool->widget(idx));
    if (toolBase)
      toolBase->retranslateUi();
  }

  ui->tool->setUpdatesEnabled(true);
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
  PreferencesDialog dlg{this};
  if (!dlg.exec())
    return;

  dlg.save();

  if (dlg.uiLocaleChanged())
    App::instance()->initializeLocale();
}

#if defined(HAVE_CURL_EASY_H)
void
MainWindow::checkForUpdates() {
  AvailableUpdateInfoDialog dlg{this};
  dlg.exec();
}

void
MainWindow::silentlyCheckForUpdates() {
  auto forceUpdateCheck = mtx::sys::get_environment_variable("FORCE_UPDATE_CHECK") == "1";

  if (!forceUpdateCheck && !Util::Settings::get().m_checkForUpdates)
    return;

  auto lastCheck = Util::Settings::get().m_lastUpdateCheck;
  if (!forceUpdateCheck && lastCheck.isValid() && (lastCheck.addDays(1) >= QDateTime::currentDateTime()))
    return;

  auto thread = new UpdateCheckThread(this);

  connect(thread, SIGNAL(checkFinished(mtx::gui::UpdateCheckStatus, mtx_release_version_t)), this, SLOT(updateCheckFinished(mtx::gui::UpdateCheckStatus, mtx_release_version_t)));

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

  auto &settings             = Util::Settings::get();
  settings.m_lastUpdateCheck = QDateTime::currentDateTime();
  auto forceUpdateCheck      = mtx::sys::get_environment_variable("FORCE_UPDATE_CHECK") == "1";

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

}}
