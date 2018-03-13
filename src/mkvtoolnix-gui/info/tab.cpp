#include "common/common_pch.h"

#include <QDebug>
#include <QStandardItem>
#include <QThread>
#include <QVector>

#include <ebml/EbmlHead.h>
#include <matroska/KaxSegment.h>
#include <matroska/KaxInfo.h>

#include "common/ebml.h"
#include "common/list_utils.h"
#include "common/mm_file_io.h"
#include "common/mm_proxy_io.h"
#include "common/mm_read_buffer_io.h"
#include "common/mm_io_x.h"
#include "common/qt.h"
#include "mkvtoolnix-gui/forms/info/tab.h"
#include "mkvtoolnix-gui/info/element_reader.h"
#include "mkvtoolnix-gui/info/info_config.h"
#include "mkvtoolnix-gui/info/initial_scan.h"
#include "mkvtoolnix-gui/info/job_settings.h"
#include "mkvtoolnix-gui/info/job_settings_dialog.h"
#include "mkvtoolnix-gui/info/model.h"
#include "mkvtoolnix-gui/info/tab.h"
#include "mkvtoolnix-gui/jobs/info_job.h"
#include "mkvtoolnix-gui/jobs/tool.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/util/header_view_manager.h"
#include "mkvtoolnix-gui/util/kax_info.h"
#include "mkvtoolnix-gui/util/message_box.h"
#include "mkvtoolnix-gui/util/serial_worker_queue.h"
#include "mkvtoolnix-gui/util/tree.h"
#include "mkvtoolnix-gui/watch_jobs/tool.h"

namespace mtx { namespace gui { namespace Info {

using namespace mtx::gui;

class TabPrivate {
public:
  std::unique_ptr<Ui::Tab> m_ui{new Ui::Tab};

  mm_io_cptr m_file;
  QString m_fileName, m_savedFileName;
  QThread *m_queueThread{};
  Util::SerialWorkerQueue *m_queue{};

