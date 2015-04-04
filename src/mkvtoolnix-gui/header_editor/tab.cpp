#include "common/common_pch.h"

#include <QFileInfo>
#include <QMessageBox>

#include <matroska/KaxInfoData.h>
#include <matroska/KaxSemantic.h>

#include "common/ebml.h"
#include "common/qt.h"
#include "common/segmentinfo.h"
#include "common/segment_tracks.h"
#include "mkvtoolnix-gui/forms/header_editor/tab.h"
#include "mkvtoolnix-gui/header_editor/bit_value_page.h"
#include "mkvtoolnix-gui/header_editor/page_model.h"
#include "mkvtoolnix-gui/header_editor/string_value_page.h"
#include "mkvtoolnix-gui/header_editor/tab.h"
#include "mkvtoolnix-gui/header_editor/top_level_page.h"
#include "mkvtoolnix-gui/header_editor/track_type_page.h"

namespace mtx { namespace gui { namespace HeaderEditor {

using namespace mtx::gui;

Tab::Tab(QWidget *parent,
         QString const &fileName)
  : QWidget{parent}
  , ui{new Ui::Tab}
  , m_fileName{fileName}
  , m_model{new PageModel{this}}
{
  // Setup UI controls.
  ui->setupUi(this);

  setupUi();
}

Tab::~Tab() {
}

void
Tab::resetData() {
  m_analyzer.reset();
  m_eSegmentInfo.reset();
  m_eTracks.reset();
  m_model->reset();
}

void
Tab::load() {
  resetData();

  if (!kax_analyzer_c::probe(to_utf8(m_fileName))) {
    QMessageBox::critical(this, QY("File parsing failed"), QY("The file you tried to open (%1) is not recognized as a valid Matroska/WebM file.").arg(m_fileName));
    emit removeThisTab();
    return;
  }

  m_analyzer = std::make_unique<QtKaxAnalyzer>(this, m_fileName);

  if (!m_analyzer->process(kax_analyzer_c::parse_mode_fast)) {
    QMessageBox::critical(this, QY("File parsing failed"), QY("The file you tried to open (%1) could not be read successfully.").arg(m_fileName));
    emit removeThisTab();
    return;
  }

  populateTree();

  m_analyzer->close_file();
}

void
Tab::setupUi() {
  auto info = QFileInfo{m_fileName};
  ui->fileName->setText(info.fileName());
  ui->directory->setText(info.path());

  ui->elements->setModel(m_model);

  m_pageContainerLayout = new QVBoxLayout{ui->pageContainer};
}

void
Tab::appendPage(PageBase *page,
                QModelIndex const &parentIdx) {
  m_pageContainerLayout->addWidget(page);
  m_model->appendPage(page, parentIdx);
}

PageModel *
Tab::getModel()
  const {
  return m_model;
}

void
Tab::retranslateUi() {
  ui->fileNameLabel->setText(QY("File name:"));
  ui->directoryLabel->setText(QY("Directory:"));

  auto &pages = m_model->getPages();
  for (auto const &page : pages)
    page->retranslateUi();
}

void
Tab::populateTree() {
  for (auto &data : m_analyzer->m_data)
    if (data->m_id == KaxInfo::ClassInfos.GlobalId) {
      handleSegmentInfo(*data);
      break;
    }

  for (auto &data : m_analyzer->m_data)
    if (data->m_id == KaxTracks::ClassInfos.GlobalId) {
      handleTracks(*data);
      break;
    }
}

void
Tab::itemSelected(QModelIndex idx) {
  auto selectedPage = m_model->selectedPage(idx);
  if (!selectedPage)
    return;

  auto pages = m_model->getPages();
  for (auto const &page : pages)
    page->setVisible(page == selectedPage);
}

QWidget *
Tab::getPageContainer()
  const {
  return ui->pageContainer;
}

QString const &
Tab::getFileName()
  const {
  return m_fileName;
}

bool
Tab::hasBeenModified() {
  auto pages = m_model->getTopLevelPages();
  for (auto const &page : pages)
    if (page->hasBeenModified())
      return true;

  return false;
}

void
Tab::doModifications() {
  auto pages = m_model->getTopLevelPages();
  for (auto const &page : pages)
    page->doModifications();

  if (m_eSegmentInfo) {
    fix_mandatory_segmentinfo_elements(m_eSegmentInfo.get());
    m_eSegmentInfo->UpdateSize(true, true);
  }

  if (m_eTracks) {
    fix_mandatory_segment_tracks_elements(m_eTracks.get());
    m_eTracks->UpdateSize(true, true);
  }
}

void
Tab::handleSegmentInfo(kax_analyzer_data_c &data) {
  m_eSegmentInfo = m_analyzer->read_element(&data);
  if (!m_eSegmentInfo)
    return;

  auto &info = static_cast<KaxInfo &>(*m_eSegmentInfo.get());
  auto page  = new TopLevelPage{*this, YT("Segment information"), m_eSegmentInfo};
  page->init();

  (new StringValuePage{*this, *page, info, KaxTitle::ClassInfos,           YT("Title"),                        YT("The title for the whole movie.")})->init();
  (new StringValuePage{*this, *page, info, KaxSegmentFilename::ClassInfos, YT("Segment file name"),            YT("The file name for this segment.")})->init();
  (new StringValuePage{*this, *page, info, KaxPrevFilename::ClassInfos,    YT("Previous file name"),           YT("An escaped file name corresponding to the previous segment.")})->init();
  (new StringValuePage{*this, *page, info, KaxNextFilename::ClassInfos,    YT("Next filename"),                YT("An escaped file name corresponding to the next segment.")})->init();
  (new BitValuePage{   *this, *page, info, KaxSegmentUID::ClassInfos,      YT("Segment unique ID"),            YT("A randomly generated unique ID to identify the current segment between many others (128 bits)."), 128})->init();
  (new BitValuePage{   *this, *page, info, KaxPrevUID::ClassInfos,         YT("Previous segment's unique ID"), YT("A unique ID to identify the previous chained segment (128 bits)."), 128})->init();
  (new BitValuePage{   *this, *page, info, KaxNextUID::ClassInfos,         YT("Next segment's unique ID"),     YT("A unique ID to identify the next chained segment (128 bits)."), 128})->init();
}

void
Tab::handleTracks(kax_analyzer_data_c &data) {
  m_eTracks = m_analyzer->read_element(&data);
  if (!m_eTracks)
    return;

  // auto kaxTracks = static_cast<KaxTracks *>(m_eTracks.get());

  // TODO: Tab::handleTracks

  auto trackIdxMkvmerge = 0u;

  for (auto const &element : static_cast<EbmlMaster &>(*m_eTracks)) {
    auto kTrackEntry = dynamic_cast<KaxTrackEntry *>(element);
    if (!kTrackEntry)
      continue;

    auto kTrackType = FindChild<KaxTrackType>(kTrackEntry);
    if (!kTrackType)
      continue;

    auto page = new TrackTypePage{*this, trackIdxMkvmerge++, m_eTracks, *kTrackEntry};
    page->init();

  }
}

void
Tab::validate() {
  auto pageIdx = m_model->validate();

  if (!pageIdx.isValid()) {
    QMessageBox::information(this, QY("Header validation"), QY("All header values are OK."));
    return;
  }

  ui->elements->selectionModel()->select(pageIdx, QItemSelectionModel::ClearAndSelect);
  itemSelected(pageIdx);

  QMessageBox::warning(this, QY("Header validation"), QY("There were errors in the header values preventing the headers from being saved. The first error has been selected."));
}

}}}
