#include "common/common_pch.h"

#include <QAbstractItemView>
#include <QMutexLocker>
#include <QSettings>
#include <QTimer>

#include "common/qt.h"
#include "mkvtoolnix-gui/jobs/model.h"
#include "mkvtoolnix-gui/jobs/mux_job.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/merge/mux_config.h"
#include "mkvtoolnix-gui/util/settings.h"
#include "mkvtoolnix-gui/util/util.h"
#include "mkvtoolnix-gui/watch_jobs/tab.h"

namespace mtx { namespace gui { namespace Jobs {

Model::Model(QObject *parent)
  : QStandardItemModel{parent}
  , m_mutex{QMutex::Recursive}
  , m_warningsIcon{Q(":/icons/16x16/dialog-warning.png")}
  , m_errorsIcon{Q(":/icons/16x16/dialog-error.png")}
  , m_started{}
  , m_dontStartJobsNow{}
{
  retranslateUi();
}

Model::~Model() {
}

void
Model::retranslateUi() {
  QMutexLocker locked{&m_mutex};

  auto labels = QStringList{} << QY("Status") << Q("") << QY("Description") << QY("Type") << QY("Progress") << QY("Date added") << QY("Date started") << QY("Date finished");
  setHorizontalHeaderLabels(labels);

  horizontalHeaderItem(StatusIconColumn)->setIcon(QIcon{Q(":/icons/16x16/dialog-warning-grayscale.png")});

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
  Util::withSelectedIndexes(view, [&](QModelIndex const &idx) {
    jobs << m_jobsById[ data(idx, Util::JobIdRole).value<uint64_t>() ].get();
  });

  return jobs;
}

uint64_t
Model::idFromRow(int row)
  const {
  return item(row)->data(Util::JobIdRole).value<uint64_t>();
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

void
Model::setRowText(QList<QStandardItem *> const &items,
                  Job const &job)
  const {
  items.at(StatusColumn)      ->setText(Job::displayableStatus(job.m_status));
  items.at(DescriptionColumn) ->setText(job.m_description);
  items.at(TypeColumn)        ->setText(job.displayableType());
  items.at(ProgressColumn)    ->setText(to_qs(boost::format("%1%%%") % job.m_progress));
  items.at(DateAddedColumn)   ->setText(Util::displayableDate(job.m_dateAdded));
  items.at(DateStartedColumn) ->setText(Util::displayableDate(job.m_dateStarted));
  items.at(DateFinishedColumn)->setText(Util::displayableDate(job.m_dateFinished));

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

  items[0]->setData(QVariant::fromValue(job.m_id), Util::JobIdRole);

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
Model::removeJobsIf(std::function<bool(Job const &)> predicate) {
  QMutexLocker locked{&m_mutex};

  auto toBeRemoved = QHash<Job const *, bool>{};

  for (auto row = rowCount(); 0 < row; --row) {
    auto job = m_jobsById[idFromRow(row - 1)].get();

    if (predicate(*job)) {
      m_jobsById.remove(job->m_id);
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

  saveJobs();
}

void
Model::add(JobPtr const &job) {
  QMutexLocker locked{&m_mutex};

  m_jobsById[job->m_id] = job;

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

  saveJobs();

  startNextAutoJob();
}

void
Model::onStatusChanged(uint64_t id) {
  QMutexLocker locked{&m_mutex};

  auto row = rowFromId(id);
  if (row == RowNotFound)
    return;

  auto const &job = *m_jobsById[id];
  auto status     = job.m_status;
  auto numBefore  = m_toBeProcessed.count();

  if (job.isToBeProcessed())
    m_toBeProcessed.insert(&job);

  if (m_toBeProcessed.count() != numBefore)
    updateProgress();

  item(row, StatusColumn)->setText(Job::displayableStatus(job.m_status));

  if (Job::Running == status)
    item(row, DateStartedColumn)->setText(Util::displayableDate(job.m_dateStarted));

  else if ((Job::DoneOk == status) || (Job::DoneWarnings == status) || (Job::Failed == status) || (Job::Aborted == status))
    item(row, DateFinishedColumn)->setText(Util::displayableDate(job.m_dateFinished));

  startNextAutoJob();

  processAutomaticJobRemoval(id, status);
}

void
Model::removeScheduledJobs() {
  QMutexLocker locked{&m_mutex};

  removeJobsIf([this](Job const &job) { return m_toBeRemoved[job.m_id]; });
  m_toBeRemoved.clear();
}

void
Model::scheduleJobForRemoval(uint64_t id) {
  QMutexLocker locked{&m_mutex};

  m_toBeRemoved[id] = true;
  QTimer::singleShot(0, this, SLOT(removeScheduledJobs()));
}

void
Model::processAutomaticJobRemoval(uint64_t id,
                                  Job::Status status) {
  auto const &cfg = Util::Settings::get();
  if (cfg.m_jobRemovalPolicy == Util::Settings::JobRemovalPolicy::Never)
    return;

  bool doneOk       = Job::DoneOk       == status;
  bool doneWarnings = Job::DoneWarnings == status;
  bool done         = doneOk || doneWarnings || (Job::Failed == status) || (Job::Aborted == status);

  if (   ((cfg.m_jobRemovalPolicy == Util::Settings::JobRemovalPolicy::IfSuccessful)    && doneOk)
      || ((cfg.m_jobRemovalPolicy == Util::Settings::JobRemovalPolicy::IfWarningsFound) && doneWarnings)
      || ((cfg.m_jobRemovalPolicy == Util::Settings::JobRemovalPolicy::Always)          && done))
    scheduleJobForRemoval(id);
}

void
Model::onProgressChanged(uint64_t id,
                         unsigned int progress) {
  QMutexLocker locked{&m_mutex};

  auto row = rowFromId(id);
  if (row < rowCount()) {
    item(row, ProgressColumn)->setText(to_qs(boost::format("%1%%%") % progress));
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
  auto numWarnings = 0;
  auto numErrors   = 0;

  for (auto const &job : m_jobsById) {
    numWarnings += job->numUnacknowledgedWarnings();
    numErrors   += job->numUnacknowledgedErrors();
  }

  emit numUnacknowledgedWarningsOrErrorsChanged(numWarnings, numErrors);
}

void
Model::startNextAutoJob() {
  if (m_dontStartJobsNow)
    return;

  QMutexLocker locked{&m_mutex};

  if (!m_started)
    return;

  Job *toStart = nullptr;
  for (auto row = 0, numRows = rowCount(); row < numRows; ++row) {
    auto job = m_jobsById[idFromRow(row)].get();

    if (Job::Running == job->m_status)
      return;
    if (!toStart && (Job::PendingAuto == job->m_status))
      toStart = job;
  }

  saveJobs();

  if (toStart) {
    toStart->start();
    updateJobStats();
    return;
  }

  // All jobs are done. Clear total progress.
  m_toBeProcessed.clear();
  updateProgress();
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
}

void
Model::updateProgress() {
  QMutexLocker locked{&m_mutex};

  if (!m_toBeProcessed.count())
    return;

  auto numRunning       = 0;
  auto numDone          = 0;
  auto runningProgress  = 0;

  for (auto const &job : m_toBeProcessed)
    if (Job::Running == job->m_status) {
      ++numRunning;
      runningProgress += job->m_progress;

    } else if (!job->isToBeProcessed() && m_toBeProcessed.contains(job))
      ++numDone;

  auto progress      = numRunning ? runningProgress / numRunning : 0u;
  auto totalProgress = (numDone * 100 + runningProgress) / m_toBeProcessed.count();

  emit progressChanged(progress, totalProgress);
}

void
Model::updateJobStats() {
  QMutexLocker locked{&m_mutex};

  auto numPendingAuto   = 0;
  auto numPendingManual = 0;
  auto numOther         = 0;

  for (auto const &job : m_jobsById)
    if (Job::PendingAuto == job->m_status)
      ++numPendingAuto;

    else if (Job::PendingManual == job->m_status)
      ++numPendingManual;

    else
      ++numOther;

  emit jobStatsChanged(numPendingAuto, numPendingManual, numOther);
}

void
Model::saveJobs()
  const {
  QSettings reg;
  saveJobs(reg);
}

void
Model::saveJobs(QSettings &settings)
  const {
  settings.beginGroup("jobQueue");
  settings.setValue("numberOfJobs", rowCount());

  for (auto row = 0, numRows = rowCount(); row < numRows; ++row) {
    settings.beginGroup(Q("job %1").arg(row));
    m_jobsById[idFromRow(row)]->saveJob(settings);
    settings.endGroup();
  }

  settings.endGroup();
}

void
Model::loadJobs(QSettings &settings) {
  QMutexLocker locked{&m_mutex};

  auto watchTab      = MainWindow::watchCurrentJobTab();
  m_dontStartJobsNow = true;

  m_jobsById.clear();
  m_toBeProcessed.clear();
  removeRows(0, rowCount());

  settings.remove("jobqQueue");
  settings.beginGroup("jobQueue");
  auto numberOfJobs = settings.value("numberOfJobs").toUInt();
  settings.endGroup();

  for (auto idx = 0u; idx < numberOfJobs; ++idx) {
    settings.beginGroup("jobQueue");
    settings.beginGroup(Q("job %1").arg(idx));

    try {
      auto job = Job::loadJob(settings);
      add(job);

      if (watchTab)
        watchTab->connectToJob(*job);

    } catch (Merge::InvalidSettingsX &) {
    }

    while (!settings.group().isEmpty())
      settings.endGroup();
  }

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

}}}
