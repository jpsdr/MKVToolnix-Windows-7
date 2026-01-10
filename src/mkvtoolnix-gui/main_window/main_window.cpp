#include "common/common_pch.h"

#include <QCloseEvent>
#include <QDesktopServices>
#include <QEvent>
#include <QIcon>
#include <QLabel>
#include <QMessageBox>
#include <QRegularExpression>
#include <QScreen>
#include <QStackedWidget>
#include <QStaticText>
#include <QVBoxLayout>
#include <QtConcurrent>

#include "common/common_urls.h"
#include "common/fs_sys_helpers.h"
#include "common/list_utils.h"
#include "common/locale_string.h"
#include "common/qt.h"
#include "common/version.h"
#include "mkvtoolnix-gui/app.h"
#include "mkvtoolnix-gui/chapter_editor/tool.h"
#include "mkvtoolnix-gui/forms/main_window/main_window.h"
#include "mkvtoolnix-gui/header_editor/tool.h"
#include "mkvtoolnix-gui/info/tool.h"
#include "mkvtoolnix-gui/jobs/tool.h"
#include "mkvtoolnix-gui/main_window/available_update_info_dialog.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/main_window/preferences_dialog.h"
#include "mkvtoolnix-gui/main_window/status_bar_progress_widget.h"
#include "mkvtoolnix-gui/main_window/taskbar_progress.h"
#include "mkvtoolnix-gui/merge/tool.h"
#include "mkvtoolnix-gui/util/cache.h"
#include "mkvtoolnix-gui/util/file_identifier.h"
#include "mkvtoolnix-gui/util/language_dialog.h"
#include "mkvtoolnix-gui/util/media_player.h"
#include "mkvtoolnix-gui/util/message_box.h"
#include "mkvtoolnix-gui/util/settings.h"
#include "mkvtoolnix-gui/util/sleep_inhibitor.h"
#include "mkvtoolnix-gui/util/system_information.h"
#include "mkvtoolnix-gui/util/text_display_dialog.h"
#include "mkvtoolnix-gui/util/waiting_spinner_widget.h"
#include "mkvtoolnix-gui/util/widget.h"
#include "mkvtoolnix-gui/watch_jobs/tab.h"
#include "mkvtoolnix-gui/watch_jobs/tool.h"

namespace mtx::gui {

MainWindow *s_mainWindow = nullptr;

class MainWindowPrivate {
  friend class MainWindow;

  std::unique_ptr<Ui::MainWindow> ui;
  StatusBarProgressWidget *statusBarProgress{};
  QStackedWidget *queueSpinnerStack{};
  Util::WaitingSpinnerWidget *queueSpinner{};
  Merge::Tool *toolMerge{};
  Info::Tool *toolInfo{};
  Jobs::Tool *toolJobs{};
  HeaderEditor::Tool *toolHeaderEditor{};
  ChapterEditor::Tool *toolChapterEditor{};
  WatchJobs::Tool *watchJobTool{};
  std::unique_ptr<Util::LanguageDialog> languageDialog;
  QList<QAction *> toolSelectionActions;
  bool queueIsRunning{};
  int spinnerIsSpinning{};
  std::unique_ptr<Util::BasicSleepInhibitor> sleepInhibitor{Util::BasicSleepInhibitor::create()};

  QHash<QObject *, QString> helpURLs;
  QHash<ToolBase *, QTabWidget *> subWindowWidgets;

  bool forceClosing{};

