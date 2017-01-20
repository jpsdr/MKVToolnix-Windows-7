#include "common/common_pch.h"

#include <QCloseEvent>
#include <QDesktopServices>
#include <QEvent>
#include <QIcon>
#include <QLabel>
#include <QMessageBox>
#include <QStaticText>
#include <QVBoxLayout>
#include <QtConcurrent>

#include "common/fs_sys_helpers.h"
#include "common/locale_string.h"
#include "common/qt.h"
#include "common/version.h"
#include "mkvtoolnix-gui/app.h"
#include "mkvtoolnix-gui/chapter_editor/tool.h"
#include "mkvtoolnix-gui/forms/main_window/main_window.h"
#include "mkvtoolnix-gui/header_editor/tool.h"
#include "mkvtoolnix-gui/jobs/tool.h"
#include "mkvtoolnix-gui/main_window/available_update_info_dialog.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/main_window/preferences_dialog.h"
#include "mkvtoolnix-gui/main_window/status_bar_progress_widget.h"
#if defined(SYS_WINDOWS)
# include "mkvtoolnix-gui/main_window/taskbar_progress.h"
#endif
#include "mkvtoolnix-gui/merge/tool.h"
#include "mkvtoolnix-gui/util/cache.h"
#include "mkvtoolnix-gui/util/file_identifier.h"
#include "mkvtoolnix-gui/util/message_box.h"
#include "mkvtoolnix-gui/util/moving_pixmap_overlay.h"
#include "mkvtoolnix-gui/util/settings.h"
#include "mkvtoolnix-gui/util/widget.h"
#include "mkvtoolnix-gui/watch_jobs/tab.h"
#include "mkvtoolnix-gui/watch_jobs/tool.h"

