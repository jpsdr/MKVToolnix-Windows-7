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
#include "common/list_utils.h"
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

#define GET_D static_cast<MainWindowPrivate *>(get()->d_ptr.data())

namespace mtx { namespace gui {

MainWindow *s_mainWindow = nullptr;

class MainWindowPrivate {
  friend class MainWindow;

  std::unique_ptr<Ui::MainWindow> ui;
  StatusBarProgressWidget *statusBarProgress{};
  Merge::Tool *toolMerge{};
  Jobs::Tool *toolJobs{};
  HeaderEditor::Tool *toolHeaderEditor{};
  ChapterEditor::Tool *toolChapterEditor{};
  WatchJobs::Tool *watchJobTool{};
  QList<QAction *> toolSelectionActions;
  std::unique_ptr<Util::MovingPixmapOverlay> movingPixmapOverlay;

  QHash<QObject *, QString> helpURLs;
  QHash<ToolBase *, QTabWidget *> subWindowWidgets;

  explicit MainWindowPrivate()
    : ui{new Ui::MainWindow}
  {
  }
};

MainWindow::MainWindow(QWidget *parent)
  : QMainWindow{parent}
  , d_ptr{new MainWindowPrivate}
{
  Q_D(MainWindow);

  s_mainWindow = this;

  // Setup UI controls.
  d->ui->setupUi(this);
  setToolSelectorVisibility();

  d->movingPixmapOverlay = std::make_unique<Util::MovingPixmapOverlay>(centralWidget());

  d->statusBarProgress = new StatusBarProgressWidget{this};
  d->ui->statusBar->addPermanentWidget(d->statusBarProgress);

  setupMenu();
  setupToolSelector();
  setupHelpURLs();

  // Setup window properties.
  setWindowIcon(Util::loadIcon(Q("mkvtoolnix-gui.png"), QList<int>{} << 32 << 48 << 64 << 128 << 256));

  retranslateUi();

  Util::restoreWidgetGeometry(this);

  App::programRunner().setup();

  jobTool()->loadAndStart();

#if defined(HAVE_UPDATE_CHECK)
  silentlyCheckForUpdates();
#else
  d->ui->actionHelpCheckForUpdates->setVisible(false);
#endif  // HAVE_UPDATE_CHECK

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
  Q_D(MainWindow);

  d->ui->statusBar->showMessage(message, 3000);
}

QWidget *
MainWindow::createNotImplementedWidget() {
  Q_D(MainWindow);

  auto widget   = new QWidget{d->ui->tool};
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
  Q_D(MainWindow);

  connect(d->ui->actionGUIExit,                   &QAction::triggered,             this, &MainWindow::close);
  connect(d->ui->actionGUIPreferences,            &QAction::triggered,             this, &MainWindow::editPreferences);

  connect(d->ui->actionHelpFAQ,                   &QAction::triggered,             this, &MainWindow::visitHelpURL);
  connect(d->ui->actionHelpKnownProblems,         &QAction::triggered,             this, &MainWindow::visitHelpURL);
  connect(d->ui->actionHelpMkvmergeDocumentation, &QAction::triggered,             this, &MainWindow::visitMkvmergeDocumentation);
  connect(d->ui->actionHelpWebSite,               &QAction::triggered,             this, &MainWindow::visitHelpURL);
  connect(d->ui->actionHelpReportBug,             &QAction::triggered,             this, &MainWindow::visitHelpURL);

  connect(d->ui->actionWindowNext,                &QAction::triggered,             this, [this]() { showNextOrPreviousSubWindow(1);  });
  connect(d->ui->actionWindowPrevious,            &QAction::triggered,             this, [this]() { showNextOrPreviousSubWindow(-1); });
  connect(d->ui->menuWindow,                      &QMenu::aboutToShow,             this, &MainWindow::setupWindowMenu);

  connect(this,                                   &MainWindow::preferencesChanged, this, &MainWindow::setToolSelectorVisibility);

#if defined(HAVE_UPDATE_CHECK)
  connect(d->ui->actionHelpCheckForUpdates,       &QAction::triggered,             this, &MainWindow::checkForUpdates);
#endif  // HAVE_UPDATE_CHECK
}

void
MainWindow::setupToolSelector() {
  Q_D(MainWindow);

  d->toolMerge         = new Merge::Tool{d->ui->tool,         d->ui->menuMerge};
  d->toolHeaderEditor  = new HeaderEditor::Tool{d->ui->tool,  d->ui->menuHeaderEditor};
  d->toolChapterEditor = new ChapterEditor::Tool{d->ui->tool, d->ui->menuChapterEditor};
  d->toolJobs          = new Jobs::Tool{d->ui->tool,          d->ui->menuJobQueue};
  d->watchJobTool      = new WatchJobs::Tool{d->ui->tool,     d->ui->menuJobOutput};

  d->ui->tool->appendTab(d->toolMerge,                        QIcon{":/icons/48x48/merge.png"},                      QY("Multiplexer"));
  // d->ui->tool->appendTab(createNotImplementedWidget(),        QIcon{":/icons/48x48/split.png"},                      QY("Extractor"));
  // d->ui->tool->appendTab(createNotImplementedWidget(),        QIcon{":/icons/48x48/document-preview-archive.png"},   QY("Info tool"));
  d->ui->tool->appendTab(d->toolHeaderEditor,                 QIcon{":/icons/48x48/document-edit.png"},              QY("Header editor"));
  d->ui->tool->appendTab(d->toolChapterEditor,                QIcon{":/icons/48x48/story-editor.png"},               QY("Chapter editor"));
  // d->ui->tool->appendTab(createNotImplementedWidget(),        QIcon{":/icons/48x48/document-edit-sign-encrypt.png"}, QY("Tags editor"));
  d->ui->tool->appendTab(d->toolJobs,                         QIcon{":/icons/48x48/view-task.png"},                  QY("Job queue"));
  d->ui->tool->appendTab(d->watchJobTool,                     QIcon{":/icons/48x48/system-run.png"},                 QY("Job output"));

  for (auto idx = 0, numTabs = d->ui->tool->count(); idx < numTabs; ++idx) {
    qobject_cast<ToolBase *>(d->ui->tool->widget(idx))->setupUi();
    d->ui->tool->setTabEnabled(idx, true);
  }

  for (auto idx = 0, numTabs = d->ui->tool->count(); idx < numTabs; ++idx)
    qobject_cast<ToolBase *>(d->ui->tool->widget(idx))->setupActions();

  d->ui->tool->setCurrentIndex(0);
  d->toolMerge->toolShown();
  showAndEnableMenu(*d->ui->menuWindow, d->subWindowWidgets.contains(d->toolMerge));

  d->toolSelectionActions << d->ui->actionGUIMergeTool    /* << d->ui->actionGUIExtractionTool << d->ui->actionGUIInfoTool*/
                          << d->ui->actionGUIHeaderEditor << d->ui->actionGUIChapterEditor  /*<< d->ui->actionGUITagEditor*/
                          << d->ui->actionGUIJobQueue     << d->ui->actionGUIJobOutput;

  d->ui->actionGUIExtractionTool->setVisible(false);
  d->ui->actionGUIInfoTool->setVisible(false);
  d->ui->actionGUITagEditor->setVisible(false);

  auto currentJobTab = d->watchJobTool->currentJobTab();

  connect(d->ui->actionGUIMergeTool,      &QAction::triggered,                                    this,                 &MainWindow::changeToolToSender);
  // connect(d->ui->actionGUIExtractionTool, &QAction::triggered,                                    this,                 &MainWindow::changeToolToSender);
  // connect(d->ui->actionGUIInfoTool,       &QAction::triggered,                                    this,                 &MainWindow::changeToolToSender);
  connect(d->ui->actionGUIHeaderEditor,   &QAction::triggered,                                    this,                 &MainWindow::changeToolToSender);
  connect(d->ui->actionGUIChapterEditor,  &QAction::triggered,                                    this,                 &MainWindow::changeToolToSender);
  // connect(d->ui->actionGUITagEditor,      &QAction::triggered,                                    this,                 &MainWindow::changeToolToSender);
  connect(d->ui->actionGUIJobQueue,       &QAction::triggered,                                    this,                 &MainWindow::changeToolToSender);
  connect(d->ui->actionGUIJobOutput,      &QAction::triggered,                                    this,                 &MainWindow::changeToolToSender);

  connect(d->ui->tool,                    &Util::FancyTabWidget::currentChanged,                  this,                 &MainWindow::toolChanged);
  connect(d->toolJobs->model(),           &Jobs::Model::progressChanged,                          d->statusBarProgress, &StatusBarProgressWidget::setProgress);
  connect(d->toolJobs->model(),           &Jobs::Model::jobStatsChanged,                          d->statusBarProgress, &StatusBarProgressWidget::setJobStats);
  connect(d->toolJobs->model(),           &Jobs::Model::numUnacknowledgedWarningsOrErrorsChanged, d->statusBarProgress, &StatusBarProgressWidget::setNumUnacknowledgedWarningsOrErrors);
  connect(currentJobTab,                  &WatchJobs::Tab::watchCurrentJobTabCleared,             d->statusBarProgress, &StatusBarProgressWidget::reset);
}

void
MainWindow::setupHelpURLs() {
  Q_D(MainWindow);

  d->helpURLs[d->ui->actionHelpFAQ]                   = "https://github.com/mbunkus/mkvtoolnix/wiki";
  d->helpURLs[d->ui->actionHelpKnownProblems]         = "https://github.com/mbunkus/mkvtoolnix/wiki/Troubleshooting";
  d->helpURLs[d->ui->actionHelpMkvmergeDocumentation] = "https://mkvtoolnix.download/doc/mkvmerge.html";
  d->helpURLs[d->ui->actionHelpWebSite]               = "https://mkvtoolnix.download/";
  d->helpURLs[d->ui->actionHelpReportBug]             = "https://github.com/mbunkus/mkvtoolnix/issues/";
}

void
MainWindow::showAndEnableMenu(QMenu &menu,
                              bool show) {
  Q_D(MainWindow);

  if (show)
    d->ui->menuBar->insertMenu(d->ui->menuHelp->menuAction(), &menu);
  else
    d->ui->menuBar->removeAction(menu.menuAction());
}

void
MainWindow::showTheseMenusOnly(QList<QMenu *> const &menus) {
  Q_D(MainWindow);

  showAndEnableMenu(*d->ui->menuMerge,         menus.contains(d->ui->menuMerge));
  showAndEnableMenu(*d->ui->menuHeaderEditor,  menus.contains(d->ui->menuHeaderEditor));
  showAndEnableMenu(*d->ui->menuChapterEditor, menus.contains(d->ui->menuChapterEditor));
  showAndEnableMenu(*d->ui->menuJobQueue,      menus.contains(d->ui->menuJobQueue));
  showAndEnableMenu(*d->ui->menuJobOutput,     menus.contains(d->ui->menuJobOutput));
}

void
MainWindow::switchToTool(ToolBase *tool) {
  Q_D(MainWindow);

  for (auto idx = 0, numTabs = d->ui->tool->count(); idx < numTabs; ++idx)
    if (d->ui->tool->widget(idx) == tool) {
      d->ui->tool->setCurrentIndex(idx);
      return;
    }
}

void
MainWindow::changeToolToSender() {
  Q_D(MainWindow);

  auto toolIndex = d->toolSelectionActions.indexOf(static_cast<QAction *>(sender()));
  if (-1 != toolIndex)
    d->ui->tool->setCurrentIndex(toolIndex);
}

void
MainWindow::toolChanged(int index) {
  Q_D(MainWindow);

  showTheseMenusOnly({});

  auto widget   = d->ui->tool->widget(index);
  auto toolBase = dynamic_cast<ToolBase *>(widget);

  if (toolBase) {
    toolBase->toolShown();
    showAndEnableMenu(*d->ui->menuWindow, d->subWindowWidgets.contains(toolBase));
  }
}

MainWindow *
MainWindow::get() {
  return s_mainWindow;
}

Ui::MainWindow *
MainWindow::getUi() {
  return GET_D->ui.get();
}

Merge::Tool *
MainWindow::mergeTool() {
  return GET_D->toolMerge;
}

HeaderEditor::Tool *
MainWindow::headerEditorTool() {
  return GET_D->toolHeaderEditor;
}

ChapterEditor::Tool *
MainWindow::chapterEditorTool() {
  return GET_D->toolChapterEditor;
}

Jobs::Tool *
MainWindow::jobTool() {
  return GET_D->toolJobs;
}

WatchJobs::Tab *
MainWindow::watchCurrentJobTab() {
  return watchJobTool()->currentJobTab();
}

WatchJobs::Tool *
MainWindow::watchJobTool() {
  return GET_D->watchJobTool;
}

void
MainWindow::retranslateUi() {
  Q_D(MainWindow);

  d->ui->retranslateUi(this);
  d->statusBarProgress->retranslateUi();

  setWindowTitle(Q(get_version_info("MKVToolNix GUI")));

  d->ui->tool->setUpdatesEnabled(false);

  // Intentionally replacing the list right away again in order not to
  // lose the translations for the three currently unimplemented
  // tools.
  auto toolTitles = QStringList{} << QY("Extraction tool") << QY("Info tool") << QY("Tag editor");
  toolTitles      = QStringList{} << QY("Multiplexer") << QY("Header editor") << QY("Chapter editor") << QY("Job queue") << QY("Job output");

  for (auto idx = 0, count = d->ui->tool->count(); idx < count; ++idx)
    d->ui->tool->setTabText(idx, toolTitles[idx]);

  // Intentionally setting the menu titles here instead of the
  // designer as the designer doesn't allow the same hotkey in the
  // same form.
  d->ui->menuGUI          ->setTitle(QY("MKVToolNix &GUI"));
  d->ui->menuMerge        ->setTitle(QY("&Multiplexer"));
  d->ui->menuHeaderEditor ->setTitle(QY("Header &editor"));
  d->ui->menuChapterEditor->setTitle(QY("&Chapter editor"));
  d->ui->menuJobQueue     ->setTitle(QY("&Job queue"));
  d->ui->menuJobOutput    ->setTitle(QY("&Job output"));
  d->ui->menuHelp         ->setTitle(QY("&Help"));

  d->ui->tool->setUpdatesEnabled(true);
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
  editPreferencesAndShowPage(PreferencesDialog::Page::Default);
}

void
MainWindow::editRunProgramConfigurations() {
  editPreferencesAndShowPage(PreferencesDialog::Page::RunPrograms);
}

void
MainWindow::editPreferencesAndShowPage(PreferencesDialog::Page page) {
  PreferencesDialog dlg{this, page};
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
#endif  // HAVE_UPDATE_CHECK

void
MainWindow::showIconMovingToTool(QString const &pixmapName,
                                 ToolBase const &tool) {
  Q_D(MainWindow);

  if (Util::Settings::get().m_disableAnimations)
    return;

  for (auto idx = 0, count = d->ui->tool->count(); idx < count; ++idx)
    if (&tool == d->ui->tool->widget(idx)) {
      auto size = 32;
      auto rect = d->ui->tool->tabBar()->tabRect(idx);

      auto from = centralWidget()->mapFromGlobal(QCursor::pos());
      auto to   = QPoint{rect.x() + (rect.width()  - size) / 2,
                         rect.y() + (rect.height() - size) / 2};
      to        = centralWidget()->mapFromGlobal(d->ui->tool->tabBar()->mapToGlobal(to));

      d->movingPixmapOverlay->addMovingPixmap(Q(":/icons/%1x%1/%2").arg(size).arg(pixmapName), from, to);

      return;
    }
}

void
MainWindow::resizeEvent(QResizeEvent *event) {
  Q_D(MainWindow);

  d->movingPixmapOverlay->resize(event->size());
  event->accept();
}

void
MainWindow::visitHelpURL() {
  Q_D(MainWindow);

  if (d->helpURLs.contains(sender()))
    QDesktopServices::openUrl(d->helpURLs[sender()]);
}

void
MainWindow::visitMkvmergeDocumentation() {
  Q_D(MainWindow);

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
    url = d->helpURLs[d->ui->actionHelpMkvmergeDocumentation];

  QDesktopServices::openUrl(url);
}

void
MainWindow::showEvent(QShowEvent *event) {
  emit windowShown();
  event->accept();
}

void
MainWindow::setToolSelectorVisibility() {
  Q_D(MainWindow);

  d->ui->tool->tabBar()->setVisible(Util::Settings::get().m_showToolSelector);
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

std::pair<ToolBase *, QTabWidget *>
MainWindow::currentSubWindowWidget() {
  Q_D(MainWindow);

  auto toolBase = dynamic_cast<ToolBase *>(d->ui->tool->widget(d->ui->tool->currentIndex()));

  if (toolBase && d->subWindowWidgets.contains(toolBase))
    return { toolBase, d->subWindowWidgets[toolBase] };

  return {};
}

void
MainWindow::registerSubWindowWidget(ToolBase &toolBase,
                                    QTabWidget &tabWidget) {
  Q_D(MainWindow);

  d->subWindowWidgets[&toolBase] = &tabWidget;
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
  Q_D(MainWindow);

  auto subWindow = currentSubWindowWidget();

  if (!subWindow.first)
    return;

  auto texts         = subWindow.first->nextPreviousWindowActionTexts();
  auto numSubWindows = subWindow.second->count();
  auto menu          = d->ui->menuWindow;

  d->ui->actionWindowNext->setText(texts.first);
  d->ui->actionWindowPrevious->setText(texts.second);

  d->ui->actionWindowNext->setEnabled(numSubWindows >= 2);
  d->ui->actionWindowPrevious->setEnabled(numSubWindows >= 2);

  for (auto &action : menu->actions())
    if (!mtx::included_in(action, d->ui->actionWindowNext, d->ui->actionWindowPrevious))
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

}}