  Model *m_model{};
};

Tab::Tab(QWidget *parent)
  : QWidget{parent}
  , p_ptr{new TabPrivate}
{
  auto p = p_func();

  // Setup UI controls.
  p->m_ui->setupUi(this);

  p->m_model = new Model{this};
  p->m_ui->elements->setModel(p->m_model);

  Util::HeaderViewManager::create(*p->m_ui->elements, "Info::Elements");

  connect(p->m_ui->elements, &QTreeView::expanded,  this,       &Tab::readLevel1Element);
  connect(p->m_ui->elements, &QTreeView::collapsed, p->m_model, &Model::forgetLevel1ElementChildren);

  retranslateUi();

  auto pair        = Util::SerialWorkerQueue::create();
  p->m_queueThread = pair.first;
  p->m_queue       = pair.second;
  p->m_queueThread->start();
}

Tab::~Tab() {
  auto p = p_func();

  p->m_queue->abort();
  p->m_queueThread->quit();
  p->m_queueThread->wait();
}

QString
Tab::title()
  const {
  return QFileInfo{p_func()->m_fileName}.fileName();
}

void
Tab::load(QString const &fileName) {
  auto p = p_func();

  try {
    p->m_model->setInfo(std::make_unique<Util::KaxInfo>());
    p->m_model->reset();

    auto &info         = p->m_model->info();
    p->m_fileName      = fileName;
    p->m_savedFileName = QDir::toNativeSeparators( Q("%1.txt").arg(QFileInfo{ fileName }.absoluteFilePath().replace(QRegularExpression{Q("\\.[^.]*$")}, {})) );
    p->m_file          = std::static_pointer_cast<mm_io_c>(std::make_shared<mm_read_buffer_io_c>(std::make_shared<mm_file_io_c>(to_utf8(fileName), MODE_READ)));

    info.moveToThread(p->m_queueThread);
    info.set_source_file(p->m_file);
    info.set_use_gui(true);
    info.set_retain_elements(true);

    connect(&info, &Util::KaxInfo::startOfFileScanStarted,     MainWindow::get(), &MainWindow::startQueueSpinner);
    connect(&info, &Util::KaxInfo::startOfFileScanFinished,    MainWindow::get(), &MainWindow::stopQueueSpinner);
    connect(&info, &Util::KaxInfo::level1ElementsScanStarted,  MainWindow::get(), &MainWindow::startQueueSpinner);
    connect(&info, &Util::KaxInfo::level1ElementsScanFinished, MainWindow::get(), &MainWindow::stopQueueSpinner);
    connect(&info, &Util::KaxInfo::startOfFileScanFinished,    this,              &Tab::expandImportantElements);
    connect(&info, &Util::KaxInfo::elementInfoFound,           this,              &Tab::showElementInfo);
    connect(&info, &Util::KaxInfo::errorFound,                 this,              &Tab::showError);
    connect(&info, &Util::KaxInfo::elementFound,               p->m_model,        &Model::addElement);

    emit titleChanged();

    p->m_queue->add(new InitialScan{info, Util::KaxInfo::ScanType::StartOfFile});
    p->m_queue->add(new InitialScan{info, Util::KaxInfo::ScanType::Level1Elements});

  } catch (mtx::mm_io::exception &ex) {
    Util::MessageBox::critical(this)->title(QY("Reading failed")).text(QY("The file you tried to open (%1) could not be read successfully.").arg(fileName)).exec();
    emit removeThisTab();

  } catch (mtx::kax_info::exception &ex) {
    qDebug() << "Info::Tab::load: kax_info exception:" << Q(ex.what());

  }
}

void
Tab::save() {
  auto p = p_func();

  JobSettingsDialog dlg{this, p->m_savedFileName};
  if (dlg.exec() == QDialog::Rejected)
    return;

  auto settings                    = dlg.settings();
  auto newConfig                   = std::make_shared<InfoConfig>();

  newConfig->m_sourceFileName      = p->m_fileName;
  newConfig->m_destinationFileName = settings.m_fileName;
  newConfig->m_calcChecksums       = settings.m_checksums;
  newConfig->m_showSummary         = settings.m_mode == JobSettings::Mode::Summary;
  newConfig->m_showHexdump         = settings.m_hexDumps != JobSettings::HexDumps::None;
  newConfig->m_showSize            = true;
  newConfig->m_showTrackInfo       = settings.m_trackInfo;
  newConfig->m_hexPositions        = settings.m_hexPositions;
  newConfig->m_verbose             = settings.m_mode      == JobSettings::Mode::Summary                 ? 4
                                   : settings.m_verbosity == JobSettings::Verbosity::StopAtFirstCluster ? 1
                                   :                                                                      4;
  p->m_savedFileName               = settings.m_fileName;

  auto job                         = std::make_shared<Jobs::InfoJob>(Jobs::Job::PendingAuto, newConfig);

  job->setDateAdded(QDateTime::currentDateTime());
  job->setDescription(job->displayableDescription());

  if (Util::Settings::get().m_switchToJobOutputAfterStarting)
    MainWindow::get()->switchToTool(MainWindow::watchJobTool());

  MainWindow::jobTool()->addJob(std::static_pointer_cast<Jobs::Job>(job));
}

void
Tab::retranslateUi() {
  auto p = p_func();

  p->m_ui->retranslateUi(this);
  p->m_model->retranslateUi();

  emit titleChanged();
}

void
Tab::showElementInfo(int level,
                     QString const &text,
                     int64_t position,
                     int64_t size) {
  qDebug() << "showElementInfo" << level << position << size << text;
}

void
Tab::showError(const QString &message) {
  Util::MessageBox::critical(this)->title(QY("Error reading Matroska file")).text(message).exec();
  emit removeThisTab();
}

void
Tab::expandImportantElements() {
  setUpdatesEnabled(false);

  auto p        = p_func();

  auto view     = p->m_ui->elements;
  auto rootItem = p->m_model->invisibleRootItem();

  for (int l0Row = 0, numL0Rows = rootItem->rowCount(); l0Row < numL0Rows; ++l0Row) {
    auto l0Item      = rootItem->child(l0Row);
    auto l0ElementId = l0Item->data(EbmlIdRole).toUInt();

    if (l0ElementId == EBML_ID(EbmlHead).GetValue())
      Util::expandCollapseAll(view, true, p->m_model->indexFromItem(l0Item));

    else if (l0ElementId == EBML_ID(KaxSegment).GetValue()) {
      view->setExpanded(p->m_model->indexFromItem(l0Item), true);

      for (int l1Row = 0, numL1Rows = l0Item->rowCount(); l1Row < numL1Rows; ++l1Row) {
        auto l1Item      = l0Item->child(l1Row);
        auto l1ElementId = l1Item->data(EbmlIdRole).toUInt();

        if (mtx::included_in(l1ElementId, EBML_ID(KaxInfo).GetValue(), EBML_ID(KaxTracks).GetValue()))
          Util::expandCollapseAll(view, true, p->m_model->indexFromItem(l1Item));
      }
    }
  }

  setUpdatesEnabled(true);
}

void
Tab::readLevel1Element(QModelIndex const &idx) {
  if (!idx.isValid())
    return;

  auto p       = p_func();
  auto item    = p->m_model->itemFromIndex(idx);
  auto element = p->m_model->elementFromIndex(idx);

  if (!element || !item->data(DeferredLoadRole).toBool())
    return;

  auto reader = new ElementReader(*p->m_file, *element, idx);
  connect(reader, &ElementReader::elementRead, p->m_model, &Model::addChildrenOfLevel1Element);

  p->m_queue->add(reader);
}

}}}
