#include "common/common_pch.h"

#include <array>

#include "common/list_utils.h"
#include "common/qt.h"
#include "mkvtoolnix-gui/jobs/job.h"
#include "mkvtoolnix-gui/jobs/mux_job.h"
#include "mkvtoolnix-gui/jobs/tool.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/merge/command_line_dialog.h"
#include "mkvtoolnix-gui/merge/tab.h"
#include "mkvtoolnix-gui/merge/tab_p.h"
#include "mkvtoolnix-gui/merge/tool.h"
#include "mkvtoolnix-gui/forms/merge/tab.h"
#include "mkvtoolnix-gui/util/file.h"
#include "mkvtoolnix-gui/util/file_dialog.h"
#include "mkvtoolnix-gui/util/message_box.h"
#include "mkvtoolnix-gui/util/option_file.h"
#include "mkvtoolnix-gui/util/process_x.h"
#include "mkvtoolnix-gui/util/settings.h"
#include "mkvtoolnix-gui/util/widget.h"
#include "mkvtoolnix-gui/watch_jobs/tool.h"

#include <QComboBox>
#include <QDebug>
#include <QDir>
#include <QMenu>
#include <QTreeView>
#include <QInputDialog>
#include <QMessageBox>
#include <QString>
#include <QTimer>

namespace mtx::gui::Merge {

using namespace mtx::gui;

TabPrivate::TabPrivate(QWidget *parent)
  : ui{new Ui::Tab}
  , filesModel{new SourceFileModel{parent}}
  , tracksModel{new TrackModel{parent}}
  , currentlySettingInputControlValues{false}
  , addFilesAction{new QAction{parent}}
  , appendFilesAction{new QAction{parent}}
  , addAdditionalPartsAction{new QAction{parent}}
  , addFilesAction2{new QAction{parent}}
  , appendFilesAction2{new QAction{parent}}
  , addAdditionalPartsAction2{new QAction{parent}}
  , removeFilesAction{new QAction{parent}}
  , removeAllFilesAction{new QAction{parent}}
  , setDestinationFileNameAction{new QAction{parent}}
  , selectAllTracksAction{new QAction{parent}}
  , enableAllTracksAction{new QAction{parent}}
  , disableAllTracksAction{new QAction{parent}}
  , selectAllVideoTracksAction{new QAction{parent}}
  , selectAllAudioTracksAction{new QAction{parent}}
  , selectAllSubtitlesTracksAction{new QAction{parent}}
  , openFilesInMediaInfoAction{new QAction{parent}}
  , openTracksInMediaInfoAction{new QAction{parent}}
  , selectTracksFromFilesAction{new QAction{parent}}
  , enableAllAttachedFilesAction{new QAction{parent}}
  , disableAllAttachedFilesAction{new QAction{parent}}
  , enableSelectedAttachedFilesAction{new QAction{parent}}
  , disableSelectedAttachedFilesAction{new QAction{parent}}
  , startMuxingLeaveAsIs{new QAction{parent}}
  , startMuxingCreateNewSettings{new QAction{parent}}
  , startMuxingCloseSettings{new QAction{parent}}
  , startMuxingRemoveInputFiles{new QAction{parent}}
  , addToJobQueueLeaveAsIs{new QAction{parent}}
  , addToJobQueueCreateNewSettings{new QAction{parent}}
  , addToJobQueueCloseSettings{new QAction{parent}}
  , addToJobQueueRemoveInputFiles{new QAction{parent}}
  , filesMenu{new QMenu{parent}}
  , tracksMenu{new QMenu{parent}}
  , attachedFilesMenu{new QMenu{parent}}
  , attachmentsMenu{new QMenu{parent}}
  , selectTracksOfTypeMenu{new QMenu{parent}}
  , addFilesMenu{new QMenu{parent}}
  , startMuxingMenu{new QMenu{parent}}
  , addToJobQueueMenu{new QMenu{parent}}
  , attachedFilesModel{new AttachedFileModel{parent}}
  , attachmentsModel{new AttachmentModel{parent}}
  , addAttachmentsAction{new QAction{parent}}
  , removeAttachmentsAction{new QAction{parent}}
  , removeAllAttachmentsAction{new QAction{parent}}
  , selectAllAttachmentsAction{new QAction{parent}}
{
}

// ------------------------------------------------------------

Tab::Tab(QWidget *parent)
  : QWidget{parent}
  , p_ptr{new TabPrivate{this}}
{
  auto &p = *p_func();

  // Setup UI controls.
  p.ui->setupUi(this);

  for (auto const &groupBox : findChildren<Util::QgsCollapsibleGroupBox *>()) {
    groupBox->setSettingGroup(Q("mergeTool"));
    groupBox->loadState();
  }

  auto mw = MainWindow::get();
  connect(mw, &MainWindow::preferencesChanged, this, [this]() { Util::setupTabWidgetHeaders(*p_func()->ui->tabs); });

  p.filesModel->setOtherModels(p.tracksModel, p.attachedFilesModel);

  setupInputControls();
  setupOutputControls();
  setupAttachmentsControls();

  setControlValuesFromConfig();

  Util::setupTabWidgetHeaders(*p.ui->tabs);

  retranslateUi();

  Util::fixScrollAreaBackground(p.ui->propertiesScrollArea);
  Util::preventScrollingWithoutFocus(this);

  p.savedState = currentState();
  p.emptyState = p.savedState;

  p.ui->files->setIconSize({ 28, 16 });

  p.ui->trackLanguage->registerBuddyLabel(*p.ui->trackLanguageLabel);
  p.ui->chapterLanguage->registerBuddyLabel(*p.ui->chapterLanguageLabel);
}

Tab::~Tab() {
}

QString const &
Tab::fileName()
  const {
  return p_func()->config.m_configFileName;
}

QString
Tab::title()
  const {
  auto &p    = *p_func();
  auto title = p.config.m_destination.isEmpty() ? QY("<No destination file>") : QFileInfo{p.config.m_destination}.fileName();
  if (!p.config.m_configFileName.isEmpty())
    title = Q("%1 (%2)").arg(title).arg(QFileInfo{p.config.m_configFileName}.fileName());

  return title;
}

void
Tab::onShowCommandLine() {
  auto exe     = Util::Settings::get().actualMkvmergeExe();
  auto options = updateConfigFromControlValues().buildMkvmergeOptions().setExecutable(exe);

  CommandLineDialog{this, options, QY("mkvmerge command line")}.exec();
}

void
Tab::load(QString const &fileName) {
  auto &p = *p_func();

  try {
    if (Util::ConfigFile::determineType(fileName) == Util::ConfigFile::UnknownType)
      throw InvalidSettingsX{};

    p.config.load(fileName);
    setControlValuesFromConfig();

    p.savedState = currentState();

    MainWindow::get()->setStatusBarMessage(QY("The configuration has been loaded."));

    Q_EMIT titleChanged();

  } catch (InvalidSettingsX &) {
    p.config.reset();

    Util::MessageBox::critical(this)->title(QY("Error loading settings file")).text(QY("The settings file '%1' contains invalid settings and was not loaded.").arg(fileName)).exec();

    Q_EMIT removeThisTab();
  }
}

void
Tab::cloneConfig(MuxConfig const &config) {
  auto &p  = *p_func();
  p.config = config;

  setControlValuesFromConfig();

  p.config.m_configFileName.clear();
  p.savedState.clear();

  Q_EMIT titleChanged();
}

void
Tab::onSaveConfig() {
  auto &p = *p_func();

  if (p.config.m_configFileName.isEmpty()) {
    onSaveConfigAs();
    return;
  }

  updateConfigFromControlValues();
  p.config.save();

  p.savedState = currentState();

  MainWindow::get()->setStatusBarMessage(QY("The configuration has been saved."));
}

void
Tab::onSaveOptionFile() {
  auto &settings = Util::Settings::get();
  auto fileName  = Util::getSaveFileName(this, QY("Save option file"), settings.m_lastConfigDir.path(), defaultFileNameForSaving(Q(".json")), QY("MKVToolNix option files (JSON-formatted)") + Q(" (*.json);;") + QY("All files") + Q(" (*)"), Q("json"));
  if (fileName.isEmpty())
    return;

  Util::OptionFile::create(fileName, updateConfigFromControlValues().buildMkvmergeOptions().effectiveOptions(Util::EscapeJSON));
  settings.m_lastConfigDir.setPath(QFileInfo{fileName}.path());
  settings.save();

  MainWindow::get()->setStatusBarMessage(QY("The option file has been created."));
}

void
Tab::onSaveConfigAs() {
  auto &p        = *p_func();
  auto &settings = Util::Settings::get();
  auto fileName  = Util::getSaveFileName(this, QY("Save settings file as"), settings.m_lastConfigDir.path(), defaultFileNameForSaving(Q(".mtxcfg")), QY("MKVToolNix GUI config files") + Q(" (*.mtxcfg);;") + QY("All files") + Q(" (*)"), Q("mtxcfg"));
  if (fileName.isEmpty())
    return;

  updateConfigFromControlValues();
  p.config.save(fileName);
  settings.m_lastConfigDir.setPath(QFileInfo{fileName}.path());
  settings.save();

  p.savedState = currentState();

  Q_EMIT titleChanged();

  MainWindow::get()->setStatusBarMessage(QY("The configuration has been saved."));
}

QString
Tab::defaultFileNameForSaving(QString const &ext) {
  auto &p = *p_func();

  return !p.config.m_destination.isEmpty() ? QFileInfo{p.config.m_destination         }.completeBaseName() + ext
       : !p.config.m_files.isEmpty()       ? QFileInfo{p.config.m_files[0]->m_fileName}.completeBaseName() + ext
       :                                     QString{};
}

QString
Tab::determineInitialDirForOpening(QLineEdit *lineEdit,
                                   InitialDirMode mode)
  const {
  auto &p = *p_func();

  if (lineEdit && !lineEdit->text().isEmpty())
    return Util::dirPath(QFileInfo{ lineEdit->text() }.path());

  if (   (mode == InitialDirMode::ContentFirstInputFileLastOpenDir)
      && !p.config.m_files.isEmpty()
      && !p.config.m_files[0]->m_fileName.isEmpty())
    return Util::dirPath(QFileInfo{ p.config.m_files[0]->m_fileName }.path());

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
  auto dir       = determineInitialDirForOpening(lineEdit, initialDirMode);
  auto fileName  = Util::getOpenFileName(this, title, dir, fullFilter);
  if (fileName.isEmpty())
    return fileName;

  settings.m_lastOpenDir.setPath(QFileInfo{fileName}.path());
  settings.save();

  if (lineEdit)
    lineEdit->setText(fileName);

  return fileName;
}

QString
Tab::determineInitialDirForSaving(QLineEdit *lineEdit)
  const {
  auto &p        = *p_func();
  auto &settings = Util::Settings::get();
  QString dir;

  if (lineEdit && !lineEdit->text().isEmpty())
    dir = QFileInfo{lineEdit->text()}.path();

  else if (settings.m_outputFileNamePolicy == Util::Settings::ToFixedDirectory)
    dir = settings.m_fixedOutputDir.path();

  else if (   (settings.m_outputFileNamePolicy == Util::Settings::ToParentOfFirstInputFile)
           || (settings.m_outputFileNamePolicy == Util::Settings::ToSameAsFirstInputFile)
           || (settings.m_outputFileNamePolicy == Util::Settings::ToRelativeOfFirstInputFile)) {
    if (!p.config.m_files.isEmpty()) {
      auto firstDir = QFileInfo{p.config.m_files.at(0)->m_fileName}.path();
      dir           = settings.m_outputFileNamePolicy == Util::Settings::ToParentOfFirstInputFile ? QFileInfo{firstDir}.path()
                    : settings.m_outputFileNamePolicy == Util::Settings::ToSameAsFirstInputFile   ? firstDir
                    :                                                                               firstDir + Q("/") + settings.m_relativeOutputDir.path();
    }

  } else if (!settings.m_lastOutputDir.path().isEmpty() && (settings.m_lastOutputDir.path() != Q(".")))
    dir = settings.m_lastOutputDir.path();

  qDebug() << "determineInitialDirForSaving()"
           << "dir"      << dir
           << "lineEdit" << (lineEdit ? lineEdit->text() : QString{})
           << "mode"     << settings.m_outputFileNamePolicy
           << "fixed"    << settings.m_fixedOutputDir.path()
           << "relative" << settings.m_relativeOutputDir.path()
           << "lastOut"  << settings.m_lastOutputDir.path();

  return Util::dirPath(dir);
}

QString
Tab::getSaveFileName(QString const &title,
                     QString const &defaultFileName,
                     QString const &filter,
                     QLineEdit *lineEdit,
                     QString const &defaultSuffix) {
  auto fullFilter = filter;
  if (!fullFilter.isEmpty())
    fullFilter += Q(";;");
  fullFilter += QY("All files") + Q(" (*)");

  auto dir      = determineInitialDirForSaving(lineEdit);
  auto fileName = Util::getSaveFileName(this, title, dir, defaultFileName, fullFilter, defaultSuffix);


  if (fileName.isEmpty())
    return fileName;

  auto &settings = Util::Settings::get();
  settings.m_lastOutputDir.setPath(QFileInfo{fileName}.path());
  settings.save();

  lineEdit->setText(fileName);

  return fileName;
}

void
Tab::setControlValuesFromConfig() {
  auto &p = *p_func();

  p.filesModel->setSourceFiles(p.config.m_files);
  p.tracksModel->setTracks(p.config.m_tracks);
  p.attachmentsModel->replaceAttachments(p.config.m_attachments);

  p.attachedFilesModel->reset();
  for (auto const &sourceFile : p.config.m_files) {
    p.attachedFilesModel->addAttachedFiles(sourceFile->m_attachedFiles);

    for (auto const &appendedFile : sourceFile->m_appendedFiles)
      p.attachedFilesModel->addAttachedFiles(appendedFile->m_attachedFiles);
  }

  p.ui->attachedFiles->sortByColumn(0, Qt::AscendingOrder);
  p.attachedFilesModel->sort(0, Qt::AscendingOrder);

  onTrackSelectionChanged();
  setOutputControlValues();
  onAttachmentSelectionChanged();
}

MuxConfig &
Tab::updateConfigFromControlValues() {
  auto &p = *p_func();

  p.config.m_attachments = p.attachmentsModel->attachments();

  return p.config;
}

void
Tab::retranslateUi() {
  auto &p = *p_func();

  p.ui->retranslateUi(this);

  retranslateInputUI();
  retranslateOutputUI();
  retranslateAttachmentsUI();

  Q_EMIT titleChanged();
}

bool
Tab::isReadyForMerging() {
  auto &p = *p_func();

  auto destination = QDir::toNativeSeparators(p.ui->output->text());

  if (destination.isEmpty()) {
    Util::MessageBox::critical(this)->title(QY("Cannot start multiplexing")).text(QY("You have to set the destination file name before you can start multiplexing or add a job to the job queue.")).exec();
    return false;
  }

  auto destinationValid = destination == Util::removeInvalidPathCharacters(destination);

#if defined(SYS_WINDOWS)
  if (destinationValid)
    destinationValid = destination.contains(QRegularExpression{Q("^[a-zA-Z]:[\\\\/]|^\\\\\\\\.+\\.+")});
#endif  // SYS_WINDOWS

  if (!destinationValid) {
    Util::MessageBox::critical(this)->title(QY("Cannot start multiplexing")).text(QY("The destination file name is invalid and must be fixed before you can start multiplexing or add a job to the job queue.")).exec();
    return false;
  }

  return true;
}

QString
Tab::findExistingDestination()
  const {
  auto &p                = *p_func();
  auto nativeDestination = QDir::toNativeSeparators(p.config.m_destination);
  QFileInfo destinationInfo{nativeDestination};

  if (destinationInfo.exists())
    return nativeDestination;

  if (!p.config.isSplittingEnabled())
    return {};

#if defined(SYS_WINDOWS)
  auto rePatternOptions     = QRegularExpression::CaseInsensitiveOption;
#else
  auto rePatternOptions     = QRegularExpression::NoPatternOption;
#endif
  auto destinationBaseName  = QRegularExpression::escape(destinationInfo.completeBaseName());
  auto destinationSuffix    = QRegularExpression::escape(destinationInfo.suffix());
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

  if (!existingDestination.isEmpty() && !MainWindow::jobTool()->checkIfOverwritingExistingFileIsOK(existingDestination))
    return false;

  return MainWindow::jobTool()->checkIfOverwritingExistingJobIsOK(p_func()->config.m_destination, p_func()->config.isSplittingEnabled());
}

bool
Tab::checkIfMissingAudioTrackIsOK() {
  auto policy = Util::Settings::get().m_mergeWarnMissingAudioTrack;
  if (policy == Util::Settings::MergeMissingAudioTrackPolicy::Never)
    return true;

  auto haveAudioTrack = false;

  for (auto const &file : p_func()->config.m_files)
    for (auto const &track : file->m_tracks) {
      if (!track->isAudio())
        continue;

      if (track->m_muxThis)
        return true;

      haveAudioTrack = true;
    }

  if (!haveAudioTrack && (policy == Util::Settings::MergeMissingAudioTrackPolicy::IfAudioTrackPresent))
    return true;

  auto answer = Util::MessageBox::question(this)
    ->title(QY("Create file without audio track"))
    .text(Q("%1 %2")
          .arg(QY("With the current multiplex settings the destination file will not contain an audio track."))
          .arg(QY("Do you want to continue?")))
    .buttonLabel(QMessageBox::Yes, QY("&Create file without audio track"))
    .buttonLabel(QMessageBox::No,  QY("Cancel"))
    .exec();

  return answer == QMessageBox::Yes;
}

void
Tab::ensureAtLeastOneTrackEnabledMaybe() {
  if (!Util::Settings::get().m_mergeEnsureAtLeastOneTrackEnabled)
    return;

  auto &p = *p_func();

  std::array<bool, 3> hasEnabledTrack;
  std::array<TrackPtr, 3> firstTrack;

  for (auto const &sourceFile : p.config.m_files) {
    if (sourceFile->m_appended)
      continue;

    for (auto const &track : sourceFile->m_tracks) {
      if (!mtx::included_in(track->m_type, TrackType::Video, TrackType::Audio, TrackType::Subtitles))
        continue;

      auto idx = track->m_type == TrackType::Video ? 0
               : track->m_type == TrackType::Audio ? 1
               :                                     2;

      if (track->m_trackEnabledFlag == 1)
        hasEnabledTrack[idx] = true;

      if (!firstTrack[idx])
        firstTrack[idx] = track;
    }
  }

  auto modified = false;

  for (auto idx = 0u; idx < hasEnabledTrack.size(); ++idx)
    if (!hasEnabledTrack[idx] && firstTrack[idx]) {
      firstTrack[idx]->m_trackEnabledFlag = true;
      modified                            = true;
    }

  if (modified)
    setControlValuesFromConfig();
}

void
Tab::addToJobQueue(bool startNow,
                   std::optional<Util::Settings::ClearMergeSettingsAction> clearSettings) {
  auto &p = *p_func();

  updateConfigFromControlValues();
  setOutputFileNameMaybe();
  ensureAtLeastOneTrackEnabledMaybe();

  if (   !isReadyForMerging()
      || !checkIfOverwritingIsOK()
      || !checkIfMissingAudioTrackIsOK())
    return;

  auto &cfg      = Util::Settings::get();
  auto newConfig = std::make_shared<MuxConfig>(p.config);
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

  } else if (cfg.m_switchToJobOutputAfterStarting)
    MainWindow::get()->switchToTool(MainWindow::watchJobTool());

