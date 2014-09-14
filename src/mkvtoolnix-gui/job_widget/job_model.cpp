#include "common/common_pch.h"

#include <QAbstractItemView>
#include <QMutexLocker>
#include <QSettings>

#include "common/qt.h"
#include "mkvtoolnix-gui/job_widget/job_model.h"
#include "mkvtoolnix-gui/job_widget/mux_job.h"
#include "mkvtoolnix-gui/util/util.h"

JobModel::JobModel(QObject *parent)
  : QStandardItemModel{parent}
  , m_mutex{QMutex::Recursive}
  , m_started{}
  , m_dontStartJobsNow{}
{
  auto labels = QStringList{};
  labels << QY("Description") << QY("Type") << QY("Status") << QY("Progress") << QY("Date added") << QY("Date started") << QY("Date finished");
  setHorizontalHeaderLabels(labels);

  horizontalHeaderItem(DescriptionColumn) ->setTextAlignment(Qt::AlignLeft);
  horizontalHeaderItem(ProgressColumn)    ->setTextAlignment(Qt::AlignRight);
  horizontalHeaderItem(DateAddedColumn)   ->setTextAlignment(Qt::AlignRight);
  horizontalHeaderItem(DateStartedColumn) ->setTextAlignment(Qt::AlignRight);
  horizontalHeaderItem(DateFinishedColumn)->setTextAlignment(Qt::AlignRight);
}

JobModel::~JobModel() {
}

QList<Job *>
JobModel::selectedJobs(QAbstractItemView *view)
  const {
  auto selectedIds = QMap<uint64_t, bool>{};
  Util::withSelectedIndexes(view, [&](QModelIndex const &idx) {
      selectedIds[ data(idx, Util::JobIdRole).value<uint64_t>() ] = true;
    });

  QList<Job *> jobs;
  for (auto const &job : m_jobs)
    if (selectedIds[job->m_id])
      jobs << job.get();

  return jobs;
}

int
JobModel::rowFromId(uint64_t id)
  const {
  auto jobItr = brng::find_if(m_jobs, [id](JobPtr const &job) { return job->m_id == id; });
  return jobItr != m_jobs.end() ? std::distance(m_jobs.begin(), jobItr) : RowNotFound;
}

Job *
JobModel::fromId(uint64_t id)
  const {
  auto jobItr = brng::find_if(m_jobs, [id](JobPtr const &job) { return job->m_id == id; });
  return jobItr != m_jobs.end() ? jobItr->get() : nullptr;
}

Job *
JobModel::fromIndex(QModelIndex const &index)
  const {
  if (!index.isValid() || (index.row() >= m_jobs.size()))
    return nullptr;

  auto const &job = m_jobs[index.row()];
  return job->m_id == item(index.row())->data(Util::JobIdRole).value<uint64_t>() ? job.get() : nullptr;
}

bool
JobModel::hasJobs()
  const {
  return !m_jobs.isEmpty();
}

QList<QStandardItem *>
JobModel::createRow(Job const &job)
  const {
  auto items    = QList<QStandardItem *>{};
  auto progress = to_qs(boost::format("%1%%%") % job.m_progress);

  items << (new QStandardItem{job.m_description})                      << (new QStandardItem{job.displayableType()})                    << (new QStandardItem{Job::displayableStatus(job.m_status)}) << (new QStandardItem{progress})
        << (new QStandardItem{Util::displayableDate(job.m_dateAdded)}) << (new QStandardItem{Util::displayableDate(job.m_dateStarted)}) << (new QStandardItem{Util::displayableDate(job.m_dateFinished)});

  items[DescriptionColumn ]->setData(QVariant::fromValue(job.m_id), Util::JobIdRole);
  items[DescriptionColumn ]->setTextAlignment(Qt::AlignLeft);
  items[ProgressColumn    ]->setTextAlignment(Qt::AlignRight);
  items[DateAddedColumn   ]->setTextAlignment(Qt::AlignRight);
  items[DateStartedColumn ]->setTextAlignment(Qt::AlignRight);
  items[DateFinishedColumn]->setTextAlignment(Qt::AlignRight);

  return items;
}

