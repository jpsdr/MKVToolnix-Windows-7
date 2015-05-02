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

  auto labels = QStringList{} << QY("Description") << QY("Type") << QY("Status") << QY("Progress") << QY("Date added") << QY("Date started") << QY("Date finished");
  setHorizontalHeaderLabels(labels);

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
Model::selectedJobs(QAbstractItemView *view)
  const {
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
  items.at(0)->setText(job.m_description);
  items.at(1)->setText(job.displayableType());
  items.at(2)->setText(Job::displayableStatus(job.m_status));
  items.at(3)->setText(to_qs(boost::format("%1%%%") % job.m_progress));
  items.at(4)->setText(Util::displayableDate(job.m_dateAdded));
  items.at(5)->setText(Util::displayableDate(job.m_dateStarted));
  items.at(6)->setText(Util::displayableDate(job.m_dateFinished));

  items[DescriptionColumn ]->setTextAlignment(Qt::AlignLeft  | Qt::AlignVCenter);
  items[ProgressColumn    ]->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
  items[DateAddedColumn   ]->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
  items[DateStartedColumn ]->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
  items[DateFinishedColumn]->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
}

QList<QStandardItem *>
Model::itemsForRow(QModelIndex const &idx) {
  auto rowItems = QList<QStandardItem *>{};

  for (auto column = 0; 7 > column; ++column)
    rowItems << itemFromIndex(idx.sibling(idx.row(), column));

  return rowItems;
}

QList<QStandardItem *>
Model::createRow(Job const &job)
  const {
  auto items = QList<QStandardItem *>{};
  for (auto idx = 0; idx < 7; ++idx)
    items << new QStandardItem{};
  setRowText(items, job);

  items[DescriptionColumn ]->setData(QVariant::fromValue(job.m_id), Util::JobIdRole);

  return items;
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
}

void
Model::add(JobPtr const &job) {
  QMutexLocker locked{&m_mutex};

  m_jobsById[job->m_id] = job;

  if (job->isToBeProcessed()) {
    m_toBeProcessed.insert(job.get());
    updateProgress();
  }

  invisibleRootItem()->appendRow(createRow(*job));

  connect(job.get(), &Job::progressChanged, this, &Model::onProgressChanged);
  connect(job.get(), &Job::statusChanged,   this, &Model::onStatusChanged);

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

  if (toStart) {
    toStart->start();
    return;
  }

  // All jobs are done. Clear total progress.
  m_toBeProcessed.clear();
  updateProgress();
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
  removeRows(0, rowCount());

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

}}}