  try {
    MainWindow::jobTool()->addJob(std::static_pointer_cast<Jobs::Job>(job));

  } catch (Util::ProcessX const &ex) {
    Util::MessageBox::critical(MainWindow::get())
      ->title(QY("Adding the job to the queue failed"))
      .text(Q(ex.what()))
      .exec();
    return;
  }

  p.savedState = currentState();

  cfg.m_mergeLastOutputDirs.add(QDir::toNativeSeparators(QFileInfo{ p.config.m_destination }.path()));
  cfg.save();

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
    QTimer::singleShot(0, this, [this]() { signalRemovalOfThisTab(); });
    return;
  }

  // Util::Settings::ClearMergeSettingsAction::NewSettings
  MainWindow::mergeTool()->appendNewTab();
  QTimer::singleShot(0, this, [this]() { signalRemovalOfThisTab(); });
}

void
Tab::signalRemovalOfThisTab() {
  Q_EMIT removeThisTab();
}

QString
Tab::currentState() {
  updateConfigFromControlValues();
  return p_func()->config.toString();
}

bool
Tab::hasBeenModified() {
  return currentState() != p_func()->savedState;
}

bool
Tab::isEmpty() {
  return currentState() == p_func()->emptyState;
}

QList<SourceFilePtr> const &
Tab::sourceFiles() {
  return p_func()->config.m_files;
}

QModelIndex
Tab::fileModelIndexForFileNum(unsigned int num) {
  return p_func()->filesModel->index(num, 0);
}

}
