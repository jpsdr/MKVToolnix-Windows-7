#include "common/common_pch.h"

#include <QDebug>
#include <QStandardItem>
#include <QThread>
#include <QVector>

#include <ebml/EbmlDummy.h>
#include <ebml/EbmlHead.h>
#include <matroska/KaxSegment.h>
#include <matroska/KaxInfo.h>

#include "common/ebml.h"
#include "common/kax_element_names.h"
#include "common/list_utils.h"
#include "common/mm_file_io.h"
#include "common/mm_io_x.h"
#include "common/qt.h"
#include "mkvtoolnix-gui/forms/info/tab.h"
#include "mkvtoolnix-gui/info/info_config.h"
#include "mkvtoolnix-gui/info/initial_scan.h"
#include "mkvtoolnix-gui/info/job_settings.h"
#include "mkvtoolnix-gui/info/job_settings_dialog.h"
#include "mkvtoolnix-gui/info/tab.h"
#include "mkvtoolnix-gui/jobs/info_job.h"
#include "mkvtoolnix-gui/jobs/tool.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/util/header_view_manager.h"
#include "mkvtoolnix-gui/util/kax_info.h"
#include "mkvtoolnix-gui/util/message_box.h"
#include "mkvtoolnix-gui/util/model.h"
#include "mkvtoolnix-gui/util/serial_worker_queue.h"
#include "mkvtoolnix-gui/util/tree.h"

namespace mtx { namespace gui { namespace Info {

using namespace mtx::gui;

namespace {
int ElementRole = Qt::UserRole + 1;
int EbmlIdRole  = Qt::UserRole + 2;
}

class TabPrivate {
public:
  std::unique_ptr<Ui::Tab> m_ui{new Ui::Tab};
  std::unique_ptr<Util::KaxInfo> m_info;

  mm_io_cptr m_file;

