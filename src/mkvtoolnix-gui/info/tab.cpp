#include "common/common_pch.h"

#include <QAction>
#include <QDebug>
#include <QMenu>
#include <QStandardItem>
#include <QThread>
#include <QVector>

#include <ebml/EbmlHead.h>
#include <matroska/KaxSegment.h>

#include "common/ebml.h"
#include "common/kax_element_names.h"
#include "common/list_utils.h"
#include "common/mm_file_io.h"
#include "common/mm_proxy_io.h"
#include "common/mm_read_buffer_io.h"
#include "common/mm_io_x.h"
#include "common/qt.h"
#include "common/strings/formatting.h"
#include "mkvtoolnix-gui/forms/info/tab.h"
#include "mkvtoolnix-gui/info/element_reader.h"
#include "mkvtoolnix-gui/info/element_viewer_dialog.h"
#include "mkvtoolnix-gui/info/info_config.h"
#include "mkvtoolnix-gui/info/initial_scan.h"
#include "mkvtoolnix-gui/info/job_settings.h"
#include "mkvtoolnix-gui/info/job_settings_dialog.h"
#include "mkvtoolnix-gui/info/model.h"
#include "mkvtoolnix-gui/info/tab.h"
#include "mkvtoolnix-gui/jobs/info_job.h"
#include "mkvtoolnix-gui/jobs/tool.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/util/date_time.h"
#include "mkvtoolnix-gui/util/header_view_manager.h"
#include "mkvtoolnix-gui/util/kax_info.h"
#include "mkvtoolnix-gui/util/message_box.h"
#include "mkvtoolnix-gui/util/model.h"
#include "mkvtoolnix-gui/util/serial_worker_queue.h"
#include "mkvtoolnix-gui/util/tree.h"
#include "mkvtoolnix-gui/watch_jobs/tool.h"

using namespace mtx::gui;

namespace mtx::gui::Info {

class TabPrivate {
public:
  std::unique_ptr<Ui::Tab> m_ui{new Ui::Tab};

  mm_io_cptr m_file;
  QString m_fileName, m_savedFileName;
  QThread *m_queueThread{};
  Util::SerialWorkerQueue *m_queue{};

  Model *m_model{};

  QAction *m_showHexDumpAction{};
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

  p->m_showHexDumpAction = new QAction{this};

  Util::HeaderViewManager::create(*p->m_ui->elements, "Info::Elements");

  connect(p->m_ui->elements,      &QTreeView::customContextMenuRequested, this,       &Tab::showContextMenu);
  connect(p->m_ui->elements,      &QTreeView::expanded,                   this,       &Tab::readLevel1Element);
  connect(p->m_ui->elements,      &QTreeView::collapsed,                  p->m_model, &Model::forgetLevel1ElementChildren);
  connect(p->m_showHexDumpAction, &QAction::triggered,                    this,       &Tab::showElementHexDumpInViewer);

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
    QFileInfo fileInfo{fileName};

    p->m_model->setInfo(std::make_unique<Util::KaxInfo>());
    p->m_model->reset();

    p->m_ui->fileName->setText(fileInfo.fileName());
    p->m_ui->directory->setText(QDir::toNativeSeparators(fileInfo.absolutePath()));
    p->m_ui->size->setText(to_qs(mtx::string::format_file_size(fileInfo.size(), mtx::string::file_size_format_e::full)));
    p->m_ui->modified->setText(Util::displayableDate(fileInfo.lastModified()));

    auto &info         = p->m_model->info();
    p->m_fileName      = fileName;
    p->m_savedFileName = QDir::toNativeSeparators( Q("%1.txt").arg(fileInfo.absoluteFilePath().replace(QRegularExpression{Q("\\.[^.]*$")}, {})) );
    p->m_file          = std::static_pointer_cast<mm_io_c>(std::make_shared<mm_read_buffer_io_c>(std::make_shared<mm_file_io_c>(to_utf8(fileName), libebml::MODE_READ)));

    info.moveToThread(p->m_queueThread);
    info.set_source_file(p->m_file);
    info.set_use_gui(true);
    info.set_retain_elements(true);
    info.disableFrameInfo();

    connect(&info, &Util::KaxInfo::startOfFileScanStarted,     MainWindow::get(), &MainWindow::startQueueSpinner);
    connect(&info, &Util::KaxInfo::startOfFileScanFinished,    MainWindow::get(), &MainWindow::stopQueueSpinner);
    connect(&info, &Util::KaxInfo::level1ElementsScanStarted,  MainWindow::get(), &MainWindow::startQueueSpinner);
    connect(&info, &Util::KaxInfo::level1ElementsScanFinished, MainWindow::get(), &MainWindow::stopQueueSpinner);
    connect(&info, &Util::KaxInfo::startOfFileScanFinished,    this,              &Tab::expandImportantElements);
    connect(&info, &Util::KaxInfo::errorFound,                 this,              &Tab::showError);
    connect(&info, &Util::KaxInfo::elementFound,               p->m_model,        &Model::addElement);
    connect(&info, &Util::KaxInfo::elementInfoFound,           p->m_model,        &Model::addElementInfo);