  explicit MainWindowPrivate()
    : ui{new Ui::MainWindow}
  {
  }
};

MainWindow::MainWindow(QWidget *parent)
  : QMainWindow{parent}
  , p_ptr{new MainWindowPrivate}
{
  auto p       = p_func();
  s_mainWindow = this;

  // Setup UI controls.
  p->ui->setupUi(this);

  setToolSelectorVisibility();
  setStayOnTopStatus();
  setupAuxiliaryWidgets();
  setupToolSelector();
  setupHelpURLs();
  setupDebuggingMenu();

  // Setup window properties.
  setWindowIcon(QIcon::fromTheme(Q("mkvtoolnix-gui")));

  retranslateUi();

  if (!Util::restoreWidgetGeometry(this))
    resizeToDefaultSize();

  App::programRunner().setup();

  setupConnections();

  jobTool()->loadAndStart();

#if defined(HAVE_UPDATE_CHECK)
  silentlyCheckForUpdates();
#else
  p->ui->actionHelpCheckForUpdates->setVisible(false);
#endif  // HAVE_UPDATE_CHECK

  new TaskbarProgress{this};

  runCacheCleanupOncePerVersion();

  Util::InstallationChecker::checkInstallation();
}

MainWindow::~MainWindow() {
}

void
MainWindow::resizeToDefaultSize() {
  QSize newSize{1060, 780};

  auto screens = App::screens();
  if (!screens.isEmpty()) {
    newSize.setWidth( std::max<unsigned int>(screens[0]->size().width()  * 3 / 4, newSize.width()));
    newSize.setHeight(std::max<unsigned int>(screens[0]->size().height() * 3 / 4, newSize.height()));
  }

  resize(newSize);
}

void
MainWindow::raiseAndActivate() {
  raise();
  activateWindow();
}

void
MainWindow::setStatusBarMessage(QString const &message) {
  auto p = p_func();

  p->ui->statusBar->showMessage(message, 3000);
}

QWidget *
MainWindow::createNotImplementedWidget() {
  auto p       = p_func();
  auto widget  = new QWidget{p->ui->tool};
  auto vlayout = new QVBoxLayout{widget};
  auto hlayout = new QHBoxLayout;
  auto text    = new QLabel{widget};

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
MainWindow::setupAuxiliaryWidgets() {
  auto p = p_func();

  p->statusBarProgress = new StatusBarProgressWidget{this};
  p->ui->statusBar->addPermanentWidget(p->statusBarProgress);

  // Queue status spinner
  p->queueSpinner = new Util::WaitingSpinnerWidget{nullptr, false, false};
  p->queueSpinner->setMinimumSize(23, 23);
  p->queueSpinner->setMinimumTrailOpacity(15.0);
  p->queueSpinner->setTrailFadePercentage(70.0);
  p->queueSpinner->setNumberOfLines(12);
  p->queueSpinner->setLineLength(9);
  p->queueSpinner->setLineWidth(2);
  p->queueSpinner->setInnerRadius(4);
  p->queueSpinner->setRevolutionsPerSecond(1);
  p->queueSpinner->setColor(QColor(0, 0, 0));

  p->queueSpinnerStack = new QStackedWidget;
  p->queueSpinnerStack->addWidget(new QWidget);
  p->queueSpinnerStack->addWidget(p->queueSpinner);
  p->queueSpinnerStack->widget(0)->resize(p->queueSpinner->size());
  p->queueSpinnerStack->setMinimumSize(p->queueSpinner->size());
  p->queueSpinnerStack->setMaximumSize(p->queueSpinner->size());

  p->ui->statusBar->addPermanentWidget(p->queueSpinnerStack);
}

void
MainWindow::setupConnections() {
  auto p             = p_func();
  auto currentJobTab = p->watchJobTool->currentJobTab();
  auto jobModel      = p->toolJobs->model();
  auto app           = App::instance();

  // Menu actions:
  connect(p->ui->actionGUIExit,                   &QAction::triggered,                                    this,                 &MainWindow::close);
  connect(p->ui->actionGUIPreferences,            &QAction::triggered,                                    this,                 &MainWindow::editPreferences);

  connect(p->ui->actionHelpFAQ,                   &QAction::triggered,                                    this,                 &MainWindow::visitHelpURL);
  connect(p->ui->actionHelpKnownProblems,         &QAction::triggered,                                    this,                 &MainWindow::visitHelpURL);
  connect(p->ui->actionHelpMkvmergeDocumentation, &QAction::triggered,                                    this,                 &MainWindow::visitMkvmergeDocumentation);
  connect(p->ui->actionHelpWebSite,               &QAction::triggered,                                    this,                 &MainWindow::visitHelpURL);
  connect(p->ui->actionHelpReportBug,             &QAction::triggered,                                    this,                 &MainWindow::visitHelpURL);
  connect(p->ui->actionHelpSystemInformation,     &QAction::triggered,                                    this,                 &MainWindow::showSystemInformation);
  connect(p->ui->actionHelpForum,                 &QAction::triggered,                                    this,                 &MainWindow::visitHelpURL);
  connect(p->ui->actionHelpCodeOfConduct,         &QAction::triggered,                                    this,                 &MainWindow::showCodeOfConduct);

  connect(p->ui->actionWindowNext,                &QAction::triggered,                                    this,                 [this]() { showNextOrPreviousSubWindow(1);  });
  connect(p->ui->actionWindowPrevious,            &QAction::triggered,                                    this,                 [this]() { showNextOrPreviousSubWindow(-1); });
  connect(p->ui->menuWindow,                      &QMenu::aboutToShow,                                    this,                 &MainWindow::setupWindowMenu);

#if defined(HAVE_UPDATE_CHECK)
  connect(p->ui->actionHelpCheckForUpdates,       &QAction::triggered,                                    this,                 &MainWindow::checkForUpdates);
#endif  // HAVE_UPDATE_CHECK

  // Tool actions:
  connect(p->ui->actionGUIMergeTool,              &QAction::triggered,                                    this,                 &MainWindow::changeToolToSender);
  // connect(p->ui->actionGUIExtractionTool,         &QAction::triggered,                                    this,                 &MainWindow::changeToolToSender);
  connect(p->ui->actionGUIInfoTool,               &QAction::triggered,                                    this,                 &MainWindow::changeToolToSender);
  connect(p->ui->actionGUIHeaderEditor,           &QAction::triggered,                                    this,                 &MainWindow::changeToolToSender);
  connect(p->ui->actionGUIChapterEditor,          &QAction::triggered,                                    this,                 &MainWindow::changeToolToSender);
  // connect(p->ui->actionGUITagEditor,              &QAction::triggered,                                    this,                 &MainWindow::changeToolToSender);
  connect(p->ui->actionGUIJobQueue,               &QAction::triggered,                                    this,                 &MainWindow::changeToolToSender);
  connect(p->ui->actionGUIJobOutput,              &QAction::triggered,                                    this,                 &MainWindow::changeToolToSender);

  connect(p->ui->tool,                            &Util::FancyTabWidget::currentChanged,                  this,                 &MainWindow::toolChanged);
  connect(jobModel,                               &Jobs::Model::progressChanged,                          p->statusBarProgress, &StatusBarProgressWidget::setProgress);
  connect(jobModel,                               &Jobs::Model::jobStatsChanged,                          p->statusBarProgress, &StatusBarProgressWidget::setJobStats);
  connect(jobModel,                               &Jobs::Model::numUnacknowledgedWarningsOrErrorsChanged, p->statusBarProgress, &StatusBarProgressWidget::setNumUnacknowledgedWarningsOrErrors);
  connect(jobModel,                               &Jobs::Model::queueStatusChanged,                       this,                 &MainWindow::startStopQueueSpinnerForQueue);
  connect(jobModel,                               &Jobs::Model::queueStatusChanged,                       this,                 &MainWindow::inhibitSleepWhileQueueIsRunning);
  connect(currentJobTab,                          &WatchJobs::Tab::watchCurrentJobTabCleared,             p->statusBarProgress, &StatusBarProgressWidget::reset);

  // Auxiliary actions:
  connect(this,                                   &MainWindow::preferencesChanged,                        this,                 &MainWindow::setToolSelectorVisibility);
  connect(this,                                   &MainWindow::preferencesChanged,                        this,                 &MainWindow::setStayOnTopStatus);
  connect(this,                                   &MainWindow::preferencesChanged,                        this,                 &MainWindow::showOrHideDebuggingMenu);

  connect(app,                                    &App::toolRequested,                                    this,                 &MainWindow::switchToTool);
  connect(app,                                    &App::systemColorSchemeChanged,                         this,                 &MainWindow::refreshWidgetsAfterColorSchemeChange);
  connect(&app->mediaPlayer(),                    &Util::MediaPlayer::errorOccurred,                      this,                 &MainWindow::handleMediaPlaybackError);
}

void
MainWindow::setupDebuggingMenu() {
  auto &p   = *p_func();
  auto tool = mergeTool();

  connect(p.ui->actionDbgMergeInputSetupHorizontalScrollAreaInputLayout, &QAction::triggered, tool, &Merge::Tool::setupHorizontalScrollAreaInputLayout);
  connect(p.ui->actionDbgMergeInputSetupHorizontalTwoColumnsInputLayout, &QAction::triggered, tool, &Merge::Tool::setupHorizontalTwoColumnsInputLayout);
  connect(p.ui->actionDbgMergeInputSetupVerticalTabWidgetInputLayout,    &QAction::triggered, tool, &Merge::Tool::setupVerticalTabWidgetInputLayout);

  showOrHideDebuggingMenu();
}

void
MainWindow::setupToolSelector() {
  auto p               = p_func();

  p->toolMerge         = new Merge::Tool{p->ui->tool,         p->ui->menuMerge};
  p->toolInfo          = new Info::Tool{p->ui->tool,          p->ui->menuInfo};
  p->toolHeaderEditor  = new HeaderEditor::Tool{p->ui->tool,  p->ui->menuHeaderEditor};
  p->toolChapterEditor = new ChapterEditor::Tool{p->ui->tool, p->ui->menuChapterEditor};
  p->toolJobs          = new Jobs::Tool{p->ui->tool,          p->ui->menuJobQueue};
  p->watchJobTool      = new WatchJobs::Tool{p->ui->tool,     p->ui->menuJobOutput};

  p->ui->tool->appendTab(p->toolMerge,                        QIcon::fromTheme(Q("merge")),                      QY("Multiplexer"));
  // p->ui->tool->appendTab(createNotImplementedWidget(),        QIcon::fromTheme(Q("split")),                      QY("Extractor"));
  p->ui->tool->appendTab(p->toolInfo,                         QIcon::fromTheme(Q("document-preview-archive")),   QY("Info tool"));
  p->ui->tool->appendTab(p->toolHeaderEditor,                 QIcon::fromTheme(Q("document-edit")),              QY("Header editor"));
  p->ui->tool->appendTab(p->toolChapterEditor,                QIcon::fromTheme(Q("story-editor")),               QY("Chapter editor"));
  // p->ui->tool->appendTab(createNotImplementedWidget(),        QIcon::fromTheme(Q("document-edit-sign-encrypt")), QY("Tags editor"));
  p->ui->tool->appendTab(p->toolJobs,                         QIcon::fromTheme(Q("view-task")),                  QY("Job queue"));
  p->ui->tool->appendTab(p->watchJobTool,                     QIcon::fromTheme(Q("system-run")),                 QY("Job output"));

  for (auto idx = 0, numTabs = p->ui->tool->count(); idx < numTabs; ++idx) {
    qobject_cast<ToolBase *>(p->ui->tool->widget(idx))->setupUi();
    p->ui->tool->setTabEnabled(idx, true);
  }

  for (auto idx = 0, numTabs = p->ui->tool->count(); idx < numTabs; ++idx)
    qobject_cast<ToolBase *>(p->ui->tool->widget(idx))->setupActions();

  p->ui->tool->setCurrentIndex(0);
  p->toolMerge->toolShown();
  showAndEnableMenu(*p->ui->menuWindow, p->subWindowWidgets.contains(p->toolMerge));

  p->toolSelectionActions << p->ui->actionGUIMergeTool    /* << p->ui->actionGUIExtractionTool */ << p->ui->actionGUIInfoTool
                          << p->ui->actionGUIHeaderEditor << p->ui->actionGUIChapterEditor  /*<< p->ui->actionGUITagEditor*/
                          << p->ui->actionGUIJobQueue     << p->ui->actionGUIJobOutput;

  p->ui->actionGUIExtractionTool->setVisible(false);
  p->ui->actionGUITagEditor->setVisible(false);
}

void
MainWindow::setupHelpURLs() {
  auto p = p_func();

  p->helpURLs[p->ui->actionHelpFAQ]                   = MTX_URL_FAQ;
  p->helpURLs[p->ui->actionHelpKnownProblems]         = MTX_URL_KNOWN_PROBLEMS;
  p->helpURLs[p->ui->actionHelpMkvmergeDocumentation] = MTX_URL_MKVMERGE_MAN;
  p->helpURLs[p->ui->actionHelpWebSite]               = MTX_URL_WEBSITE;
  p->helpURLs[p->ui->actionHelpReportBug]             = MTX_URL_ISSUES;
  p->helpURLs[p->ui->actionHelpForum]                 = MTX_URL_FORUM;
}

void
MainWindow::showOrHideDebuggingMenu() {
  auto &p = *p_func();

  if (Util::Settings::get().m_showDebuggingMenu)
    p.ui->menuBar->addMenu(p.ui->menuDebugging);
  else
    p.ui->menuBar->removeAction(p.ui->menuDebugging->menuAction());
}

void
MainWindow::showAndEnableMenu(QMenu &menu,
                              bool show) {
  auto p = p_func();

  if (show)
    p->ui->menuBar->insertMenu(p->ui->menuHelp->menuAction(), &menu);
  else
    p->ui->menuBar->removeAction(menu.menuAction());
}

void
MainWindow::showTheseMenusOnly(QList<QMenu *> const &menus) {
  auto p = p_func();

  for (auto menu : std::vector<QMenu *>{ p->ui->menuMerge, p->ui->menuInfo, p->ui->menuHeaderEditor, p->ui->menuChapterEditor, p->ui->menuJobQueue, p->ui->menuJobOutput })
    showAndEnableMenu(*menu, menus.contains(menu));
}

void
MainWindow::switchToTool(ToolBase *tool) {
  auto p = p_func();

  for (auto idx = 0, numTabs = p->ui->tool->count(); idx < numTabs; ++idx)
    if (p->ui->tool->widget(idx) == tool) {
      p->ui->tool->setCurrentIndex(idx);
      return;
    }
}

void
MainWindow::changeToolToSender() {
  auto p         = p_func();
  auto toolIndex = p->toolSelectionActions.indexOf(static_cast<QAction *>(sender()));

  if (-1 != toolIndex)
    p->ui->tool->setCurrentIndex(toolIndex);
}

void
MainWindow::toolChanged(int index) {
  auto p = p_func();

  showTheseMenusOnly({});

  auto widget   = p->ui->tool->widget(index);
  auto toolBase = dynamic_cast<ToolBase *>(widget);

  if (toolBase) {
    toolBase->toolShown();
    showAndEnableMenu(*p->ui->menuWindow, p->subWindowWidgets.contains(toolBase));
  }
}

MainWindow *
MainWindow::get() {
  return s_mainWindow;
}

Ui::MainWindow *
MainWindow::getUi() {
  return static_cast<MainWindowPrivate *>(get()->p_ptr.get())->ui.get();
}

Merge::Tool *
MainWindow::mergeTool() {
  return static_cast<MainWindowPrivate *>(get()->p_ptr.get())->toolMerge;
}

Info::Tool *
MainWindow::infoTool() {
  return static_cast<MainWindowPrivate *>(get()->p_ptr.get())->toolInfo;
}

HeaderEditor::Tool *
MainWindow::headerEditorTool() {
  return static_cast<MainWindowPrivate *>(get()->p_ptr.get())->toolHeaderEditor;
}

ChapterEditor::Tool *
MainWindow::chapterEditorTool() {
  return static_cast<MainWindowPrivate *>(get()->p_ptr.get())->toolChapterEditor;
}

Jobs::Tool *
MainWindow::jobTool() {
  return static_cast<MainWindowPrivate *>(get()->p_ptr.get())->toolJobs;
}

WatchJobs::Tab *
MainWindow::watchCurrentJobTab() {
  return watchJobTool()->currentJobTab();
}

WatchJobs::Tool *
MainWindow::watchJobTool() {
  return static_cast<MainWindowPrivate *>(get()->p_ptr.get())->watchJobTool;
}

void
MainWindow::retranslateUi() {
  auto p = p_func();

  p->ui->retranslateUi(this);
  p->statusBarProgress->retranslateUi();

  setWindowTitle(Q(get_version_info("MKVToolNix GUI")));

  p->ui->tool->setUpdatesEnabled(false);

  // Intentionally replacing the list right away again in order not to
  // lose the translations for the three currently unimplemented
  // tools.
  auto toolTitles = QStringList{} << QY("Extraction tool") << QY("Tag editor");
  toolTitles      = QStringList{} << QY("Multiplexer") << QY("Info tool") << QY("Header editor") << QY("Chapter editor") << QY("Job queue") << QY("Job output");

  for (auto idx = 0, count = p->ui->tool->count(); idx < count; ++idx)
    p->ui->tool->setTabText(idx, toolTitles[idx]);

  // Intentionally setting the menu titles here instead of the
  // designer as the designer doesn't allow the same hotkey in the
  // same form.
  p->ui->menuGUI          ->setTitle(QY("MKVToolNix &GUI"));
  p->ui->menuMerge        ->setTitle(QY("&Multiplexer"));
  p->ui->menuInfo         ->setTitle(QY("&Info tool"));
  p->ui->menuHeaderEditor ->setTitle(QY("Header &editor"));
  p->ui->menuChapterEditor->setTitle(QY("&Chapter editor"));
  p->ui->menuJobQueue     ->setTitle(QY("&Job queue"));
  p->ui->menuJobOutput    ->setTitle(QY("&Job output"));
  p->ui->menuHelp         ->setTitle(QY("&Help"));

  p->ui->tool->setUpdatesEnabled(true);
}

void
MainWindow::forceClose() {
  auto &p = *p_func();

  p.forceClosing = true;
  close();
}

bool
MainWindow::beforeCloseCheckRunningJobs() {
  auto &p   = *p_func();
  auto tool = jobTool();
  if (!tool)
    return true;

  auto model = tool->model();
  if (!model->hasRunningJobs())
    return true;

  if (   !p.forceClosing
      && Util::Settings::get().m_warnBeforeAbortingJobs
      && (Util::MessageBox::question(this)
            ->title(QY("Abort running jobs"))
            .text(Q("%1 %2").arg(QY("There is currently a job running.")).arg(QY("Do you really want to abort all currently running jobs?")))
            .buttonLabel(QMessageBox::Yes, QY("&Abort jobs"))
            .buttonLabel(QMessageBox::No,  QY("Cancel"))
            .exec()) == QMessageBox::No)
    return false;

  model->stop();
  model->withAllJobs([](Jobs::Job &job) {
    if (Jobs::Job::Running == job.status()) {
      job.setQuitAfterFinished(true);
      job.abort();
    }
  });

  return false;
}

void
MainWindow::closeEvent(QCloseEvent *event) {
  auto &p = *p_func();

  Q_EMIT aboutToClose();

  if (p.forceClosing) {
    Util::saveWidgetGeometry(this);
    return;
  }

  auto ok =       mergeTool()->closeAllTabs();
  ok      = ok && headerEditorTool()->closeAllTabs();
  ok      = ok && chapterEditorTool()->closeAllTabs();
  ok      = ok && beforeCloseCheckRunningJobs();

  if (!ok) {
    event->ignore();
    return;
  }

  Util::saveWidgetGeometry(this);

  event->accept();
}

void
MainWindow::editPreferences() {
  editPreferencesAndShowPage(PreferencesDialog::Page::PreviouslySelected);
}

void
MainWindow::editRunProgramConfigurations() {
  editPreferencesAndShowPage(PreferencesDialog::Page::RunPrograms);
}

void
MainWindow::editPreferencesAndShowPage(PreferencesDialog::Page page) {
  auto &p = *p_func();

  PreferencesDialog dlg{this, page};
  if (!dlg.exec())
    return;

  setUpdatesEnabled(false);

  dlg.save();

  auto &app = *App::instance();

  if (dlg.uiLocaleChanged() || dlg.disableToolTipsChanged())
    app.initializeLocale();

  if (dlg.uiLocaleChanged() || dlg.probeRangePercentageChanged())
    [[maybe_unused]] auto future = QtConcurrent::run(Util::FileIdentifier::cleanAllCacheFiles);

  if (dlg.uiFontChanged())
    app.setupUiFont();

  if (dlg.uiFontChanged() || dlg.languageRegionCharacterSetSettingsChanged())
    app.reinitializeLanguageLists();

  mtx::bcp47::language_c::set_normalization_mode(Util::Settings::get().m_bcp47NormalizationMode);

  p.languageDialog.release();

  Q_EMIT preferencesChanged();

  setUpdatesEnabled(true);
}

#if defined(HAVE_UPDATE_CHECK)
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

  qDebug() << "Creating checker";

  auto checker = new UpdateChecker{this};

  connect(checker, &UpdateChecker::checkFinished, this, &MainWindow::updateCheckFinished);

  QTimer::singleShot(0, checker, [checker]() { checker->start(); });
}

QString
MainWindow::versionStringForSettings(version_number_t const &version) {
  return Q("version_%1").arg(Q(version.to_string()).replace(QRegularExpression{Q("[^0-9]+")}, Q("_")));
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
#endif  // HAVE_UPDATE_CHECK

void
MainWindow::visitHelpURL() {
  auto p = p_func();

  if (p->helpURLs.contains(sender()))
    QDesktopServices::openUrl(p->helpURLs[sender()]);
}

void
MainWindow::visitMkvmergeDocumentation() {
  auto p              = p_func();
  auto appDirPath     = App::applicationDirPath();
  auto potentialPaths = QStringList{};

  try {
    auto localeStr = locale_string_c{to_utf8(Util::Settings::get().localeToUse())};

    potentialPaths << Q("%1/doc/%2").arg(appDirPath).arg(Q(localeStr.str(locale_string_c::full)));
    potentialPaths << Q("%1/doc/%2").arg(appDirPath).arg(Q(localeStr.str(static_cast<locale_string_c::eval_type_e>(locale_string_c::language | locale_string_c::territory))));
    potentialPaths << Q("%1/doc/%2").arg(appDirPath).arg(Q(localeStr.str(locale_string_c::language)));

  } catch (mtx::locale_string_format_x const &) {
  }

  potentialPaths << Q("%1/doc/en").arg(appDirPath);

  auto url = QUrl{};

  for (auto const &path : potentialPaths) {
    auto fileName = Q("%1/mkvmerge.html").arg(path);

    if (QFileInfo{fileName}.exists()) {
      url.setScheme(Q("file"));
      url.setPath(fileName);
      break;
    }
  }

  if (url.isEmpty())
    url = p->helpURLs[p->ui->actionHelpMkvmergeDocumentation];

  QDesktopServices::openUrl(url);
}

void
MainWindow::showCodeOfConduct() {
  Util::TextDisplayDialog dlg{this};

  dlg.setTitle(QY("The MKVToolNix Code of Conduct"));

  QFile coc{Q(":/CODE_OF_CONDUCT.md")};
  if (coc.open(QIODevice::ReadOnly))
    dlg.setText(Q(std::string{coc.readAll().constData()}), Util::TextDisplayDialog::Format::Markdown)
      .setSaveInfo(Q("mkvtoolnix_code_of_conduct.md"), QY("Markdown files"), Q("md"));

  dlg.exec();
}

void
MainWindow::showSystemInformation() {
  Util::TextDisplayDialog dlg{this};

  dlg.setTitle(QY("System information"))
    .setText(Util::gatherSystemInformation(), Util::TextDisplayDialog::Format::Markdown)
    .setSaveInfo(Q("mkvtoolnix_gui_system_information.md"), QY("Markdown files"), Q("md"))
    .exec();
}

void
MainWindow::showEvent(QShowEvent *event) {
  Q_EMIT windowShown();
  event->accept();
}

void
MainWindow::refreshWidgetsAfterColorSchemeChange() {
  // On macOS, Qt doesn't automatically refresh widgets when the system
  // appearance changes. Re-applying the style forces all widgets to
  // re-read their palette and style properties.
  //
  // We need to preserve the font because setStyle() resets it.
  auto currentFont = App::font();
  auto styleName = App::style()->objectName();

  App::setStyle(styleName);
  App::setFont(currentFont);

  // Clear Qt's icon theme cache by toggling the theme name.
  // Icons are rendered and cached with colors from the active color scheme.
  // Without clearing the cache, icons retain their old-scheme colors.
  auto themeName = QIcon::themeName();
  QIcon::setThemeName(QString{});
  QIcon::setThemeName(themeName);

  // Notify other components that may need to update after the color scheme change.
  Q_EMIT preferencesChanged();
}

void
MainWindow::setToolSelectorVisibility() {
  auto p = p_func();

  p->ui->tool->tabBar()->setVisible(Util::Settings::get().m_showToolSelector);
}

void
MainWindow::setStayOnTopStatus() {
  auto oldFlags = windowFlags();
  auto newFlags = Util::Settings::get().m_uiStayOnTop ? oldFlags |  Qt::WindowStaysOnTopHint
                :                                       oldFlags & ~Qt::WindowStaysOnTopHint;

  if (oldFlags == newFlags)
    return;

  setWindowFlags(newFlags);
  show();
}

std::optional<bool>
MainWindow::filterWheelEventForStrongFocus(QObject *watched,
                                           QEvent *event) {
  if (event->type() != QEvent::Wheel)
    return {};

  auto widget = qobject_cast<QWidget *>(watched);
  if (   !widget
      ||  widget->hasFocus()
      || (widget->focusPolicy() != Qt::StrongFocus))
    return {};

  event->ignore();
  return true;
}

bool
MainWindow::eventFilter(QObject *watched,
                        QEvent *event) {
  auto result = filterWheelEventForStrongFocus(watched, event);

  if (result)
    return *result;

  return QMainWindow::eventFilter(watched, event);
}

QIcon
MainWindow::yesIcon() {
  return QIcon::fromTheme("dialog-ok-apply");
}

QIcon
MainWindow::noIcon() {
  return QIcon::fromTheme("dialog-cancel");
}

void
MainWindow::displayInstallationProblems(Util::InstallationChecker::Problems const &problems) {
  if (problems.isEmpty())
    return;

  auto numProblems    = problems.size();
  auto problemsString = QString{};

  for (auto const &problem : problems) {
    auto description = QString{};

    switch (problem.first) {
      case Util::InstallationChecker::ProblemType::FileNotFound:
        description = QY("The file '%1' could not be found in the installation folder.").arg(problem.second);
        break;

      case Util::InstallationChecker::ProblemType::MkvmergeNotFound:
        description = QY("The mkvmerge executable was not found.");
        break;

      case Util::InstallationChecker::ProblemType::MkvmergeCannotBeExecuted:
        description = QY("The mkvmerge executable was found, but it couldn't be executed.");
        break;

      case Util::InstallationChecker::ProblemType::MkvmergeVersionNotRecognized:
        description = QY("The version line reported by mkvmerge ('%1') could not be recognized.").arg(problem.second);
        break;

      case Util::InstallationChecker::ProblemType::MkvmergeVersionDiffers:
        description = QY("The versions of mkvmerge (%1) and the GUI (%2) differ.").arg(problem.second).arg(Q(get_current_version().to_string()));
        break;

      case Util::InstallationChecker::ProblemType::TemporaryDirectoryNotWritable:
        description = QY("Temporary files cannot be created. Additional information: %1").arg(problem.second);
        break;

      case Util::InstallationChecker::ProblemType::PortableDirectoryNotWritable:
        description = QY("MKVToolNix GUI acts as a portable application but cannot write to its base directory, '%1'. If it should act as an installed application, remove the file '%2'.")
          .arg(QDir::toNativeSeparators(problem.second))
          .arg(QDir::toNativeSeparators(Q("%1/data/portable-app").arg(problem.second)));
        break;
    }

    problemsString += Q("<li>%1</li>").arg(description.toHtmlEscaped());
  }

  Util::MessageBox::critical(this)
    ->title(QNY("Problem with MKVToolNix installation", "Problems with MKVToolNix installation", numProblems))
    .text(Q("<p>%1</p>"
            "<ul>%2</ul>"
            "<p>%3 %4</p>")
          .arg(QNY("A problem has been detected with this installation of MKVToolNix:", "Several problems have been detected with this installation of MKVToolNix:", numProblems).toHtmlEscaped())
          .arg(problemsString)
          .arg(QY("Certain functions won't work correctly in this situation.").toHtmlEscaped())
          .arg(QNY("Please re-install MKVToolNix or fix the problem manually.", "Please re-install MKVToolNix or fix the problems manually.", numProblems).toHtmlEscaped()))
    .exec();
}

void
MainWindow::runCacheCleanupOncePerVersion()
  const {
  Util::Settings::runOncePerVersion(Q("cacheCleanup"), []() {
    [[maybe_unused]] auto future = QtConcurrent::run(Util::Cache::cleanOldCacheFiles);
  });
}

std::pair<ToolBase *, QTabWidget *>
MainWindow::currentSubWindowWidget() {
  auto p        = p_func();
  auto toolBase = dynamic_cast<ToolBase *>(p->ui->tool->widget(p->ui->tool->currentIndex()));

  if (toolBase && p->subWindowWidgets.contains(toolBase))
    return { toolBase, p->subWindowWidgets[toolBase] };

  return {};
}

void
MainWindow::registerSubWindowWidget(ToolBase &toolBase,
                                    QTabWidget &tabWidget) {
  p_func()->subWindowWidgets[&toolBase] = &tabWidget;
}

void
MainWindow::showNextOrPreviousSubWindow(int delta) {
  auto subWindow = currentSubWindowWidget();

  if (!subWindow.first)
    return;

  auto numSubWindows = subWindow.second->count();
  if (numSubWindows < 2)
    return;

  auto newIndex = subWindow.second->currentIndex() + delta;
  newIndex      = (newIndex + (newIndex < 0 ? numSubWindows : 0)) % numSubWindows;

  subWindow.second->setCurrentIndex(newIndex);
}

void
MainWindow::setupWindowMenu() {
  auto p         = p_func();
  auto subWindow = currentSubWindowWidget();

  if (!subWindow.first)
    return;

  auto texts         = subWindow.first->nextPreviousWindowActionTexts();
  auto numSubWindows = subWindow.second->count();
  auto menu          = p->ui->menuWindow;

  p->ui->actionWindowNext->setText(texts.first);
  p->ui->actionWindowPrevious->setText(texts.second);

  p->ui->actionWindowNext->setEnabled(numSubWindows >= 2);
  p->ui->actionWindowPrevious->setEnabled(numSubWindows >= 2);

  for (auto &action : menu->actions())
    if (!mtx::included_in(action, p->ui->actionWindowNext, p->ui->actionWindowPrevious))
      delete action;

  if (!numSubWindows)
    return;

  menu->addSeparator();

  for (auto tabIdx = 0; tabIdx < numSubWindows; ++tabIdx) {
    auto prefix = tabIdx <= 8 ? Q("&%1: ").arg(tabIdx + 1)
                : tabIdx == 9 ? Q("1&0: ")
                :               Q("");
    auto text   = subWindow.second->tabText(tabIdx);
    auto action = menu->addAction(Q("%1%2").arg(prefix).arg(text));

    connect(action, &QAction::triggered, this, [this, tabIdx]() { showSubWindow(tabIdx); });
  }
}

void
MainWindow::showSubWindow(unsigned int tabIdx) {
  auto subWindow = currentSubWindowWidget();

  if (subWindow.first && (tabIdx < static_cast<unsigned int>(subWindow.second->count())))
    subWindow.second->setCurrentIndex(tabIdx);
}

void
MainWindow::startStopQueueSpinnerForQueue(Jobs::QueueStatus status) {
  auto p                 = p_func();
  auto newQueueIsRunning = status == Jobs::QueueStatus::Running;

  if (p->queueIsRunning == newQueueIsRunning)
    return;

  p->queueIsRunning = newQueueIsRunning;
  startStopQueueSpinner(newQueueIsRunning);
}

void
MainWindow::inhibitSleepWhileQueueIsRunning(Jobs::QueueStatus status) {
  auto p = p_func();

  if (status == Jobs::QueueStatus::Running)
    p->sleepInhibitor->inhibit();
  else
    p->sleepInhibitor->uninhibit();
}

void
MainWindow::startStopQueueSpinner(bool start) {
  auto p = p_func();

  if (start) {
    ++p->spinnerIsSpinning;
    if (p->spinnerIsSpinning) {
      p->queueSpinnerStack->setCurrentIndex(1);
      p->queueSpinner->start();
    }

  } else if (p->spinnerIsSpinning > 0) {
    --p->spinnerIsSpinning;
    if (!p->spinnerIsSpinning) {
      p->queueSpinnerStack->setCurrentIndex(0);
      p->queueSpinner->stop();
    }
  }
}

void
MainWindow::startQueueSpinner() {
  startStopQueueSpinner(true);
}

void
MainWindow::stopQueueSpinner() {
  startStopQueueSpinner(false);
}

void
MainWindow::handleMediaPlaybackError(QMediaPlayer::Error error,
                                     QString const &fileName) {
  QStringList messages;

  messages << QY("The audio file '%1' could not be played back.").arg(fileName);

  if (error == QMediaPlayer::FormatError)
    messages << QY("Either the file format or the audio codec is not supported.");

  else if ((error == QMediaPlayer::ResourceError) || (error == QMediaPlayer::AccessDeniedError))
    messages << QY("No audio device is available for playback, or accessing it failed.");

  Util::MessageBox::critical(this)
    ->title(QY("Error playing audio"))
    .text(messages.join(Q(" ")))
    .exec();
}

Util::LanguageDialog &
MainWindow::setupLanguageDialog() {
  auto p = p_func();

  if (!p->languageDialog)
    p->languageDialog.reset(new Util::LanguageDialog{nullptr});
  return *p->languageDialog;
}

Util::LanguageDialog &
MainWindow::languageDialog() {
  return MainWindow::get()->setupLanguageDialog();
}

}
