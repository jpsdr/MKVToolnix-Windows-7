#include "common/common_pch.h"

#include "common/debugging.h"
#include "common/list_utils.h"
#include "common/qt.h"
#include "mkvtoolnix-gui/jobs/job.h"
#include "mkvtoolnix-gui/jobs/mux_job.h"
#include "mkvtoolnix-gui/jobs/tool.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/merge/command_line_dialog.h"
#include "mkvtoolnix-gui/merge/file_identification_thread.h"
#include "mkvtoolnix-gui/merge/tab.h"
#include "mkvtoolnix-gui/merge/tool.h"
#include "mkvtoolnix-gui/forms/main_window/main_window.h"
#include "mkvtoolnix-gui/forms/merge/tab.h"
#include "mkvtoolnix-gui/util/file.h"
#include "mkvtoolnix-gui/util/file_dialog.h"
#include "mkvtoolnix-gui/util/message_box.h"
#include "mkvtoolnix-gui/util/option_file.h"
#include "mkvtoolnix-gui/util/settings.h"
#include "mkvtoolnix-gui/util/widget.h"
#include "mkvtoolnix-gui/watch_jobs/tool.h"

#include <QComboBox>
#include <QDir>
#include <QMenu>
#include <QTreeView>
#include <QInputDialog>
#include <QMessageBox>
#include <QString>
#include <QTimer>