    Q_EMIT titleChanged();

    p->m_queue->add(new InitialScan{info, Util::KaxInfo::ScanType::StartOfFile});
    p->m_queue->add(new InitialScan{info, Util::KaxInfo::ScanType::Level1Elements});

  } catch (mtx::mm_io::exception &ex) {
    Util::MessageBox::critical(this)->title(QY("Reading failed")).text(QY("The file you tried to open (%1) could not be read successfully.").arg(fileName)).exec();
    Q_EMIT removeThisTab();

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

  p->m_showHexDumpAction->setText(QY("Show &hex dump"));

  Q_EMIT titleChanged();
}

void
Tab::showError(const QString &message) {
  Util::MessageBox::critical(this)->title(QY("Error reading Matroska file")).text(message).exec();
  Q_EMIT removeThisTab();
}

void
Tab::expandImportantElements() {
  setUpdatesEnabled(false);

  auto p        = p_func();

  auto view     = p->m_ui->elements;
  auto rootItem = p->m_model->invisibleRootItem();

  for (int l0Row = 0, numL0Rows = rootItem->rowCount(); l0Row < numL0Rows; ++l0Row) {
    auto l0Item      = rootItem->child(l0Row);
    auto l0ElementId = l0Item->data(Roles::EbmlId).toUInt();

    if (l0ElementId == EBML_ID(libebml::EbmlHead).GetValue())
      Util::expandCollapseAll(view, true, p->m_model->indexFromItem(l0Item));

    else if (l0ElementId == EBML_ID(libmatroska::KaxSegment).GetValue()) {
      view->setExpanded(p->m_model->indexFromItem(l0Item), true);

      for (int l1Row = 0, numL1Rows = l0Item->rowCount(); l1Row < numL1Rows; ++l1Row) {
        auto l1Item      = l0Item->child(l1Row);
        auto l1ElementId = l1Item->data(Roles::EbmlId).toUInt();

        if (mtx::included_in(l1ElementId, EBML_ID(libmatroska::KaxInfo).GetValue(), EBML_ID(libmatroska::KaxTracks).GetValue()))
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

  if (!element || !item->data(Roles::DeferredLoad).toBool())
    return;

  item->setText(QY("Loading…"));

  auto reader = new ElementReader(*p->m_file, *element, idx);
  connect(reader, &ElementReader::elementRead, p->m_model, &Model::addChildrenOfLevel1Element);

  p->m_queue->add(reader);
}

void
Tab::showContextMenu(QPoint const &pos) {
  auto p       = p_func();
  auto idx     = Util::selectedRowIdx(p->m_ui->elements);
  auto element = p->m_model->elementFromIndex(idx);
  auto items   = p->m_model->itemsForRow(idx);

  if (   !element
      && (   !items[0]->data(Roles::Position).isValid()
          || !items[0]->data(Roles::Size).isValid()
          || !items[0]->data(Roles::PseudoType).isValid()))
    return;

  QMenu menu{this};

  menu.addAction(p->m_showHexDumpAction);

  menu.exec(p->m_ui->elements->viewport()->mapToGlobal(pos));
}

void
Tab::showElementHexDumpInViewer() {
  auto p              = p_func();
  auto idx            = Util::selectedRowIdx(p->m_ui->elements);
  auto element        = p->m_model->elementFromIndex(idx);
  auto items          = p->m_model->itemsForRow(idx);
  auto storedPosition = items[0]->data(Roles::Position);
  auto storedSize     = items[0]->data(Roles::Size);
  auto pseudoType     = items[0]->data(Roles::PseudoType);

  if (   !element
      && (   !storedPosition.isValid()
          || !storedSize.isValid()
          || !pseudoType.isValid()))
    return;

  memory_cptr mem;
  std::optional<uint64_t> signaledElementSize;
  uint64_t effectiveElementSize{}, effectiveElementPosition{};

  {
    QMutexLocker locker{&p->m_model->info().mutex()};

    if (element) {
      if (element->IsFiniteSize())
        signaledElementSize = element->HeadSize() + element->GetSize();

      effectiveElementPosition = element->GetElementPosition();
      effectiveElementSize     = signaledElementSize ? *signaledElementSize : p->m_file->get_size() - effectiveElementPosition;

    } else {
      effectiveElementPosition = storedPosition.toLongLong();
      effectiveElementSize     = storedSize.toLongLong();
      signaledElementSize      = effectiveElementSize;
    }

    try {
      p->m_file->setFilePointer(effectiveElementPosition);
      mem = p->m_file->read(std::min<unsigned int>(10 * 1024, effectiveElementSize));

    } catch (mtx::mm_io::exception &) {
    }
  }

  if (!mem)
    return;

  auto dlg    = new ElementViewerDialog{this};
  auto result = dlg
    ->setContent(mem, !!element)
    .setId(element ? get_ebml_id(*element).GetValue() : pseudoType.toUInt())
    .setPosition(effectiveElementPosition)
    .setSize(signaledElementSize, effectiveElementSize)
    .exec();

  if (result == ElementViewerDialog::DetachWindow)
    dlg->detachWindow();

  else
    delete dlg;
}

}