void
JobModel::removeJobsIf(std::function<bool(Job const &)> predicate) {
  QMutexLocker locked{&m_mutex};

  auto toBeRemoved = QHash<Job const *, bool>{};

  for (int idx = m_jobs.size(); 0 < idx; --idx) {
    auto job = m_jobs[idx - 1].get();

    if (predicate(*job)) {
      toBeRemoved[job] = true;
      removeRow(idx - 1);
    }
  }

  brng::remove_erase_if(m_jobs, [&toBeRemoved](JobPtr const &job) { return toBeRemoved[job.get()]; });
  auto const keys = toBeRemoved.keys();
  for (auto const &job : keys)
    m_toBeProcessed.remove(job);

  updateProgress();
}

void
JobModel::add(JobPtr const &job) {
  QMutexLocker locked{&m_mutex};

  m_jobs << job;

  if (job->isToBeProcessed()) {
    m_toBeProcessed.insert(job.get());
    updateProgress();
  }

  invisibleRootItem()->appendRow(createRow(*job));

  connect(job.get(), SIGNAL(progressChanged(uint64_t,unsigned int)), this, SLOT(onProgressChanged(uint64_t,unsigned int)));
  connect(job.get(), SIGNAL(statusChanged(uint64_t,Job::Status)),    this, SLOT(onStatusChanged(uint64_t,Job::Status)));

  startNextAutoJob();
}

void
JobModel::onStatusChanged(uint64_t id,
                          Job::Status status) {
  QMutexLocker locked{&m_mutex};

  auto row = rowFromId(id);
  if (row == RowNotFound)
    return;

  auto const &job = *m_jobs[row];
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
}

void
JobModel::onProgressChanged(uint64_t id,
                            unsigned int progress) {
  QMutexLocker locked{&m_mutex};

  auto row = rowFromId(id);
  if (row < m_jobs.size()) {
    item(row, ProgressColumn)->setText(to_qs(boost::format("%1%%%") % progress));
    updateProgress();
  }
}

void
JobModel::startNextAutoJob() {
  if (m_dontStartJobsNow)
    return;

  QMutexLocker locked{&m_mutex};

  if (!m_started)
    return;

  for (auto const &job : m_jobs)
    if (job->m_status == Job::Running)
      return;

  for (auto const &job : m_jobs)
    if (job->m_status == Job::PendingAuto) {
      job->start();
      return;
    }

  // All jobs are done. Clear total progress.
  m_toBeProcessed.clear();
  updateProgress();
}

void
JobModel::start() {
  m_started = true;
  startNextAutoJob();
}

void
JobModel::stop() {
  m_started = false;
}

void
JobModel::updateProgress() {
  QMutexLocker locked{&m_mutex};

  if (!m_toBeProcessed.count()) {
    emit progressChanged(0, 0);
    return;
  }

  auto numRunning      = 0u;
  auto numDone         = 0u;
  auto runningProgress = 0u;

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
JobModel::saveJobs(QSettings &settings)
  const {
  settings.beginGroup("jobQueue");
  settings.setValue("numberOfJobs", m_jobs.size());

  auto idx = 0u;
  for (auto const &job : m_jobs) {
    settings.beginGroup(Q("job %1").arg(idx++));
    job->saveJob(settings);
    settings.endGroup();
  }

  settings.endGroup();
}

void
JobModel::loadJobs(QSettings &settings) {
  QMutexLocker locked{&m_mutex};

  m_dontStartJobsNow = true;

  removeRows(0, rowCount());
  m_jobs.clear();

  settings.beginGroup("jobQueue");
  auto numberOfJobs = settings.value("numberOfJobs").toUInt();
  for (auto idx = 0u; idx < numberOfJobs; ++idx) {
    settings.beginGroup(Q("job %1").arg(idx++));
    add(Job::loadJob(settings));
    settings.endGroup();
  }

  settings.endGroup();

  updateProgress();

  m_dontStartJobsNow = false;
}