namespace mtx { namespace gui { namespace Merge {

using namespace mtx::gui;

Tab::Tab(QWidget *parent)
  : QWidget{parent}
  , m_lastAddAppendFileIdx{-1}
  , ui{new Ui::Tab}
  , m_mouseButtonsForFilesToAddDelayed{}
  , m_filesModel{new SourceFileModel{this}}
  , m_tracksModel{new TrackModel{this}}
  , m_currentlySettingInputControlValues{false}
  , m_addFilesAction{new QAction{this}}
  , m_appendFilesAction{new QAction{this}}
  , m_addAdditionalPartsAction{new QAction{this}}
  , m_addFilesAction2{new QAction{this}}
  , m_appendFilesAction2{new QAction{this}}
  , m_addAdditionalPartsAction2{new QAction{this}}
  , m_removeFilesAction{new QAction{this}}
  , m_removeAllFilesAction{new QAction{this}}
  , m_setDestinationFileNameAction{new QAction{this}}
  , m_selectAllTracksAction{new QAction{this}}
  , m_enableAllTracksAction{new QAction{this}}
  , m_disableAllTracksAction{new QAction{this}}
  , m_selectAllVideoTracksAction{new QAction{this}}
  , m_selectAllAudioTracksAction{new QAction{this}}
  , m_selectAllSubtitlesTracksAction{new QAction{this}}
  , m_openFilesInMediaInfoAction{new QAction{this}}
  , m_openTracksInMediaInfoAction{new QAction{this}}
  , m_selectTracksFromFilesAction{new QAction{this}}
  , m_enableAllAttachedFilesAction{new QAction{this}}
  , m_disableAllAttachedFilesAction{new QAction{this}}
  , m_enableSelectedAttachedFilesAction{new QAction{this}}
  , m_disableSelectedAttachedFilesAction{new QAction{this}}
  , m_startMuxingLeaveAsIs{new QAction{this}}
  , m_startMuxingCreateNewSettings{new QAction{this}}
  , m_startMuxingCloseSettings{new QAction{this}}
  , m_startMuxingRemoveInputFiles{new QAction{this}}
  , m_addToJobQueueLeaveAsIs{new QAction{this}}
  , m_addToJobQueueCreateNewSettings{new QAction{this}}
  , m_addToJobQueueCloseSettings{new QAction{this}}
  , m_addToJobQueueRemoveInputFiles{new QAction{this}}
  , m_filesMenu{new QMenu{this}}
  , m_tracksMenu{new QMenu{this}}
  , m_attachedFilesMenu{new QMenu{this}}
  , m_attachmentsMenu{new QMenu{this}}
  , m_selectTracksOfTypeMenu{new QMenu{this}}
  , m_addFilesMenu{new QMenu{this}}
  , m_startMuxingMenu{new QMenu{this}}
  , m_addToJobQueueMenu{new QMenu{this}}
  , m_attachedFilesModel{new AttachedFileModel{this}}
  , m_attachmentsModel{new AttachmentModel{this}}
  , m_addAttachmentsAction{new QAction{this}}
  , m_removeAttachmentsAction{new QAction{this}}
  , m_removeAllAttachmentsAction{new QAction{this}}
  , m_selectAllAttachmentsAction{new QAction{this}}
  , m_identifier{new FileIdentificationThread{this}}
  , m_debugTrackModel{"track_model"}
{
  // Setup UI controls.
  ui->setupUi(this);

  auto mw = MainWindow::get();
  connect(mw, &MainWindow::preferencesChanged, this, &Tab::setupTabPositions);

  m_filesModel->setOtherModels(m_tracksModel, m_attachedFilesModel);

  setupInputControls();
  setupOutputControls();
  setupAttachmentsControls();
  setupTabPositions();
  setupFileIdentificationThread();

  setControlValuesFromConfig();

  retranslateUi();

  Util::fixScrollAreaBackground(ui->propertiesScrollArea);
  Util::preventScrollingWithoutFocus(this);

  m_savedState = currentState();
  m_emptyState = m_savedState;
}

Tab::~Tab() {
  m_identifier->abortPlaylistScan();
  m_identifier->quit();
  m_identifier->wait();
}

QString const &
Tab::fileName()
  const {
  return m_config.m_configFileName;
}

QString
Tab::title()
  const {
  auto title = m_config.m_destination.isEmpty() ? QY("<No destination file>") : QFileInfo{m_config.m_destination}.fileName();
  if (!m_config.m_configFileName.isEmpty())
    title = Q("%1 (%2)").arg(title).arg(QFileInfo{m_config.m_configFileName}.fileName());

  return title;
}

void
Tab::onShowCommandLine() {
  auto options = (QStringList{} << Util::Settings::get().actualMkvmergeExe()) + updateConfigFromControlValues().buildMkvmergeOptions();
  CommandLineDialog{this, options, QY("mkvmerge command line")}.exec();
}

void
Tab::load(QString const &fileName) {
  try {
    if (Util::ConfigFile::determineType(fileName) == Util::ConfigFile::UnknownType)
      throw InvalidSettingsX{};

    m_config.load(fileName);
    setControlValuesFromConfig();

    m_tracksModel->updateEffectiveDefaultTrackFlags();

    m_savedState = currentState();

    MainWindow::get()->setStatusBarMessage(QY("The configuration has been loaded."));

    emit titleChanged();

  } catch (InvalidSettingsX &) {
    m_config.reset();

    Util::MessageBox::critical(this)->title(QY("Error loading settings file")).text(QY("The settings file '%1' contains invalid settings and was not loaded.").arg(fileName)).exec();

    emit removeThisTab();
  }
}

void
Tab::cloneConfig(MuxConfig const &config) {
  m_config = config;

  setControlValuesFromConfig();

  m_config.m_configFileName.clear();
  m_savedState.clear();

  emit titleChanged();
}

void
Tab::onSaveConfig() {
  if (m_config.m_configFileName.isEmpty()) {
    onSaveConfigAs();
    return;
  }

  updateConfigFromControlValues();
  m_config.save();

  m_savedState = currentState();

  MainWindow::get()->setStatusBarMessage(QY("The configuration has been saved."));
}

void
Tab::onSaveOptionFile() {
  auto &settings = Util::Settings::get();
  auto fileName  = Util::getSaveFileName(this, QY("Save option file"), settings.m_lastConfigDir.path(), QY("MKVToolNix option files (JSON-formatted)") + Q(" (*.json);;") + QY("All files") + Q(" (*)"));
  if (fileName.isEmpty())
    return;

  Util::OptionFile::create(fileName, updateConfigFromControlValues().buildMkvmergeOptions());
  settings.m_lastConfigDir = QFileInfo{fileName}.path();
  settings.save();

  MainWindow::get()->setStatusBarMessage(QY("The option file has been created."));
}

void
Tab::onSaveConfigAs() {
  auto &settings = Util::Settings::get();
  auto fileName  = Util::getSaveFileName(this, QY("Save settings file as"), settings.m_lastConfigDir.path(), QY("MKVToolNix GUI config files") + Q(" (*.mtxcfg);;") + QY("All files") + Q(" (*)"));
  if (fileName.isEmpty())
    return;

  updateConfigFromControlValues();
  m_config.save(fileName);
  settings.m_lastConfigDir = QFileInfo{fileName}.path();
  settings.save();

  m_savedState = currentState();

  emit titleChanged();

  MainWindow::get()->setStatusBarMessage(QY("The configuration has been saved."));
}

QString
Tab::determineInitialDir(QLineEdit *lineEdit,
                         InitialDirMode mode)
  const {
  if (lineEdit && !lineEdit->text().isEmpty())
    return Util::dirPath(QFileInfo{ lineEdit->text() }.path());

  if (   (mode == InitialDirMode::ContentFirstInputFileLastOpenDir)
      && !m_config.m_files.isEmpty()
      && !m_config.m_files[0]->m_fileName.isEmpty())
    return Util::dirPath(QFileInfo{ m_config.m_files[0]->m_fileName }.path());

  return Util::Settings::get().lastOpenDirPath();
}

QString
Tab::getOpenFileName(QString const &title,
                     QString const &filter,
                     QLineEdit *lineEdit,
                     InitialDirMode initialDirMode) {
  auto fullFilter = filter;
  if (!fullFilter.isEmpty())
    fullFilter += Q(";;");
  fullFilter += QY("All files") + Q(" (*)");

  auto &settings = Util::Settings::get();
  auto dir       = determineInitialDir(lineEdit, initialDirMode);
  auto fileName  = Util::getOpenFileName(this, title, dir, fullFilter);
  if (fileName.isEmpty())
    return fileName;

  settings.m_lastOpenDir = QFileInfo{fileName}.path();
  settings.save();

  if (lineEdit)
    lineEdit->setText(fileName);

  return fileName;
}


QString
Tab::getSaveFileName(QString const &title,
                     QString const &filter,
                     QLineEdit *lineEdit) {
  auto fullFilter = filter;
  if (!fullFilter.isEmpty())
    fullFilter += Q(";;");
  fullFilter += QY("All files") + Q(" (*)");

  auto &settings = Util::Settings::get();
  auto dir       = !lineEdit->text().isEmpty()                                                               ? lineEdit->text()
                 : !settings.m_lastOutputDir.path().isEmpty() && (settings.m_lastOutputDir.path() != Q(".")) ? Util::dirPath(settings.m_lastOutputDir.path())
                 :                                                                                             Util::dirPath(settings.m_lastOpenDir.path());
  auto fileName  = Util::getSaveFileName(this, title, dir, fullFilter);
  if (fileName.isEmpty())
    return fileName;

  settings.m_lastOutputDir = QFileInfo{fileName}.path();
  settings.save();

  lineEdit->setText(fileName);

  return fileName;
}

void
Tab::setControlValuesFromConfig() {
  m_filesModel->setSourceFiles(m_config.m_files);
  m_tracksModel->setTracks(m_config.m_tracks);
  m_attachmentsModel->replaceAttachments(m_config.m_attachments);

  m_attachedFilesModel->reset();
  for (auto const &sourceFile : m_config.m_files) {
    m_attachedFilesModel->addAttachedFiles(sourceFile->m_attachedFiles);

    for (auto const &appendedFile : sourceFile->m_appendedFiles)
      m_attachedFilesModel->addAttachedFiles(appendedFile->m_attachedFiles);
  }

  ui->attachedFiles->sortByColumn(0, Qt::AscendingOrder);
  m_attachedFilesModel->sort(0, Qt::AscendingOrder);

  resizeFilesColumnsToContents();
  resizeTracksColumnsToContents();
  resizeAttachedFilesColumnsToContents();
  resizeAttachmentsColumnsToContents();

  onTrackSelectionChanged();
  setOutputControlValues();
  onAttachmentSelectionChanged();
}

MuxConfig &
Tab::updateConfigFromControlValues() {
  m_config.m_attachments = m_attachmentsModel->attachments();

  return m_config;
}

void
Tab::retranslateUi() {
  ui->retranslateUi(this);

  retranslateInputUI();
  retranslateOutputUI();
  retranslateAttachmentsUI();

  emit titleChanged();
}

bool
Tab::isReadyForMerging() {
  if (m_config.m_destination.isEmpty()) {
    Util::MessageBox::critical(this)->title(QY("Cannot start multiplexing")).text(QY("You have to set the destination file name before you can start multiplexing or add a job to the job queue.")).exec();
    return false;
  }

  if (m_config.m_destination != Util::removeInvalidPathCharacters(m_config.m_destination)) {
    Util::MessageBox::critical(this)->title(QY("Cannot start multiplexing")).text(QY("The destination file name is invalid and must be fixed before you can start multiplexing or add a job to the job queue.")).exec();
    return false;
  }

  return true;
}

QString
Tab::findExistingDestination()
  const {
  auto nativeDestination = QDir::toNativeSeparators(m_config.m_destination);
  QFileInfo destinationInfo{nativeDestination};

  if (MuxConfig::DoNotSplit == m_config.m_splitMode)
    return destinationInfo.exists() ? nativeDestination : QString{};

#if defined(SYS_WINDOWS)
  auto rePatternOptions     = QRegularExpression::CaseInsensitiveOption;
#else
  auto rePatternOptions     = QRegularExpression::NoPatternOption;
#endif
  auto destinationBaseName  = QRegularExpression::escape(destinationInfo.baseName());
  auto destinationSuffix    = QRegularExpression::escape(destinationInfo.completeSuffix());
  auto splitNameTestPattern = Q("^%1-\\d+%2%3$").arg(destinationBaseName).arg(destinationSuffix.isEmpty() ? Q("") : Q("\\.")).arg(destinationSuffix);
  auto splitNameTestRE      = QRegularExpression{splitNameTestPattern, rePatternOptions};
  auto destinationDir       = destinationInfo.dir();

  for (auto const &existingFileName : destinationDir.entryList(QDir::NoFilter, QDir::Name | QDir::IgnoreCase))
    if (splitNameTestRE.match(existingFileName).hasMatch())
      return QDir::toNativeSeparators(destinationDir.filePath(existingFileName));

  return {};
}

bool
Tab::checkIfOverwritingIsOK() {
  if (!Util::Settings::get().m_warnBeforeOverwriting)
    return true;

  auto existingDestination = findExistingDestination();

  if (!existingDestination.isEmpty()) {
    auto answer = Util::MessageBox::question(this)
      ->title(QY("Overwrite existing file"))
      .text(Q("%1 %2")
            .arg(QY("The file '%1' exists already.").arg(existingDestination))
            .arg(QY("Do you want to overwrite the file?")))
      .buttonLabel(QMessageBox::Yes, QY("&Overwrite file"))
      .buttonLabel(QMessageBox::No,  QY("Cancel"))
      .exec();
    if (answer != QMessageBox::Yes)
      return false;
  }

  auto nativeDestination            = QDir::toNativeSeparators(m_config.m_destination);
  auto jobWithSameDestinationExists = false;

  MainWindow::jobTool()->model()->withAllJobs([&jobWithSameDestinationExists, &nativeDestination](Jobs::Job &job) {
    auto muxJob = qobject_cast<Jobs::MuxJob *>(&job);

    if (   muxJob
        && mtx::included_in(job.status(), Jobs::Job::PendingManual, Jobs::Job::PendingAuto, Jobs::Job::Running)
        && (QDir::toNativeSeparators(muxJob->config().m_destination) == nativeDestination))
      jobWithSameDestinationExists = true;
  });

  if (jobWithSameDestinationExists) {
    auto answer = Util::MessageBox::question(this)
      ->title(QY("Overwrite existing file"))
      .text(Q("%1 %2 %3")
            .arg(QY("A job creating the file '%1' is already in the job queue.").arg(m_config.m_destination))
            .arg(QY("If you add another job with the same destination file then file created before will be overwritten."))
            .arg(QY("Do you want to overwrite the file?")))
      .buttonLabel(QMessageBox::Yes, QY("&Overwrite file"))
      .buttonLabel(QMessageBox::No,  QY("Cancel"))
      .exec();
    if (answer != QMessageBox::Yes)
      return false;
  }

  return true;
}

void
Tab::addToJobQueue(bool startNow,
                   boost::optional<Util::Settings::ClearMergeSettingsAction> clearSettings) {
  updateConfigFromControlValues();
  setOutputFileNameMaybe();

  if (!isReadyForMerging() || !checkIfOverwritingIsOK())
    return;

  auto &cfg      = Util::Settings::get();
  auto newConfig = std::make_shared<MuxConfig>(m_config);
  auto job       = std::make_shared<Jobs::MuxJob>(startNow ? Jobs::Job::PendingAuto : Jobs::Job::PendingManual, newConfig);

  job->setDateAdded(QDateTime::currentDateTime());
  job->setDescription(job->displayableDescription());

  if (!startNow) {
    if (!cfg.m_useDefaultJobDescription) {
      auto newDescription = QString{};

      while (newDescription.isEmpty()) {
        bool ok = false;
        newDescription = QInputDialog::getText(this, QY("Enter job description"), QY("Please enter the new job's description."), QLineEdit::Normal, job->description(), &ok);
        if (!ok)
          return;
      }

      job->setDescription(newDescription);
    }

    MainWindow::get()->showIconMovingToTool(Q("task-delegate.png"), *MainWindow::jobTool());

  } else {
    if (cfg.m_switchToJobOutputAfterStarting)
      MainWindow::get()->switchToTool(MainWindow::watchJobTool());
    else
      MainWindow::get()->showIconMovingToTool(Q("media-playback-start.png"), *MainWindow::watchJobTool());
  }

  MainWindow::jobTool()->addJob(std::static_pointer_cast<Jobs::Job>(job));

  m_savedState = currentState();

  auto action = clearSettings ? *clearSettings : Util::Settings::get().m_clearMergeSettings;
  handleClearingMergeSettings(action);
}

void
Tab::handleClearingMergeSettings(Util::Settings::ClearMergeSettingsAction action) {
  if (Util::Settings::ClearMergeSettingsAction::None == action)
    return;

  if (Util::Settings::ClearMergeSettingsAction::RemoveInputFiles == action) {
    onRemoveAllFiles();
    return;
  }

  if (Util::Settings::ClearMergeSettingsAction::CloseSettings == action) {
    QTimer::singleShot(0, this, SLOT(signalRemovalOfThisTab()));
    return;
  }

  // Util::Settings::ClearMergeSettingsAction::NewSettings
  MainWindow::mergeTool()->newConfig();
  QTimer::singleShot(0, this, SLOT(signalRemovalOfThisTab()));
}

void
Tab::signalRemovalOfThisTab() {
  emit removeThisTab();
}

QString
Tab::currentState() {
  updateConfigFromControlValues();
  return m_config.toString();
}

bool
Tab::hasBeenModified() {
  return currentState() != m_savedState;
}

bool
Tab::isEmpty() {
  return currentState() == m_emptyState;
}

void
Tab::setupTabPositions() {
  ui->tabs->setTabPosition(Util::Settings::get().m_tabPosition);
}

}}}