  QString m_fileName, m_savedFileName;
  QVector<QStandardItem *> m_treeInsertionPosition;
  QThread *m_queueThread{};
  Util::SerialWorkerQueue *m_queue{};
};

Tab::Tab(QWidget *parent)
  : QWidget{parent}
  , p_ptr{new TabPrivate}
{
  auto p = p_func();

  // Setup UI controls.
  p->m_ui->setupUi(this);

  auto model = new QStandardItemModel{};
  model->setColumnCount(4);
  p->m_ui->elements->setModel(model);

  Util::HeaderViewManager::create(*p->m_ui->elements, "Info::Elements");

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
    auto model    = static_cast<QStandardItemModel *>(p->m_ui->elements->model());
    p->m_fileName = fileName;
    p->m_file     = std::static_pointer_cast<mm_io_c>(std::make_shared<mm_file_io_c>(to_utf8(fileName), MODE_READ));
    p->m_info     = std::make_unique<Util::KaxInfo>();

    model->removeRows(0, model->rowCount());
    p->m_treeInsertionPosition.clear();
    p->m_treeInsertionPosition << model->invisibleRootItem();

    p->m_info->moveToThread(p->m_queueThread);
    p->m_info->set_source_file(p->m_file);
    p->m_info->set_use_gui(true);
    p->m_info->set_retain_elements(true);

    connect(p->m_info.get(), &Util::KaxInfo::started,            []() { MainWindow::get()->startStopQueueSpinner(true); });
    connect(p->m_info.get(), &Util::KaxInfo::finished,           []() { MainWindow::get()->startStopQueueSpinner(false); });
    connect(p->m_info.get(), &Util::KaxInfo::finished,           this, &Tab::expandImportantElements);
    connect(p->m_info.get(), &Util::KaxInfo::element_info_found, this, &Tab::showElementInfo);
    connect(p->m_info.get(), &Util::KaxInfo::element_found,      this, &Tab::showElement);
    connect(p->m_info.get(), &Util::KaxInfo::error_found,        this, &Tab::showError);

    emit titleChanged();

    p->m_queue->add(new InitialScan{*p->m_info});

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

  MainWindow::jobTool()->addJob(std::static_pointer_cast<Jobs::Job>(job));
}

void
Tab::retranslateUi() {
  auto p = p_func();

  p->m_ui->retranslateUi(this);

  auto &model = *static_cast<QStandardItemModel *>(p->m_ui->elements->model());

  Util::setDisplayableAndSymbolicColumnNames(model, {
    { QY("Elements"), Q("elements") },
    { QY("Content"),  Q("content")  },
    { QY("Position"), Q("position") },
    { QY("Size"),     Q("size")     },
  });

  model.horizontalHeaderItem(2)->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
  model.horizontalHeaderItem(3)->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

  emit titleChanged();

  if (!p->m_info)
    return;

  Util::walkTree(model, QModelIndex{}, [this, &model](QModelIndex const &idx) {
    QList<QStandardItem *> items;
    for (int columnIdx = 0, numColumns = model.columnCount(); columnIdx < numColumns; ++columnIdx)
      items << model.itemFromIndex(idx.sibling(idx.row(), columnIdx));

    auto element = reinterpret_cast<EbmlElement *>(items[0]->data(ElementRole).toULongLong());
    if (!element)
      return;

    setItemsFromElement(items, *element);
  });

}

void
Tab::showElementInfo(int level,
                     QString const &text,
                     int64_t position,
                     int64_t size) {
  qDebug() << "showElementInfo" << level << position << size << text;
}

void
Tab::showElement(int level,
                 EbmlElement *element) {
  auto p = p_func();

  if (!element)
    return;

  QList<QStandardItem *> items{ new QStandardItem{}, new QStandardItem{}, new QStandardItem{}, new QStandardItem{} };
  setItemsFromElement(items, *element);

  while (p->m_treeInsertionPosition.size() > (level + 1))
    p->m_treeInsertionPosition.removeLast();

  p->m_treeInsertionPosition.last()->appendRow(items);

  p->m_treeInsertionPosition << items[0];
}

void
Tab::showError(const QString &message) {
  Util::MessageBox::critical(this)->title(QY("Error reading Matroska file")).text(message).exec();
  emit removeThisTab();
}

void
Tab::setItemsFromElement(QList<QStandardItem *> &items,
                         EbmlElement &element) {
  auto p    = p_func();
  auto name = kax_element_names_c::get(element);
  std::string content;

  if (name.empty())
    name = (boost::format(Y("Unknown (ID: 0x%1%)")) % p->m_info->format_ebml_id_as_hex(element)).str();

  else if (dynamic_cast<EbmlDummy *>(&element))
    name = (boost::format(Y("Known element, but invalid at this position: %1% (ID: 0x%2%)")) % name % p->m_info->format_ebml_id_as_hex(element)).str();

  else
    content = p->m_info->format_element_value(element);

  items[0]->setText(Q(name));
  items[1]->setText(Q(content));
  items[2]->setText(Q("%1").arg(element.GetElementPosition()));
  items[3]->setText(Q("%1").arg(element.HeadSize() + element.GetSize()));

  items[2]->setTextAlignment(Qt::AlignRight);
  items[3]->setTextAlignment(Qt::AlignRight);

  items[0]->setData(reinterpret_cast<qulonglong>(&element),          ElementRole);
  items[0]->setData(static_cast<qint64>(EbmlId(element).GetValue()), EbmlIdRole);
}

void
Tab::expandImportantElements() {
  setUpdatesEnabled(false);

 auto p        = p_func();

 auto view     = p->m_ui->elements;
 auto model    = static_cast<QStandardItemModel *>(view->model());
 auto rootItem = model->invisibleRootItem();

  for (int l0Row = 0, numL0Rows = rootItem->rowCount(); l0Row < numL0Rows; ++l0Row) {
    auto l0Item      = rootItem->child(l0Row);
    auto l0ElementId = l0Item->data(EbmlIdRole).toUInt();

    if (l0ElementId == EBML_ID(EbmlHead).GetValue())
      Util::expandCollapseAll(view, true, model->indexFromItem(l0Item));

    else if (l0ElementId == EBML_ID(KaxSegment).GetValue()) {
      view->setExpanded(model->indexFromItem(l0Item), true);

      for (int l1Row = 0, numL1Rows = l0Item->rowCount(); l1Row < numL1Rows; ++l1Row) {
        auto l1Item      = l0Item->child(l1Row);
        auto l1ElementId = l1Item->data(EbmlIdRole).toUInt();

        if (mtx::included_in(l1ElementId, EBML_ID(KaxInfo).GetValue(), EBML_ID(KaxTracks).GetValue()))
          Util::expandCollapseAll(view, true, model->indexFromItem(l1Item));
      }
    }
  }

  setUpdatesEnabled(true);
}

}}}