namespace mtx { namespace gui {

MainWindow *MainWindow::ms_mainWindow = nullptr;

MainWindow::MainWindow(QWidget *parent)
  : QMainWindow{parent}
  , ui{new Ui::MainWindow}
{
  ms_mainWindow = this;

  // Setup UI controls.
  ui->setupUi(this);
  setToolSelectorVisibility();

  m_movingPixmapOverlay = std::make_unique<Util::MovingPixmapOverlay>(centralWidget());

  m_statusBarProgress = new StatusBarProgressWidget{this};
  ui->statusBar->addPermanentWidget(m_statusBarProgress);

  setupMenu();
  setupToolSelector();
  setupHelpURLs();

  // Setup window properties.
  setWindowIcon(Util::loadIcon(Q("mkvtoolnix-gui.png"), QList<int>{} << 32 << 48 << 64 << 128 << 256));

  retranslateUi();

  Util::restoreWidgetGeometry(this);

  jobTool()->loadAndStart();

  silentlyCheckForUpdates();

#if defined(SYS_WINDOWS)
  new TaskbarProgress{this};
#endif

  runCacheCleanupOncePerVersion();

  Util::InstallationChecker::checkInstallation();
}

MainWindow::~MainWindow() {
}

void
MainWindow::raiseAndActivate() {
  raise();
  activateWindow();
}

void
MainWindow::setStatusBarMessage(QString const &message) {
  ui->statusBar->showMessage(message, 3000);
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
  connect(ui->actionGUIExit,                   &QAction::triggered,             this, &MainWindow::close);
  connect(ui->actionGUIPreferences,            &QAction::triggered,             this, &MainWindow::editPreferences);

  connect(ui->actionHelpFAQ,                   &QAction::triggered,             this, &MainWindow::visitHelpURL);
  connect(ui->actionHelpKnownProblems,         &QAction::triggered,             this, &MainWindow::visitHelpURL);
  connect(ui->actionHelpMkvmergeDocumentation, &QAction::triggered,             this, &MainWindow::visitMkvmergeDocumentation);
  connect(ui->actionHelpWebSite,               &QAction::triggered,             this, &MainWindow::visitHelpURL);
  connect(ui->actionHelpReportBug,             &QAction::triggered,             this, &MainWindow::visitHelpURL);

  connect(this,                                &MainWindow::preferencesChanged, this, &MainWindow::setToolSelectorVisibility);

  connect(ui->actionHelpCheckForUpdates,       &QAction::triggered,             this, &MainWindow::checkForUpdates);
}

void
MainWindow::setupToolSelector() {
  m_toolMerge         = new Merge::Tool{ui->tool,         ui->menuMerge};
  m_toolHeaderEditor  = new HeaderEditor::Tool{ui->tool,  ui->menuHeaderEditor};
  m_toolChapterEditor = new ChapterEditor::Tool{ui->tool, ui->menuChapterEditor};
  m_toolJobs          = new Jobs::Tool{ui->tool,          ui->menuJobQueue};
  m_watchJobTool      = new WatchJobs::Tool{ui->tool,     ui->menuJobOutput};

  ui->tool->appendTab(m_toolMerge,                  QIcon{":/icons/48x48/merge.png"},                      QY("Multiplexer"));
  // ui->tool->appendTab(createNotImplementedWidget(), QIcon{":/icons/48x48/split.png"},                      QY("Extractor"));
  // ui->tool->appendTab(createNotImplementedWidget(), QIcon{":/icons/48x48/document-preview-archive.png"},   QY("Info tool"));
  ui->tool->appendTab(m_toolHeaderEditor,           QIcon{":/icons/48x48/document-edit.png"},              QY("Header editor"));
  ui->tool->appendTab(m_toolChapterEditor,          QIcon{":/icons/48x48/story-editor.png"},               QY("Chapter editor"));
  // ui->tool->appendTab(createNotImplementedWidget(), QIcon{":/icons/48x48/document-edit-sign-encrypt.png"}, QY("Tags editor"));
  ui->tool->appendTab(m_toolJobs,                   QIcon{":/icons/48x48/view-task.png"},                  QY("Job queue"));
  ui->tool->appendTab(m_watchJobTool,               QIcon{":/icons/48x48/system-run.png"},                 QY("Job output"));

  for (auto idx = 0, numTabs = ui->tool->count(); idx < numTabs; ++idx) {
    qobject_cast<ToolBase *>(ui->tool->widget(idx))->setupUi();
    ui->tool->setTabEnabled(idx, true);
  }

  for (auto idx = 0, numTabs = ui->tool->count(); idx < numTabs; ++idx)
    qobject_cast<ToolBase *>(ui->tool->widget(idx))->setupActions();

  ui->tool->setCurrentIndex(0);
  m_toolMerge->toolShown();

  m_toolSelectionActions << ui->actionGUIMergeTool    /* << ui->actionGUIExtractionTool << ui->actionGUIInfoTool*/
                         << ui->actionGUIHeaderEditor << ui->actionGUIChapterEditor  /*<< ui->actionGUITagEditor*/
                         << ui->actionGUIJobQueue     << ui->actionGUIJobOutput;

  ui->actionGUIExtractionTool->setVisible(false);
  ui->actionGUIInfoTool->setVisible(false);
  ui->actionGUITagEditor->setVisible(false);

  auto currentJobTab = m_watchJobTool->currentJobTab();

  connect(ui->actionGUIMergeTool,      &QAction::triggered,                                    this,                &MainWindow::changeToolToSender);
  // connect(ui->actionGUIExtractionTool, &QAction::triggered,                                    this,                &MainWindow::changeToolToSender);
  // connect(ui->actionGUIInfoTool,       &QAction::triggered,                                    this,                &MainWindow::changeToolToSender);
  connect(ui->actionGUIHeaderEditor,   &QAction::triggered,                                    this,                &MainWindow::changeToolToSender);
  connect(ui->actionGUIChapterEditor,  &QAction::triggered,                                    this,                &MainWindow::changeToolToSender);
  // connect(ui->actionGUITagEditor,      &QAction::triggered,                                    this,                &MainWindow::changeToolToSender);
  connect(ui->actionGUIJobQueue,       &QAction::triggered,                                    this,                &MainWindow::changeToolToSender);
  connect(ui->actionGUIJobOutput,      &QAction::triggered,                                    this,                &MainWindow::changeToolToSender);

  connect(ui->tool,                    &Util::FancyTabWidget::currentChanged,                  this,                &MainWindow::toolChanged);
  connect(m_toolJobs->model(),         &Jobs::Model::progressChanged,                          m_statusBarProgress, &StatusBarProgressWidget::setProgress);
  connect(m_toolJobs->model(),         &Jobs::Model::jobStatsChanged,                          m_statusBarProgress, &StatusBarProgressWidget::setJobStats);
  connect(m_toolJobs->model(),         &Jobs::Model::numUnacknowledgedWarningsOrErrorsChanged, m_statusBarProgress, &StatusBarProgressWidget::setNumUnacknowledgedWarningsOrErrors);
  connect(currentJobTab,               &WatchJobs::Tab::watchCurrentJobTabCleared,             m_statusBarProgress, &StatusBarProgressWidget::reset);
}

void
MainWindow::setupHelpURLs() {
  m_helpURLs[ui->actionHelpFAQ]                   = "https://github.com/mbunkus/mkvtoolnix/wiki";
  m_helpURLs[ui->actionHelpKnownProblems]         = "https://github.com/mbunkus/mkvtoolnix/wiki/Troubleshooting";
  m_helpURLs[ui->actionHelpMkvmergeDocumentation] = "https://mkvtoolnix.download/doc/mkvmerge.html";
  m_helpURLs[ui->actionHelpWebSite]               = "https://mkvtoolnix.download/";
  m_helpURLs[ui->actionHelpReportBug]             = "https://github.com/mbunkus/mkvtoolnix/issues/";
}

void
MainWindow::showAndEnableMenu(QMenu &menu,
                              bool show) {

  if (show)
    ui->menuBar->insertMenu(ui->menuHelp->menuAction(), &menu);
  else
    ui->menuBar->removeAction(menu.menuAction());
}

void
MainWindow::showTheseMenusOnly(QList<QMenu *> const &menus) {
  showAndEnableMenu(*ui->menuMerge,         menus.contains(ui->menuMerge));
  showAndEnableMenu(*ui->menuHeaderEditor,  menus.contains(ui->menuHeaderEditor));
  showAndEnableMenu(*ui->menuChapterEditor, menus.contains(ui->menuChapterEditor));
  showAndEnableMenu(*ui->menuJobQueue,      menus.contains(ui->menuJobQueue));
  showAndEnableMenu(*ui->menuJobOutput,     menus.contains(ui->menuJobOutput));
}

void
MainWindow::switchToTool(ToolBase *tool) {
  for (auto idx = 0, numTabs = ui->tool->count(); idx < numTabs; ++idx)
    if (ui->tool->widget(idx) == tool) {
      ui->tool->setCurrentIndex(idx);
      return;
    }
}

void
MainWindow::changeToolToSender() {
  auto toolIndex = m_toolSelectionActions.indexOf(static_cast<QAction *>(sender()));
  if (-1 != toolIndex)
    ui->tool->setCurrentIndex(toolIndex);
}

void
MainWindow::toolChanged(int index) {
  showTheseMenusOnly({});

  auto widget   = ui->tool->widget(index);
  auto toolBase = dynamic_cast<ToolBase *>(widget);

  if (toolBase)
    toolBase->toolShown();
}

MainWindow *
MainWindow::get() {
  return ms_mainWindow;
}

Ui::MainWindow *
MainWindow::getUi() {
  return ms_mainWindow->ui.get();
}

Merge::Tool *
MainWindow::mergeTool() {
  return get()->m_toolMerge;
}

HeaderEditor::Tool *
MainWindow::headerEditorTool() {
  return get()->m_toolHeaderEditor;
}

ChapterEditor::Tool *
MainWindow::chapterEditorTool() {
  return get()->m_toolChapterEditor;
}

Jobs::Tool *
MainWindow::jobTool() {
  return get()->m_toolJobs;
}

WatchJobs::Tab *
MainWindow::watchCurrentJobTab() {
  return watchJobTool()->currentJobTab();
}

WatchJobs::Tool *
MainWindow::watchJobTool() {
  return get()->m_watchJobTool;
}

void
MainWindow::retranslateUi() {
  ui->retranslateUi(this);
  m_statusBarProgress->retranslateUi();

  setWindowTitle(Q(get_version_info("MKVToolNix GUI")));

  ui->tool->setUpdatesEnabled(false);

  // Intentionally replacing the list right away again in order not to
  // lose the translations for the three currently unimplemented
  // tools.
  auto toolTitles = QStringList{} << QY("Extraction tool") << QY("Info tool") << QY("Tag editor");
  toolTitles      = QStringList{} << QY("Multiplexer") << QY("Header editor") << QY("Chapter editor") << QY("Job queue") << QY("Job output");

  for (auto idx = 0, count = ui->tool->count(); idx < count; ++idx)
    ui->tool->setTabText(idx, toolTitles[idx]);

  // Intentionally setting the menu titles here instead of the
  // designer as the designer doesn't allow the same hotkey in the
  // same form.
  ui->menuGUI          ->setTitle(QY("MKVToolNix &GUI"));
  ui->menuMerge        ->setTitle(QY("&Multiplexer"));
  ui->menuHeaderEditor ->setTitle(QY("Header &editor"));
  ui->menuChapterEditor->setTitle(QY("&Chapter editor"));
  ui->menuJobQueue     ->setTitle(QY("&Job queue"));
  ui->menuJobOutput    ->setTitle(QY("&Job output"));
  ui->menuHelp         ->setTitle(QY("&Help"));

  ui->tool->setUpdatesEnabled(true);
}

bool
MainWindow::beforeCloseCheckRunningJobs() {
  auto tool = jobTool();
  if (!tool)
    return true;

  auto model = tool->model();
  if (!model->hasRunningJobs())
    return true;

  if (   Util::Settings::get().m_warnBeforeAbortingJobs
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
  emit aboutToClose();

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
  PreferencesDialog dlg{this};
  if (!dlg.exec())
    return;

  dlg.save();

  if (dlg.uiLocaleChanged())
    App::instance()->initializeLocale();

  if (dlg.uiLocaleChanged() || dlg.probeRangePercentageChanged())
    QtConcurrent::run(Util::FileIdentifier::cleanAllCacheFiles);

  App::instance()->reinitializeLanguageLists();
  App::setupUiFont();

  emit preferencesChanged();
}

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

  QTimer::singleShot(0, checker, SLOT(start()));
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

void
MainWindow::showIconMovingToTool(QString const &pixmapName,
                                 ToolBase const &tool) {
  if (Util::Settings::get().m_disableAnimations)
    return;

  for (auto idx = 0, count = ui->tool->count(); idx < count; ++idx)
    if (&tool == ui->tool->widget(idx)) {
      auto size = 32;
      auto rect = ui->tool->tabBar()->tabRect(idx);

      auto from = centralWidget()->mapFromGlobal(QCursor::pos());
      auto to   = QPoint{rect.x() + (rect.width()  - size) / 2,
                         rect.y() + (rect.height() - size) / 2};
      to        = centralWidget()->mapFromGlobal(ui->tool->tabBar()->mapToGlobal(to));

      m_movingPixmapOverlay->addMovingPixmap(Q(":/icons/%1x%1/%2").arg(size).arg(pixmapName), from, to);

      return;
    }
}

void
MainWindow::resizeEvent(QResizeEvent *event) {
  m_movingPixmapOverlay->resize(event->size());
  event->accept();
}

void
MainWindow::visitHelpURL() {
  if (m_helpURLs.contains(sender()))
    QDesktopServices::openUrl(m_helpURLs[sender()]);
}

void
MainWindow::visitMkvmergeDocumentation() {
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
    url = m_helpURLs[ui->actionHelpMkvmergeDocumentation];

  QDesktopServices::openUrl(url);
}

void
MainWindow::showEvent(QShowEvent *event) {
  emit windowShown();
  event->accept();
}

void
MainWindow::setToolSelectorVisibility() {
  ui->tool->tabBar()->setVisible(Util::Settings::get().m_showToolSelector);
}

boost::optional<bool>
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

QIcon const &
MainWindow::yesIcon() {
  static auto s_yesIcon = std::unique_ptr<QIcon>{};
  if (!s_yesIcon)
    s_yesIcon.reset(new QIcon{":/icons/16x16/dialog-ok-apply.png"});

  return *s_yesIcon;
}

QIcon const &
MainWindow::noIcon() {
  static auto s_noIcon = std::unique_ptr<QIcon>{};
  if (!s_noIcon)
    s_noIcon.reset(new QIcon{":/icons/16x16/dialog-cancel.png"});

  return *s_noIcon;
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
    QtConcurrent::run(Util::Cache::cleanOldCacheFiles);
  });
}

}}
