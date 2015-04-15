#include "common/common_pch.h"

#include "common/qt.h"
#include "mkvtoolnix-gui/jobs/mux_job.h"
#include "mkvtoolnix-gui/jobs/tool.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/merge/command_line_dialog.h"
#include "mkvtoolnix-gui/merge/tab.h"
#include "mkvtoolnix-gui/forms/main_window/main_window.h"
#include "mkvtoolnix-gui/forms/merge/tab.h"
#include "mkvtoolnix-gui/util/option_file.h"
#include "mkvtoolnix-gui/util/settings.h"
#include "mkvtoolnix-gui/util/util.h"

#include <QComboBox>
#include <QMenu>
#include <QTreeView>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QString>
#include <QTimer>

namespace mtx { namespace gui { namespace Merge {

using namespace mtx::gui;

Tab::Tab(QWidget *parent)
  : QWidget{parent}
  , ui{new Ui::Tab}
  , m_filesModel{new SourceFileModel{this}}
  , m_tracksModel{new TrackModel{this}}
  , m_currentlySettingInputControlValues{false}
  , m_addFilesAction{new QAction{this}}
  , m_appendFilesAction{new QAction{this}}
  , m_addAdditionalPartsAction{new QAction{this}}
  , m_removeFilesAction{new QAction{this}}
  , m_removeAllFilesAction{new QAction{this}}
  , m_attachmentsModel{new AttachmentModel{this}}
  , m_addAttachmentsAction{new QAction{this}}
  , m_removeAttachmentsAction{new QAction{this}}
{
  // Setup UI controls.
  ui->setupUi(this);

  m_filesModel->setTracksModel(m_tracksModel);

  setupInputControls();
  setupOutputControls();
  setupAttachmentsControls();

  setControlValuesFromConfig();

  retranslateUi();
}

Tab::~Tab() {
}

QString
Tab::title()
  const {
  auto title = m_config.m_destination.isEmpty() ? QY("<no output file>") : QFileInfo{m_config.m_destination}.fileName();
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
    m_config.load(fileName);
    setControlValuesFromConfig();

    MainWindow::get()->setStatusBarMessage(QY("The configuration has been loaded."));

    emit titleChanged();

  } catch (InvalidSettingsX &) {
    m_config.reset();

    QMessageBox::critical(this, QY("Error loading settings file"), QY("The settings file '%1' contains invalid settings and was not loaded.").arg(fileName));

    emit removeThisTab();
  }
}

void
Tab::onSaveConfig() {
  if (m_config.m_configFileName.isEmpty()) {
    onSaveConfigAs();
    return;
  }

  updateConfigFromControlValues();
  m_config.save();

  MainWindow::get()->setStatusBarMessage(QY("The configuration has been saved."));
}

void
Tab::onSaveOptionFile() {
  auto &settings = Util::Settings::get();
  auto fileName  = QFileDialog::getSaveFileName(this, QY("Save option file"), settings.m_lastConfigDir.path(), QY("All files") + Q(" (*)"));
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
  auto fileName  = QFileDialog::getSaveFileName(this, QY("Save settings file as"), settings.m_lastConfigDir.path(), QY("MKVToolnix GUI config files") + Q(" (*.mtxcfg);;") + QY("All files") + Q(" (*)"));
  if (fileName.isEmpty())
    return;

  updateConfigFromControlValues();
  m_config.save(fileName);
  settings.m_lastConfigDir = QFileInfo{fileName}.path();
  settings.save();

  emit titleChanged();

  MainWindow::get()->setStatusBarMessage(QY("The configuration has been saved."));
}

void
Tab::onAddToJobQueue() {
  addToJobQueue(false);
}

void
Tab::onStartMuxing() {
  addToJobQueue(true);
}

QString
Tab::getOpenFileName(QString const &title,
                     QString const &filter,
                     QLineEdit *lineEdit) {
  auto fullFilter = filter;
  if (!fullFilter.isEmpty())
    fullFilter += Q(";;");
  fullFilter += QY("All files") + Q(" (*)");

  auto &settings = Util::Settings::get();
  auto dir       = lineEdit->text().isEmpty() ? settings.m_lastOpenDir.path() : QFileInfo{ lineEdit->text() }.path();
  auto fileName  = QFileDialog::getOpenFileName(this, title, dir, fullFilter);
  if (fileName.isEmpty())
    return fileName;

  settings.m_lastOpenDir = QFileInfo{fileName}.path();
  settings.save();

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
  auto dir       = lineEdit->text().isEmpty() ? settings.m_lastOutputDir.path() : QFileInfo{ lineEdit->text() }.path();
  auto fileName  = QFileDialog::getSaveFileName(this, title, dir, fullFilter);
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

  resizeFilesColumnsToContents();
  resizeTracksColumnsToContents();
  resizeAttachmentsColumnsToContents();

  onFileSelectionChanged();
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
  retranslateAttachmentsUI();

  emit titleChanged();
}

bool
Tab::isReadyForMerging() {
  if (m_config.m_files.isEmpty()) {
    QMessageBox::critical(this, QY("Cannot start merging"), QY("You have to add at least one source file before you can start merging or add a job to the job queue."));
    return false;
  }

  if (m_config.m_destination.isEmpty()) {
    QMessageBox::critical(this, QY("Cannot start merging"), QY("You have to set the output file name before you can start merging or add a job to the job queue."));
    return false;
  }

  return true;
}

void
Tab::addToJobQueue(bool startNow) {
  if (!isReadyForMerging())
    return;

  auto newConfig     = std::make_shared<MuxConfig>(m_config);
  auto job           = std::make_shared<Jobs::MuxJob>(startNow ? Jobs::Job::PendingAuto : Jobs::Job::PendingManual, newConfig);
  job->m_dateAdded   = QDateTime::currentDateTime();
  job->m_description = job->displayableDescription();

  if (!startNow) {
    auto newDescription = QString{};

    while (newDescription.isEmpty()) {
      bool ok = false;
      newDescription = QInputDialog::getText(this, QY("Enter job description"), QY("Please enter the new job's description."), QLineEdit::Normal, job->m_description, &ok);
      if (!ok)
        return;
    }

    job->m_description = newDescription;
  }

  MainWindow::getJobTool()->addJob(std::static_pointer_cast<Jobs::Job>(job));
}

}}}
