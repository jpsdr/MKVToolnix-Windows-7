#include "common/common_pch.h"

#include <QDir>
#include <QFileInfo>
#include <QTimer>

#include "common/kax_element_names.h"
#include "common/qt.h"
#include "mkvtoolnix-gui/info/info_config.h"
#include "mkvtoolnix-gui/jobs/info_job.h"
#include "mkvtoolnix-gui/jobs/info_job_p.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/merge/mux_config.h"
#include "mkvtoolnix-gui/merge/source_file.h"
#include "mkvtoolnix-gui/util/config_file.h"
#include "mkvtoolnix-gui/util/settings.h"

namespace mtx::gui::Jobs {

using namespace mtx::gui;

InfoJobPrivate::InfoJobPrivate(Job::Status pStatus,
                               mtx::gui::Info::InfoConfigPtr const &pConfig)
  : JobPrivate{pStatus}
  , config{pConfig}
{
}

InfoJob::InfoJob(Status status,
                 Info::InfoConfigPtr const &config)
  : Job{*new InfoJobPrivate{status, config}}
{
}

InfoJob::InfoJob(InfoJobPrivate &p)
  : Job{p}
{
}

InfoJob::~InfoJob() {
  auto p = p_func();

  if (p->thread) {
    p->thread->quit();
    p->thread->wait();
  }
}

void
InfoJob::setupInfoJobConnections() {
  auto p = p_func();

  connect(p->thread, &QThread::started,                       p->info,   &Util::KaxInfo::scanStartOfFile);
  connect(p->thread, &QThread::finished,                      p->info,   &QObject::deleteLater);
  connect(p->thread, &QThread::finished,                      p->thread, &QObject::deleteLater);

  connect(p->info,   &Util::KaxInfo::startOfFileScanStarted,  this,      &InfoJob::infoStarted);
  connect(p->info,   &Util::KaxInfo::startOfFileScanFinished, this,      &InfoJob::infoFinished);
  connect(p->info,   &Util::KaxInfo::errorFound,              this,      &InfoJob::showError);
  connect(p->info,   &Util::KaxInfo::progressChanged,         this,      &InfoJob::updateProgress);
}

void
InfoJob::start() {
  auto p = p_func();

  if (p->info)
    return;

  p->thread = new QThread;
  p->info   = new Util::KaxInfo;

  p->info->set_use_gui(p->config->m_destinationFileName.isEmpty());
  p->info->set_source_file_name(to_utf8(p->config->m_sourceFileName));
  p->info->set_destination_file_name(to_utf8(p->config->m_destinationFileName));
  p->info->set_calc_checksums(p->config->m_calcChecksums);
  p->info->set_show_summary(p->config->m_showSummary);
  p->info->set_show_hexdump(p->config->m_showHexdump);
  p->info->set_show_size(p->config->m_showSize);
  p->info->set_show_track_info(p->config->m_showTrackInfo);
  p->info->set_hex_positions(p->config->m_hexPositions);
  p->info->set_hexdump_max_size(p->config->m_hexdumpMaxSize);
  p->info->set_verbosity(p->config->m_verbose);

  p->info->moveToThread(p->thread);

  setupInfoJobConnections();

  p->thread->start();
}

void
InfoJob::infoStarted() {
  auto p = p_func();

  setStatus(Job::Running);
  setProgress(0);

  emit lineRead(QY("Source file name: %1").arg(QDir::toNativeSeparators(p->config->m_sourceFileName)), InfoLine);
  emit lineRead(QY("Destination file name: %1").arg(QDir::toNativeSeparators(p->config->m_destinationFileName)), InfoLine);
}

void
InfoJob::infoFinished(mtx::kax_info_c::result_e result) {
  setStatus(  result == mtx::kax_info_c::result_e::succeeded ? Job::DoneOk
            : result == mtx::kax_info_c::result_e::failed    ? Job::Failed
            :                                                  Job::Aborted);

  if (result == mtx::kax_info_c::result_e::succeeded)
    setProgress(100);

  p_func()->info = nullptr;
}

void
InfoJob::updateProgress(int percentage,
                        QString const &) {
  setProgress(percentage);
}

void
InfoJob::showError(QString const &text) {
  emit lineRead(text, ErrorLine);
}

QString
InfoJob::destinationFileName()
  const {
  return p_func()->config->m_destinationFileName;
}

QString
InfoJob::displayableType()
  const {
  return QY("Info job");
}

QString
InfoJob::displayableDescription()
  const {
  QFileInfo info{p_func()->config->m_sourceFileName};
  return QY("Listing content of file \"%1\" in directory \"%2\"").arg(info.fileName()).arg(QDir::toNativeSeparators(info.dir().path()));
}

bool
InfoJob::isEditable()
  const {
  return false;
}

void
InfoJob::saveJobInternal(Util::ConfigFile &settings)
  const {
  auto p = p_func();

  settings.setValue("jobType", "InfoJob");

  settings.beginGroup("infoConfig");
  p->config->save(settings);
  settings.endGroup();
}

JobPtr
InfoJob::loadInfoJob(Util::ConfigFile &settings) {
  auto config = std::make_shared<Info::InfoConfig>();

  settings.beginGroup("infoConfig");
  config->load(settings);
  settings.endGroup();

  auto job = std::make_shared<InfoJob>(PendingManual, config);
  job->loadJobBasis(settings);

  return std::static_pointer_cast<Job>(job);
}

Info::InfoConfig const &
InfoJob::config()
  const {
  auto p = p_func();

  Q_ASSERT(!!p->config);
  return *p->config;
}

void
InfoJob::abort() {
  auto p = p_func();

  if (p->info)
    p->info->abort();
}

void
InfoJob::runProgramSetupVariables(ProgramRunner::VariableMap &variables)
  const{
  Job::runProgramSetupVariables(variables);

  variables[Q("JOB_TYPE")] << Q("info");
}

}
