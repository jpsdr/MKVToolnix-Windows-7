#include "common/common_pch.h"

#include <QAbstractItemView>
#include <QDebug>
#include <QMutexLocker>
#include <QSettings>
#include <QTimer>

#include "common/list_utils.h"
#include "common/qt.h"
#include "common/sorting.h"
#include "mkvtoolnix-gui/app.h"
#include "mkvtoolnix-gui/jobs/model.h"
#include "mkvtoolnix-gui/jobs/mux_job.h"
#include "mkvtoolnix-gui/jobs/program_runner.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/merge/mux_config.h"
#include "mkvtoolnix-gui/util/date_time.h"
#include "mkvtoolnix-gui/util/ini_config_file.h"
#include "mkvtoolnix-gui/util/model.h"
#include "mkvtoolnix-gui/util/settings.h"
#include "mkvtoolnix-gui/watch_jobs/tab.h"
#include "mkvtoolnix-gui/watch_jobs/tool.h"

namespace mtx::gui::Jobs {

namespace {

QString
formatStatusForSorting(JobPtr const &job) {
  auto value = job->status() == Job::Status::PendingManual ? 0
             : job->status() == Job::Status::PendingAuto   ? 1
             : job->status() == Job::Status::Running       ? 2
             : job->status() == Job::Status::DoneOk        ? 3
             : job->status() == Job::Status::DoneWarnings  ? 4
             : job->status() == Job::Status::Failed        ? 5
             : job->status() == Job::Status::Aborted       ? 6
             : job->status() == Job::Status::Disabled      ? 7
             :                                               8;

  return QString::number(value);
}

QString
formatWarningsErrorForSorting(JobPtr const &job) {
  auto value = !job->errors().isEmpty()   ? 2
             : !job->warnings().isEmpty() ? 1
             :                              0;

  return QString::number(value);
}

QString
formatDescriptionForSorting(JobPtr const &job) {
  return job->displayableDescription();
}

QString
formatTypeForSorting(JobPtr const &job) {
  return job->displayableType();
}

QString
formatProgressForSorting(JobPtr const &job) {
  return Q(fmt::format("{0:04}", job->progress()));
}

QString
formatDateAddedForSorting(JobPtr const &job) {
  return Util::displayableDate(job->dateAdded());
}

QString
formatDateStartedForSorting(JobPtr const &job) {
  return Util::displayableDate(job->dateStarted());
}

QString
formatDateFinishedForSorting(JobPtr const &job) {
  return Util::displayableDate(job->dateFinished());
}

} // anonymous namespace

Model::Model(QObject *parent)
  : QStandardItemModel{parent}
  , m_mutex{}
  , m_warningsIcon{Util::fixStandardItemIcon(QIcon::fromTheme(Q("dialog-warning")))}
  , m_errorsIcon{Util::fixStandardItemIcon(QIcon::fromTheme(Q("dialog-error")))}
  , m_started{}
  , m_dontStartJobsNow{}
  , m_running{}
  , m_queueNumDone{}
{
  retranslateUi();

  connect(this, &Model::queueStatusChanged, this, &Model::runProgramOnQueueStop);
}

Model::~Model() {
}

void
Model::retranslateUi() {
  QMutexLocker locked{&m_mutex};

  Util::setDisplayableAndSymbolicColumnNames(*this, {
    { QY("Status"),        Q("status")         },
    { Q(""),               Q("warningsErrors") },
    { QY("Description"),   Q("description")    },
    { QY("Type"),          Q("type")           },
    { QY("Progress"),      Q("progress")       },
    { QY("Date added"),    Q("dateAdded")      },
    { QY("Date started"),  Q("dateStarted")    },
    { QY("Date finished"), Q("dateFinished")   },
  });

  horizontalHeaderItem(StatusIconColumn)->setIcon(QIcon::fromTheme(Q("dialog-warning-grayscale")));
  horizontalHeaderItem(StatusIconColumn)->setData(QY("Warnings/Errors"), Util::HiddenDescriptionRole);

  horizontalHeaderItem(DescriptionColumn) ->setTextAlignment(Qt::AlignLeft  | Qt::AlignVCenter);
  horizontalHeaderItem(ProgressColumn)    ->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
  horizontalHeaderItem(DateAddedColumn)   ->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
  horizontalHeaderItem(DateStartedColumn) ->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
  horizontalHeaderItem(DateFinishedColumn)->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

  for (auto row = 0, numRows = rowCount(); row < numRows; ++row) {
    auto idx = index(row, 0);
    setRowText(itemsForRow(idx), *m_jobsById[ data(idx, Util::JobIdRole).value<uint64_t>() ]);
  }
}

QList<Job *>
Model::selectedJobs(QAbstractItemView *view) {
  QMutexLocker locked{&m_mutex};

  QList<Job *> jobs;
  Util::withSelectedIndexes(view, [this, &jobs](auto const &idx) {
    jobs << m_jobsById[ this->data(idx, Util::JobIdRole).template value<uint64_t>() ].get();
  });

  return jobs;
}

uint64_t
Model::idFromRow(int row)
  const {
  auto itm = item(row);
  return itm ? itm->data(Util::JobIdRole).value<uint64_t>() : 0;
}

int
Model::rowFromId(uint64_t id)
  const {
  for (auto row = 0, numRows = rowCount(); row < numRows; ++row)
    if (idFromRow(row) == id)
      return row;
  return RowNotFound;
}

Job *
Model::fromId(uint64_t id)
  const {
  return m_jobsById.contains(id) ? m_jobsById[id].get() : nullptr;
}


bool
Model::hasJobs()
  const {
  return !!rowCount();
}

bool
Model::hasRunningJobs() {
  QMutexLocker locked{&m_mutex};

  for (auto const &job : m_jobsById)
    if (Job::Running == job->status())
      return true;

  return false;
}

bool
Model::isRunning()
  const {
  return m_running;
}

void
Model::setRowText(QList<QStandardItem *> const &items,
                  Job const &job)
  const {
  items.at(StatusColumn)      ->setText(Job::displayableStatus(job.status()));
  items.at(DescriptionColumn) ->setText(job.description());
  items.at(TypeColumn)        ->setText(job.displayableType());
  items.at(ProgressColumn)    ->setText(to_qs(fmt::format("{0}%", job.progress())));
  items.at(DateAddedColumn)   ->setText(Util::displayableDate(job.dateAdded()));
  items.at(DateStartedColumn) ->setText(Util::displayableDate(job.dateStarted()));
  items.at(DateFinishedColumn)->setText(Util::displayableDate(job.dateFinished()));

  items[DescriptionColumn ]->setTextAlignment(Qt::AlignLeft  | Qt::AlignVCenter);
  items[ProgressColumn    ]->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
  items[DateAddedColumn   ]->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
  items[DateStartedColumn ]->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
  items[DateFinishedColumn]->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

  auto numWarnings = job.numUnacknowledgedWarnings();
  auto numErrors   = job.numUnacknowledgedErrors();

  items[StatusIconColumn]->setIcon(numErrors ? m_errorsIcon : numWarnings ? m_warningsIcon : QIcon{});
}

QList<QStandardItem *>
Model::itemsForRow(QModelIndex const &idx) {
  auto rowItems = QList<QStandardItem *>{};

  for (auto column = 0; 8 > column; ++column)
    rowItems << itemFromIndex(idx.sibling(idx.row(), column));

  return rowItems;
}

QList<QStandardItem *>
Model::createRow(Job const &job)
  const {
  auto items = QList<QStandardItem *>{};
  for (auto idx = 0; idx < 8; ++idx)
    items << new QStandardItem{};
  setRowText(items, job);

  items[0]->setData(QVariant::fromValue(job.id()), Util::JobIdRole);

  return items;
}

void
Model::withSelectedJobs(QAbstractItemView *view,
                        std::function<void(Job &)> const &worker) {
  QMutexLocker locked{&m_mutex};

  auto jobs = selectedJobs(view);
  for (auto const &job : jobs)
    worker(*job);
}

void
Model::withSelectedJobsAsList(QAbstractItemView *view,
                              std::function<void(QList<Job *> const &)> const &worker) {
  QMutexLocker locked{&m_mutex};
  worker(selectedJobs(view));
}

void
Model::withAllJobs(std::function<void(Job &)> const &worker) {
  QMutexLocker locked{&m_mutex};

  for (auto row = 0, numRows = rowCount(); row < numRows; ++row)
    worker(*m_jobsById[idFromRow(row)]);
}

void
Model::withJob(uint64_t id,
               std::function<void(Job &)> const &worker) {
  QMutexLocker locked{&m_mutex};

  if (m_jobsById.contains(id))
    worker(*m_jobsById[id]);
}

void
Model::removeJobsIf(std::function<bool(Job const &)> predicate) {
  QMutexLocker locked{&m_mutex};

  auto toBeRemoved = QHash<Job const *, bool>{};

  for (auto row = rowCount(); 0 < row; --row) {
    auto job = m_jobsById[idFromRow(row - 1)].get();

    if (predicate(*job)) {
      job->removeQueueFile();
      m_jobsById.remove(job->id());
      toBeRemoved[job] = true;
      removeRow(row - 1);
    }
  }

  auto const keys = toBeRemoved.keys();
  for (auto const &job : keys)
    m_toBeProcessed.remove(job);

  updateProgress();
  updateJobStats();
  updateNumUnacknowledgedWarningsOrErrors();
}

void
Model::add(JobPtr const &job) {
  QMutexLocker locked{&m_mutex};

  m_jobsById[job->id()] = job;

  updateJobStats();
  updateNumUnacknowledgedWarningsOrErrors();

  if (job->isToBeProcessed()) {
    m_toBeProcessed.insert(job.get());
    updateProgress();
  }

  invisibleRootItem()->appendRow(createRow(*job));

  connect(job.get(), &Job::progressChanged,                          this, &Model::onProgressChanged);
  connect(job.get(), &Job::statusChanged,                            this, &Model::onStatusChanged);
  connect(job.get(), &Job::numUnacknowledgedWarningsOrErrorsChanged, this, &Model::onNumUnacknowledgedWarningsOrErrorsChanged);

  if (m_dontStartJobsNow)
    return;

  startNextAutoJob();
}

void
Model::onStatusChanged(uint64_t id,
                       mtx::gui::Jobs::Job::Status oldStatus,
                       mtx::gui::Jobs::Job::Status newStatus) {
  QMutexLocker locked{&m_mutex};

  auto row = rowFromId(id);
  if (row == RowNotFound)
    return;

  auto &job   = *m_jobsById[id];
  auto status = job.status();

  // qDebug() << "Model::onStatusChanged id" << id << "old" << static_cast<int>(oldStatus) << "new" << newStatus;

  if (job.isToBeProcessed())
    m_toBeProcessed.insert(&job);

  if (included_in(status, Job::PendingManual, Job::PendingAuto, Job::Running))
    job.setDateFinished(QDateTime{});

  item(row, StatusColumn)->setText(Job::displayableStatus(status));
  item(row, DateStartedColumn)->setText(Util::displayableDate(job.dateStarted()));
  item(row, DateFinishedColumn)->setText(Util::displayableDate(job.dateFinished()));

  if ((Job::Running == status) && !m_running) {
    m_running        = true;
    m_queueStartTime = QDateTime::currentDateTime();
    m_queueNumDone   = 0;

    qDebug() << "onStatusChanged emitting queueStatusChanged(Running)";
    Q_EMIT queueStatusChanged(QueueStatus::Running);
  }

  if ((Job::Running == oldStatus) && (Job::Running != newStatus))
    ++m_queueNumDone;

  updateProgress();

  if (newStatus != Job::Running)
    job.saveQueueFile();

  startNextAutoJob();

  processAutomaticJobRemoval(id, status);
}

void
Model::removeScheduledJobs() {
  QMutexLocker locked{&m_mutex};

  removeJobsIf([this](Job const &job) { return m_toBeRemoved[job.id()]; });
  m_toBeRemoved.clear();
}

void
Model::scheduleJobForRemoval(uint64_t id) {
  QMutexLocker locked{&m_mutex};

  m_toBeRemoved[id] = true;
  QTimer::singleShot(0, this, [this]() { removeScheduledJobs(); });
}

bool
Model::canJobBeRemovedAccordingToPolicy(Job::Status status,
                                        Util::Settings::JobRemovalPolicy policy) {
  if (policy == Util::Settings::JobRemovalPolicy::Never)
    return false;

  bool doneOk       = Job::DoneOk       == status;
  bool doneWarnings = Job::DoneWarnings == status;
  bool done         = doneOk || doneWarnings || (Job::Failed == status) || (Job::Aborted == status);

  if (   ((policy == Util::Settings::JobRemovalPolicy::IfSuccessful)    && doneOk)
      || ((policy == Util::Settings::JobRemovalPolicy::IfWarningsFound) && (doneOk || doneWarnings))
      || ((policy == Util::Settings::JobRemovalPolicy::Always)          && done))
    return true;

  return false;
}

void
Model::processAutomaticJobRemoval(uint64_t id,
                                  Job::Status status) {
  if (canJobBeRemovedAccordingToPolicy(status, Util::Settings::get().m_jobRemovalPolicy))
    scheduleJobForRemoval(id);
}

void
Model::onProgressChanged(uint64_t id,
                         unsigned int progress) {
  QMutexLocker locked{&m_mutex};

  auto row = rowFromId(id);
  if (row < rowCount()) {
    item(row, ProgressColumn)->setText(to_qs(fmt::format("{0}%", progress)));
    updateProgress();
  }
}

void
Model::onNumUnacknowledgedWarningsOrErrorsChanged(uint64_t id,
                                                  int,
                                                  int) {
  QMutexLocker locked{&m_mutex};

  auto row = rowFromId(id);
  if (-1 != row)
    setRowText(itemsForRow(index(row, 0)), *m_jobsById[id]);

  updateNumUnacknowledgedWarningsOrErrors();
}

void
Model::updateNumUnacknowledgedWarningsOrErrors() {
  auto const appStart     = App::instance()->appStartTimestamp();
  auto numCurrentWarnings = 0;
  auto numCurrentErrors   = 0;
  auto numOldWarnings     = 0;
  auto numOldErrors       = 0;

  for (auto const &job : m_jobsById) {
    if (job->dateStarted().isValid() && (job->dateStarted() >= appStart)) {
      numCurrentWarnings += job->numUnacknowledgedWarnings();
      numCurrentErrors   += job->numUnacknowledgedErrors();

    } else {
      numOldWarnings += job->numUnacknowledgedWarnings();
      numOldErrors   += job->numUnacknowledgedErrors();
    }
  }

  Q_EMIT numUnacknowledgedWarningsOrErrorsChanged(numOldWarnings, numCurrentWarnings, numOldErrors, numCurrentErrors);
}

void
Model::startJobInSingleJobMode(Job &job) {
  MainWindow::watchCurrentJobTab()->connectToJob(job);

  job.start();

  updateJobStats();
}

void
Model::startJobsInMultiJobMode(QVector<Job *> const &jobs,
                               unsigned int numRunning) {
  auto maxConcurrent = Util::Settings::get().m_maximumConcurrentJobs;

  qDebug() << "startJobsInMultiJobMode numRunning" << numRunning << "maxConcurrent" << maxConcurrent;

  if (numRunning >= maxConcurrent)
    return;

  auto numToStart = std::min<unsigned int>(jobs.size(), maxConcurrent - numRunning);

  qDebug() << "startJobsInMultiJobMode numToStart" << numToStart;

  for (auto idx = 0u; idx < numToStart; ++idx)
    startJobImmediately(*jobs[idx]);
}

void
Model::cleanupAtEndOfQueue() {
  qDebug() << "cleanupAtEndOfQueue";

  // All jobs are done. Clear total progress.
  m_toBeProcessed.clear();
  updateProgress();
  updateJobStats();

  auto wasRunning = m_running;
  m_running       = false;

  if (wasRunning) {
    qDebug() << "cleanupAtEndOfQueue emitting queueStatsChanged(Stopped)";
    Q_EMIT queueStatusChanged(QueueStatus::Stopped);
  }
}

void
Model::startNextAutoJob() {
  if (m_dontStartJobsNow)
    return;

  QMutexLocker locked{&m_mutex};

  updateJobStats();

  if (!m_started)
    return;

  QVector<Job *> toStart;
  auto numRunning = 0u;

  for (auto row = 0, numRows = rowCount(); row < numRows; ++row) {
    auto job    = m_jobsById[idFromRow(row)].get();
    auto status = job->status();

    if (Job::PendingAuto == status)
      toStart << job;

    else if (Job::Running == status)
      ++numRunning;
  }

  qDebug() << "startNextAutoJob numRunning" << numRunning << "toStart" << toStart;

  if (toStart.isEmpty()) {
    if (!numRunning)
      cleanupAtEndOfQueue();
    return;
  }

  if (Util::Settings::get().m_maximumConcurrentJobs > 1) {
    startJobsInMultiJobMode(toStart, numRunning);
    return;
  }

  if (!numRunning)
    startJobInSingleJobMode(*toStart[0]);
}

void
Model::startJobImmediately(Job &job) {
  QMutexLocker locked{&m_mutex};

  qDebug() << "startJobImmediately" << &job;

  MainWindow::watchCurrentJobTab()->disconnectFromJob(job);
  MainWindow::watchJobTool()->viewOutput(job);

  job.setProgress(0);
  job.start();
  updateJobStats();
}

void
Model::start() {
  m_started = true;
  startNextAutoJob();
}

void
Model::stop() {
  m_started = false;

  auto wasRunning = m_running;
  m_running       = false;
  if (wasRunning) {
    qDebug() << "stop emitting queueStatusChanged(Stopped)";
    Q_EMIT queueStatusChanged(QueueStatus::Stopped);
  }
}

void
Model::resetTotalProgress() {
  QMutexLocker locked{&m_mutex};

  if (isRunning() || !m_queueNumDone)
    return;

  m_queueNumDone = 0;

  Q_EMIT progressChanged(0, 0);
}

void
Model::updateProgress() {
  QMutexLocker locked{&m_mutex};

  if (m_toBeProcessed.isEmpty())
    return;

  auto numRunning       = 0;
  auto numPendingAuto   = 0;
  auto runningProgress  = 0;

  for (auto const &job : m_toBeProcessed)
    if (Job::Running == job->status()) {
      ++numRunning;
      runningProgress += job->progress();

    } else if (Job::PendingAuto == job->status())
      ++numPendingAuto;

  if ((m_queueNumDone + numRunning + numPendingAuto) == 0)
    return;

  auto progress      = numRunning ? runningProgress / numRunning : 0u;
  auto totalProgress = (m_queueNumDone * 100 + runningProgress) / (m_queueNumDone + numRunning + numPendingAuto);

  qDebug() << "updateProgress: total" << totalProgress << "numDone" << m_queueNumDone << "numRunning" << numRunning << "numPendingAuto" << numPendingAuto << "runningProgress" << runningProgress;

  Q_EMIT progressChanged(progress, totalProgress);
}

void
Model::updateJobStats() {
  QMutexLocker locked{&m_mutex};

  auto numJobs = std::map<Job::Status, int>{ { Job::PendingAuto, 0 }, { Job::PendingManual, 0 }, { Job::Running, 0 }, { Job::Disabled, 0 } };

  for (auto const &job : m_jobsById) {
    auto idx = mtx::included_in(job->status(), Job::PendingAuto, Job::PendingManual, Job::Running) ? job->status() : Job::Disabled;
    ++numJobs[idx];
  }

  Q_EMIT jobStatsChanged(numJobs[ Job::PendingAuto ], numJobs[ Job::PendingManual ], numJobs[ Job::Running ], numJobs[ Job::Disabled ]);
}

void
Model::convertJobQueueToSeparateIniFiles() {
  auto reg = Util::Settings::registry();

  reg->beginGroup("jobQueue");

  auto order        = reg->value("order").toStringList();
  auto numberOfJobs = reg->value("numberOfJobs", 0).toUInt();

  reg->endGroup();

  for (auto idx = 0u; idx < numberOfJobs; ++idx) {
    reg->beginGroup("jobQueue");
    reg->beginGroup(Q("job %1").arg(idx));

    try {
      Util::IniConfigFile cfg{*reg};
      auto job = Job::loadJob(cfg);
      if (!job)
        continue;

      job->saveQueueFile();
      order << job->uuid().toString();

    } catch (Merge::InvalidSettingsX &) {
    }

    while (!reg->group().isEmpty())
      reg->endGroup();
  }

  reg->beginGroup("jobQueue");
  reg->remove("numberOfJobs");
  for (auto idx = 0u; idx < numberOfJobs; ++idx)
    reg->remove(Q("job %1").arg(idx));
  reg->setValue("order", order);
  reg->endGroup();
}

void
Model::removeCompletedJobsBeforeExiting() {
  auto policy = Util::Settings::get().m_jobRemovalOnExitPolicy;

  removeJobsIf([policy](auto const &job) { return canJobBeRemovedAccordingToPolicy(job.status(), policy); });
}

void
Model::saveJobs() {
  QMutexLocker locked{&m_mutex};

  // If removal of old jobs is activated then don't save a job
  // that's been run and whose "added" date is more than the
  // configured number of days in the past.
  auto &cfg = Util::Settings::get();

  if (cfg.m_removeOldJobs) {
    auto now              = QDateTime::currentDateTime();
    auto autoRemovalMsecs = static_cast<qint64>(cfg.m_removeOldJobsDays) * 1000 * 60 * 60 * 24;

    removeJobsIf([now, autoRemovalMsecs](Job const &job) {
      return mtx::included_in(job.status(), Job::DoneOk, Job::DoneWarnings, Job::Failed, Job::Aborted)
          && (job.dateAdded().msecsTo(now) >= autoRemovalMsecs);
    });
  }

  auto order = QStringList{};

  for (auto row = 0, numRows = rowCount(); row < numRows; ++row) {
    auto &job = m_jobsById[idFromRow(row)];

    job->saveQueueFile();
    order << job->uuid().toString();
  }

  auto reg = Util::Settings::registry();
  reg->beginGroup("jobQueue");
  reg->setValue("order", order);
  reg->endGroup();
}

void
Model::loadJobs() {
  QMutexLocker locked{&m_mutex};

  m_dontStartJobsNow = true;

  m_jobsById.clear();
  m_toBeProcessed.clear();
  removeRows(0, rowCount());

  auto order       = Util::Settings::registry()->value("jobQueue/order").toStringList();
  auto orderByUuid = QHash<QString, int>{};
  auto orderIdx    = 0;

  for (auto const &uuid : order)
    orderByUuid[uuid] = orderIdx++;

  auto queueLocation = Job::queueLocation();
  auto jobQueueFiles = QDir{queueLocation}.entryList(QStringList{} << Q("*.mtxcfg"), QDir::Files);
  auto knownJobs     = QList<JobPtr>{};
  auto unknownJobs   = QList<JobPtr>{};

  for (auto const &fileName : jobQueueFiles) {
    try {
      auto job = Job::loadJob(Q("%1/%2").arg(queueLocation).arg(fileName));
      if (!job)
        continue;

      auto uuid = job->uuid().toString();

      if (orderByUuid.contains(uuid))
        knownJobs << job;
      else
        unknownJobs << job;

    } catch (Merge::InvalidSettingsX &) {
    }
  }

  mtx::sort::by(knownJobs.begin(),   knownJobs.end(),   [&orderByUuid](JobPtr const &job) { return orderByUuid[ job->uuid().toString() ];   });
  mtx::sort::by(unknownJobs.begin(), unknownJobs.end(),             [](JobPtr const &job) { return Util::displayableDate(job->dateAdded()); });

  for (auto const &job : knownJobs)
    add(job);

  for (auto const &job : unknownJobs)
    add(job);

  updateProgress();

  m_dontStartJobsNow = false;
}

Qt::DropActions
Model::supportedDropActions()
  const {
  return Qt::MoveAction;
}

Qt::ItemFlags
Model::flags(QModelIndex const &index)
  const {
  auto defaultFlags = QStandardItemModel::flags(index) & ~Qt::ItemIsDropEnabled;
  return index.isValid() ? defaultFlags | Qt::ItemIsDragEnabled : defaultFlags | Qt::ItemIsDropEnabled;
}

bool
Model::canDropMimeData(QMimeData const *data,
                       Qt::DropAction action,
                       int row,
                       int,
                       QModelIndex const &parent)
  const {
  if (   !data
      || (Qt::MoveAction != action)
      || parent.isValid()
      || (0 > row))
    return false;

  return true;
}

bool
Model::dropMimeData(QMimeData const *data,
                    Qt::DropAction action,
                    int row,
                    int column,
                    QModelIndex const &parent) {
  if (!canDropMimeData(data, action, row, column, parent))
    return false;

  auto result = QStandardItemModel::dropMimeData(data, action, row, 0, parent);

  Util::requestAllItems(*this);

  if (result)
    Q_EMIT orderChanged();

  return result;
}

void
Model::acknowledgeAllWarnings() {
  QMutexLocker locked{&m_mutex};

  for (auto const &job : m_jobsById)
    job->acknowledgeWarnings();
}

void
Model::acknowledgeAllErrors() {
  QMutexLocker locked{&m_mutex};

  for (auto const &job : m_jobsById)
    job->acknowledgeErrors();
}

void
Model::acknowledgeSelectedWarnings(QAbstractItemView *view) {
  withSelectedJobs(view, [](Job &job) { job.acknowledgeWarnings(); });
}

void
Model::acknowledgeSelectedErrors(QAbstractItemView *view) {
  withSelectedJobs(view, [](Job &job) { job.acknowledgeErrors(); });
}

QDateTime
Model::queueStartTime()
  const {
  return m_queueStartTime;
}

void
Model::runProgramOnQueueStop(QueueStatus status) {
  if (QueueStatus::Stopped != status)
    return;

  App::programRunner().run(Util::Settings::RunAfterJobQueueFinishes, [](ProgramRunner::VariableMap &) {
    // Nothing to do in this case.
  });
}

void
Model::sortAllJobs(int logicalColumnIdx,
                   Qt::SortOrder order) {
  QMutexLocker locked{&m_mutex};

  m_dontStartJobsNow = true;

  auto jobs          = m_jobsById.values();
  auto formatter     = logicalColumnIdx == 0 ? formatStatusForSorting
                     : logicalColumnIdx == 1 ? formatWarningsErrorForSorting
                     : logicalColumnIdx == 2 ? formatDescriptionForSorting
                     : logicalColumnIdx == 3 ? formatTypeForSorting
                     : logicalColumnIdx == 4 ? formatProgressForSorting
                     : logicalColumnIdx == 5 ? formatDateAddedForSorting
                     : logicalColumnIdx == 6 ? formatDateStartedForSorting
                     :                         formatDateFinishedForSorting;

  mtx::sort::by(jobs.begin(), jobs.end(), formatter);

  if (order == Qt::DescendingOrder)
    std::reverse(jobs.begin(), jobs.end());

  removeRows(0, rowCount());

  for (auto const &job : jobs)
    invisibleRootItem()->appendRow(createRow(*job));

  m_dontStartJobsNow = false;
}

void
Model::sortListOfJobs(QList<Job *> &jobs,
                      bool reverse) {
  auto rows = QHash<Job *, int>{};

  for (auto const &job : jobs)
    rows[job] = rowFromId(job->id());

  std::sort(jobs.begin(), jobs.end(), [&rows](Job *a, Job *b) { return rows[a] < rows[b]; });

  if (reverse)
    std::reverse(jobs.begin(), jobs.end());
}

void
Model::moveJobsUpOrDown(QList<Job *> jobs,
                        bool up) {
  auto couldNotBeMoved = QHash<uint64_t, bool>{};
  auto const direction = up ? -1 : +1;
  auto const numRows   = rowCount();

  sortListOfJobs(jobs, !up);

  for (auto const &job : jobs) {
    auto id  = job->id();
    auto row = rowFromId(id);

    if (row == RowNotFound) {
      qDebug() << "row not found for job ID" << id;
      continue;
    }

    auto targetRow = row + direction;
    Job *targetJob = (targetRow >= 0) && (targetRow < numRows) ? m_jobsById[idFromRow(targetRow)].get() : nullptr;

    if (!targetJob || couldNotBeMoved[targetJob->id()]) {
      couldNotBeMoved[id] = true;
      continue;
    }

    auto rootItem = invisibleRootItem();
    auto rowItems = rootItem->takeRow(row);
    rootItem->insertRow(targetRow, rowItems);
  }

  Q_EMIT orderChanged();
}

}
