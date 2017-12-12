#include "common/common_pch.h"

#include <QDir>
#include <QFileInfo>
#include <QMenu>
#include <QMessageBox>

#include <matroska/KaxAttached.h>
#include <matroska/KaxAttachments.h>
#include <matroska/KaxInfoData.h>
#include <matroska/KaxSemantic.h>

#include "common/construct.h"
#include "common/ebml.h"
#include "common/extern_data.h"
#include "common/list_utils.h"
#include "common/mm_io_x.h"
#include "common/property_element.h"
#include "common/qt.h"
#include "common/segmentinfo.h"
#include "common/strings/formatting.h"
#include "common/unique_numbers.h"
#include "mkvtoolnix-gui/forms/header_editor/tab.h"
#include "mkvtoolnix-gui/header_editor/action_for_dropped_files_dialog.h"
#include "mkvtoolnix-gui/header_editor/ascii_string_value_page.h"
#include "mkvtoolnix-gui/header_editor/attached_file_page.h"
#include "mkvtoolnix-gui/header_editor/attachments_page.h"
#include "mkvtoolnix-gui/header_editor/bit_value_page.h"
#include "mkvtoolnix-gui/header_editor/bool_value_page.h"
#include "mkvtoolnix-gui/header_editor/float_value_page.h"
#include "mkvtoolnix-gui/header_editor/language_value_page.h"
#include "mkvtoolnix-gui/header_editor/page_model.h"
#include "mkvtoolnix-gui/header_editor/string_value_page.h"
#include "mkvtoolnix-gui/header_editor/tab.h"
#include "mkvtoolnix-gui/header_editor/time_value_page.h"
#include "mkvtoolnix-gui/header_editor/tool.h"
#include "mkvtoolnix-gui/header_editor/top_level_page.h"
#include "mkvtoolnix-gui/header_editor/track_type_page.h"
#include "mkvtoolnix-gui/header_editor/unsigned_integer_value_page.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/util/basic_tree_view.h"
#include "mkvtoolnix-gui/util/file_dialog.h"
#include "mkvtoolnix-gui/util/header_view_manager.h"
#include "mkvtoolnix-gui/util/model.h"
#include "mkvtoolnix-gui/util/message_box.h"
#include "mkvtoolnix-gui/util/settings.h"
#include "mkvtoolnix-gui/util/widget.h"

namespace mtx { namespace gui { namespace HeaderEditor {

using namespace mtx::gui;

Tab::Tab(QWidget *parent,
         QString const &fileName)
  : QWidget{parent}
  , ui{new Ui::Tab}
  , m_fileName{fileName}
  , m_model{new PageModel{this}}
  , m_treeContextMenu{new QMenu{this}}
  , m_expandAllAction{new QAction{this}}
  , m_collapseAllAction{new QAction{this}}
  , m_addAttachmentsAction{new QAction{this}}
  , m_removeAttachmentAction{new QAction{this}}
  , m_saveAttachmentContentAction{new QAction{this}}
  , m_replaceAttachmentContentAction{new QAction{this}}
  , m_replaceAttachmentContentSetValuesAction{new QAction{this}}
{
  // Setup UI controls.
  ui->setupUi(this);

  setupUi();

  retranslateUi();
}

Tab::~Tab() {
}

void
Tab::resetData() {
  m_analyzer.reset();
  m_eSegmentInfo.reset();
  m_eTracks.reset();
  m_model->reset();
  m_segmentinfoPage = nullptr;
}

void
Tab::load() {
  QVector<int> selectedRows;

  auto selectedIdx = ui->elements->selectionModel()->currentIndex();
  if (!selectedIdx.isValid()) {
    auto rowIndexes = ui->elements->selectionModel()->selectedRows();
    if (!rowIndexes.isEmpty())
      selectedIdx = rowIndexes.first();
  }

  while (selectedIdx.isValid()) {
    selectedRows.insert(0, selectedIdx.row());
    selectedIdx = selectedIdx.sibling(selectedIdx.row(), 0).parent();
  }

  QHash<QString, bool> expansionStatus;

  for (auto const &page : m_model->allExpandablePages()) {
    auto key = dynamic_cast<TopLevelPage &>(*page).internalIdentifier();
    expansionStatus[key] = ui->elements->isExpanded(page->m_pageIdx);
  }

  resetData();

  if (!kax_analyzer_c::probe(to_utf8(m_fileName))) {
    auto text = Q("%1 %2")
      .arg(QY("The file you tried to open (%1) is not recognized as a valid Matroska/WebM file.").arg(m_fileName))
      .arg(QY("Possible reasons are: the file is not a Matroska file; the file is write-protected; the file is locked by another process; you do not have permission to access the file."));
    Util::MessageBox::critical(this)->title(QY("File parsing failed")).text(text).exec();
    emit removeThisTab();
    return;
  }

  m_analyzer = std::make_unique<QtKaxAnalyzer>(this, m_fileName);
  bool ok    = false;
  QString error;

  try {
    ok = m_analyzer->set_parse_mode(kax_analyzer_c::parse_mode_fast)
      .set_open_mode(MODE_READ)
      .set_throw_on_error(true)
      .process();

  } catch (mtx::kax_analyzer_x &ex) {
    error = QY("Error details: %1.").arg(Q(ex.what()));
  }

  if (!ok) {
    if (error.isEmpty())
      error = QY("Possible reasons are: the file is not a Matroska file; the file is write-protected; the file is locked by another process; you do not have permission to access the file.");

    auto text = Q("%1 %2")
      .arg(QY("The file you tried to open (%1) could not be read successfully.").arg(m_fileName))
      .arg(error);
    Util::MessageBox::critical(this)->title(QY("File parsing failed")).text(text).exec();
    emit removeThisTab();
    return;
  }

  populateTree();

  m_analyzer->close_file();

  for (auto const &page : m_model->allExpandablePages()) {
    auto key = dynamic_cast<TopLevelPage &>(*page).internalIdentifier();
    ui->elements->setExpanded(page->m_pageIdx, expansionStatus[key]);
  }

  Util::resizeViewColumnsToContents(ui->elements);

  if (selectedRows.isEmpty())
    return;

  selectedIdx = m_model->index(selectedRows.takeFirst(), 0);
  for (auto row : selectedRows)
    selectedIdx = m_model->index(row, 0, selectedIdx);

  auto selection = QItemSelection{selectedIdx, selectedIdx.sibling(selectedIdx.row(), m_model->columnCount() - 1)};
  ui->elements->selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Current);
  selectionChanged(selectedIdx, QModelIndex{});
}

void
Tab::save() {
  auto segmentinfoModified = false;
  auto tracksModified      = false;
  auto attachmentsModified = false;

  for (auto const &page : m_model->topLevelPages()) {
    if (!page->hasBeenModified())
      continue;

    if (page == m_segmentinfoPage)
      segmentinfoModified = true;

    else if (page == m_attachmentsPage)
      attachmentsModified = true;

    else
      tracksModified      = true;
  }

  if (!segmentinfoModified && !tracksModified && !attachmentsModified) {
    Util::MessageBox::information(this)->title(QY("File has not been modified")).text(QY("The header values have not been modified. There is nothing to save.")).exec();
    return;
  }

  auto pageIdx = m_model->validate();
  if (pageIdx.isValid()) {
    reportValidationFailure(false, pageIdx);
    return;
  }

  doModifications();

  bool ok = true;

  try {
    if (segmentinfoModified && m_eSegmentInfo) {
      auto result = m_analyzer->update_element(m_eSegmentInfo, true);
      if (kax_analyzer_c::uer_success != result) {
        QtKaxAnalyzer::displayUpdateElementResult(this, result, QY("Saving the modified segment information header failed."));
        ok = false;
      }
    }

    if (tracksModified && m_eTracks) {
      auto result = m_analyzer->update_element(m_eTracks, true);
      if (kax_analyzer_c::uer_success != result) {
        QtKaxAnalyzer::displayUpdateElementResult(this, result, QY("Saving the modified track headers failed."));
        ok = false;
      }
    }

    if (attachmentsModified) {
      auto attachments = std::make_shared<KaxAttachments>();

      for (auto const &attachedFilePage : m_attachmentsPage->m_children)
        attachments->PushElement(*dynamic_cast<AttachedFilePage &>(*attachedFilePage).m_attachment.get());

      auto result = attachments->ListSize() ? m_analyzer->update_element(attachments.get(), true)
                  :                           m_analyzer->remove_elements(KaxAttachments::ClassInfos.GlobalId);

      attachments->RemoveAll();

      if (kax_analyzer_c::uer_success != result) {
        QtKaxAnalyzer::displayUpdateElementResult(this, result, QY("Saving the modified attachments failed."));
        ok = false;
      }
    }

  } catch (mtx::kax_analyzer_x &ex) {
    QMessageBox::critical(this, QY("Error writing Matroska file"), QY("Error details: %1.").arg(Q(ex.what())));
    ok = false;
  }

  m_analyzer->close_file();

  load();

  if (ok)
    MainWindow::get()->setStatusBarMessage(QY("The file has been saved successfully."));
}

void
Tab::setupUi() {
  Util::Settings::get().handleSplitterSizes(ui->headerEditorSplitter);

  auto info = QFileInfo{m_fileName};
  ui->fileName->setText(info.fileName());
  ui->directory->setText(QDir::toNativeSeparators(info.path()));

  ui->elements->setModel(m_model);
  ui->elements->acceptDroppedFiles(true);

  Util::HeaderViewManager::create(*ui->elements, "HeaderEditor::Elements");
  Util::preventScrollingWithoutFocus(this);

  connect(ui->elements,                              &Util::BasicTreeView::customContextMenuRequested, this, &Tab::showTreeContextMenu);
  connect(ui->elements,                              &Util::BasicTreeView::filesDropped,               this, &Tab::handleDroppedFiles);
  connect(ui->elements,                              &Util::BasicTreeView::deletePressed,              this, &Tab::removeSelectedAttachment);
  connect(ui->elements,                              &Util::BasicTreeView::insertPressed,              this, &Tab::selectAttachmentsAndAdd);
  connect(ui->elements->selectionModel(),            &QItemSelectionModel::currentChanged,             this, &Tab::selectionChanged);
  connect(m_expandAllAction,                         &QAction::triggered,                              this, &Tab::expandAll);
  connect(m_collapseAllAction,                       &QAction::triggered,                              this, &Tab::collapseAll);
  connect(m_addAttachmentsAction,                    &QAction::triggered,                              this, &Tab::selectAttachmentsAndAdd);
  connect(m_removeAttachmentAction,                  &QAction::triggered,                              this, &Tab::removeSelectedAttachment);
  connect(m_saveAttachmentContentAction,             &QAction::triggered,                              this, &Tab::saveAttachmentContent);
  connect(m_replaceAttachmentContentAction,          &QAction::triggered,                              [this]() { replaceAttachmentContent(false); });
  connect(m_replaceAttachmentContentSetValuesAction, &QAction::triggered,                              [this]() { replaceAttachmentContent(true); });
}

void
Tab::appendPage(PageBase *page,
                QModelIndex const &parentIdx) {
  ui->pageContainer->addWidget(page);
  m_model->appendPage(page, parentIdx);
}

PageModel *
Tab::model()
  const {
  return m_model;
}

PageBase *
Tab::currentlySelectedPage()
  const {
  return m_model->selectedPage(ui->elements->selectionModel()->currentIndex());
}

void
Tab::retranslateUi() {
  ui->fileNameLabel->setText(QY("File name:"));
  ui->directoryLabel->setText(QY("Directory:"));

  m_expandAllAction->setText(QY("&Expand all"));
  m_collapseAllAction->setText(QY("&Collapse all"));
  m_addAttachmentsAction->setText(QY("&Add attachments"));
  m_removeAttachmentAction->setText(QY("&Remove attachment"));
  m_saveAttachmentContentAction->setText(QY("&Save attachment content to a file"));
  m_replaceAttachmentContentAction->setText(QY("Re&place attachment with a new file"));
  m_replaceAttachmentContentSetValuesAction->setText(QY("Replace attachment with a new file and &derive name && MIME type from it"));

  m_addAttachmentsAction->setIcon(QIcon{Q(":/icons/16x16/list-add.png")});
  m_removeAttachmentAction->setIcon(QIcon{Q(":/icons/16x16/list-remove.png")});
  m_saveAttachmentContentAction->setIcon(QIcon{Q(":/icons/16x16/document-save.png")});
  m_replaceAttachmentContentAction->setIcon(QIcon{Q(":/icons/16x16/document-open.png")});

  setupToolTips();

  for (auto const &page : m_model->pages())
    page->retranslateUi();

  m_model->retranslateUi();

  Util::resizeViewColumnsToContents(ui->elements);
}

void
Tab::setupToolTips() {
  Util::setToolTip(ui->elements, QY("Right-click for actions for header elements and attachments"));
}

void
Tab::populateTree() {
  m_analyzer->with_elements(KaxInfo::ClassInfos.GlobalId, [this](kax_analyzer_data_c const &data) {
    handleSegmentInfo(data);
  });

  m_analyzer->with_elements(KaxTracks::ClassInfos.GlobalId, [this](kax_analyzer_data_c const &data) {
    handleTracks(data);
  });

  handleAttachments();
}

void
Tab::selectionChanged(QModelIndex const &current,
                      QModelIndex const &) {
  if (m_ignoreSelectionChanges)
    return;

  auto selectedPage = m_model->selectedPage(current);
  if (selectedPage)
    ui->pageContainer->setCurrentWidget(selectedPage);
}

QString const &
Tab::fileName()
  const {
  return m_fileName;
}

QString
Tab::title()
  const {
  return QFileInfo{m_fileName}.fileName();
}

PageBase *
Tab::hasBeenModified() {
  auto &pages = m_model->topLevelPages();
  for (auto const &page : pages) {
    auto modifiedPage = page->hasBeenModified();
    if (modifiedPage)
      return modifiedPage;
  }

  return nullptr;
}

void
Tab::pruneEmptyMastersForTrack(TrackTypePage &page) {
  auto trackType = FindChildValue<KaxTrackType>(page.m_master);

  if (!mtx::included_in(trackType, track_video, track_audio))
    return;

  std::unordered_map<EbmlMaster *, bool> handled;

  if (trackType == track_video) {
    auto trackVideo            = &GetChildEmptyIfNew<KaxTrackVideo>(page.m_master);
    auto videoColour           = &GetChildEmptyIfNew<KaxVideoColour>(trackVideo);
    auto videoColourMasterMeta = &GetChildEmptyIfNew<KaxVideoColourMasterMeta>(videoColour);
    auto videoProjection       = &GetChildEmptyIfNew<KaxVideoProjection>(trackVideo);

    remove_master_from_parent_if_empty_or_only_defaults(videoColour,    videoColourMasterMeta, handled);
    remove_master_from_parent_if_empty_or_only_defaults(trackVideo,     videoColour,           handled);
    remove_master_from_parent_if_empty_or_only_defaults(trackVideo,     videoProjection,       handled);
    remove_master_from_parent_if_empty_or_only_defaults(&page.m_master, trackVideo,            handled);

  } else
    // trackType is track_audio
    remove_master_from_parent_if_empty_or_only_defaults(&page.m_master, &GetChildEmptyIfNew<KaxTrackAudio>(page.m_master), handled);
}

void
Tab::pruneEmptyMastersForAllTracks() {
  for (auto const &page : m_model->topLevelPages())
    if (dynamic_cast<TrackTypePage *>(page))
      pruneEmptyMastersForTrack(static_cast<TrackTypePage &>(*page));
}

void
Tab::doModifications() {
  auto &pages = m_model->topLevelPages();
  for (auto const &page : pages)
    page->doModifications();

  pruneEmptyMastersForAllTracks();

  if (m_eSegmentInfo) {
    fix_mandatory_elements(m_eSegmentInfo.get());
    m_eSegmentInfo->UpdateSize(true, true);
  }

  if (m_eTracks) {
    fix_mandatory_elements(m_eTracks.get());
    m_eTracks->UpdateSize(true, true);
  }
}

ValuePage *
Tab::createValuePage(TopLevelPage &parentPage,
                     EbmlMaster &parentMaster,
                     property_element_c const &element) {
  ValuePage *page{};
  auto const type = element.m_type;

  page = element.m_callbacks == &KaxTrackLanguage::ClassInfos     ? new LanguageValuePage{       *this, parentPage, parentMaster, *element.m_callbacks, element.m_title, element.m_description}
       : type                == property_element_c::EBMLT_BOOL    ? new BoolValuePage{           *this, parentPage, parentMaster, *element.m_callbacks, element.m_title, element.m_description}
       : type                == property_element_c::EBMLT_BINARY  ? new BitValuePage{            *this, parentPage, parentMaster, *element.m_callbacks, element.m_title, element.m_description, element.m_bit_length}
       : type                == property_element_c::EBMLT_FLOAT   ? new FloatValuePage{          *this, parentPage, parentMaster, *element.m_callbacks, element.m_title, element.m_description}
       : type                == property_element_c::EBMLT_INT     ? new UnsignedIntegerValuePage{*this, parentPage, parentMaster, *element.m_callbacks, element.m_title, element.m_description}
       : type                == property_element_c::EBMLT_UINT    ? new UnsignedIntegerValuePage{*this, parentPage, parentMaster, *element.m_callbacks, element.m_title, element.m_description}
       : type                == property_element_c::EBMLT_STRING  ? new AsciiStringValuePage{    *this, parentPage, parentMaster, *element.m_callbacks, element.m_title, element.m_description}
       : type                == property_element_c::EBMLT_USTRING ? new StringValuePage{         *this, parentPage, parentMaster, *element.m_callbacks, element.m_title, element.m_description}
       : type                == property_element_c::EBMLT_DATE    ? new TimeValuePage{           *this, parentPage, parentMaster, *element.m_callbacks, element.m_title, element.m_description}
       :                                                             static_cast<ValuePage *>(nullptr);

  if (page)
    page->init();

  return page;
}

void
Tab::handleSegmentInfo(kax_analyzer_data_c const &data) {
  m_eSegmentInfo = m_analyzer->read_element(data);
  if (!m_eSegmentInfo)
    return;

  auto &info = dynamic_cast<KaxInfo &>(*m_eSegmentInfo.get());
  auto page  = new TopLevelPage{*this, YT("Segment information")};
  page->setInternalIdentifier("segmentInfo");
  page->init();

  auto &propertyElements = property_element_c::get_table_for(KaxInfo::ClassInfos, nullptr, true);
  for (auto const &element : propertyElements)
    createValuePage(*page, info, element);

  m_segmentinfoPage = page;
}

void
Tab::handleTracks(kax_analyzer_data_c const &data) {
  m_eTracks = m_analyzer->read_element(data);
  if (!m_eTracks)
    return;

  auto trackIdxMkvmerge  = 0u;
  auto &propertyElements = property_element_c::get_table_for(KaxTracks::ClassInfos, nullptr, true);

  for (auto const &element : dynamic_cast<EbmlMaster &>(*m_eTracks)) {
    auto kTrackEntry = dynamic_cast<KaxTrackEntry *>(element);
    if (!kTrackEntry)
      continue;

    auto kTrackType = FindChild<KaxTrackType>(kTrackEntry);
    if (!kTrackType)
      continue;

    auto trackType = kTrackType->GetValue();
    auto page      = new TrackTypePage{*this, *kTrackEntry, trackIdxMkvmerge++};
    page->init();

    QHash<EbmlCallbacks const *, EbmlMaster *> parentMastersByCallback;
    QHash<EbmlCallbacks const *, TopLevelPage *> parentPagesByCallback;

    parentMastersByCallback[nullptr] = kTrackEntry;
    parentPagesByCallback[nullptr]   = page;

    if (track_video == trackType) {
      auto colourPage = new TopLevelPage{*this, YT("Colour information")};
      colourPage->setInternalIdentifier(Q("videoColour %1").arg(trackIdxMkvmerge - 1));
      colourPage->setParentPage(*page);
      colourPage->init();

      auto colourMasterMetaPage = new TopLevelPage{*this, YT("Colour mastering meta information")};
      colourMasterMetaPage->setInternalIdentifier(Q("videoColourMasterMeta %1").arg(trackIdxMkvmerge - 1));
      colourMasterMetaPage->setParentPage(*page);
      colourMasterMetaPage->init();

      auto projectionPage = new TopLevelPage{*this, YT("Video projection information")};
      projectionPage->setInternalIdentifier(Q("videoProjection %1").arg(trackIdxMkvmerge - 1));
      projectionPage->setParentPage(*page);
      projectionPage->init();

      parentMastersByCallback[&KaxTrackVideo::ClassInfos]            = &GetChildEmptyIfNew<KaxTrackVideo>(kTrackEntry);
      parentMastersByCallback[&KaxVideoColour::ClassInfos]           = &GetChildEmptyIfNew<KaxVideoColour>(parentMastersByCallback[&KaxTrackVideo::ClassInfos]);
      parentMastersByCallback[&KaxVideoColourMasterMeta::ClassInfos] = &GetChildEmptyIfNew<KaxVideoColourMasterMeta>(parentMastersByCallback[&KaxVideoColour::ClassInfos]);
      parentMastersByCallback[&KaxVideoProjection::ClassInfos]       = &GetChildEmptyIfNew<KaxVideoProjection>(parentMastersByCallback[&KaxTrackVideo::ClassInfos]);

      parentPagesByCallback[&KaxTrackVideo::ClassInfos]              = page;
      parentPagesByCallback[&KaxVideoColour::ClassInfos]             = colourPage;
      parentPagesByCallback[&KaxVideoColourMasterMeta::ClassInfos]   = colourMasterMetaPage;
      parentPagesByCallback[&KaxVideoProjection::ClassInfos]         = projectionPage;

    } else if (track_audio == trackType) {
      parentMastersByCallback[&KaxTrackAudio::ClassInfos]            = &GetChildEmptyIfNew<KaxTrackAudio>(kTrackEntry);
      parentPagesByCallback[&KaxTrackAudio::ClassInfos]              = page;
    }

    for (auto const &element : propertyElements) {
      auto parentMasterCallbacks = element.m_sub_sub_sub_master_callbacks ? element.m_sub_sub_sub_master_callbacks
                                 : element.m_sub_sub_master_callbacks     ? element.m_sub_sub_master_callbacks
                                 :                                          element.m_sub_master_callbacks;
      auto parentPage            = parentPagesByCallback[parentMasterCallbacks];
      auto parentMaster          = parentMastersByCallback[parentMasterCallbacks];

      if (parentPage && parentMaster)
        createValuePage(*parentPage, *parentMaster, element);
    }
  }
}

void
Tab::handleAttachments() {
  auto attachments = KaxAttachedList{};

  m_analyzer->with_elements(KaxAttachments::ClassInfos.GlobalId, [this, &attachments](kax_analyzer_data_c const &data) {
    auto master = std::dynamic_pointer_cast<KaxAttachments>(m_analyzer->read_element(data));
    if (!master)
      return;

    auto idx = 0u;
    while (idx < master->ListSize()) {
      auto attached = dynamic_cast<KaxAttached *>((*master)[idx]);
      if (attached) {
        attachments << KaxAttachedPtr{attached};
        master->Remove(idx);
      } else
        ++idx;
    }
  });

  m_attachmentsPage = new AttachmentsPage{*this, attachments};
  m_attachmentsPage->init();
}

void
Tab::validate() {
  auto pageIdx = m_model->validate();
  // TODO: Tab::validate: handle attachments

  if (!pageIdx.isValid()) {
    Util::MessageBox::information(this)->title(QY("Header validation")).text(QY("All header values are OK.")).exec();
    return;
  }

  reportValidationFailure(false, pageIdx);
}

void
Tab::reportValidationFailure(bool isCritical,
                             QModelIndex const &pageIdx) {
  ui->elements->selectionModel()->setCurrentIndex(pageIdx, QItemSelectionModel::ClearAndSelect);
  ui->elements->selectionModel()->select(pageIdx, QItemSelectionModel::ClearAndSelect);
  selectionChanged(pageIdx, QModelIndex{});

  if (isCritical)
    Util::MessageBox::critical(this)->title(QY("Header validation")).text(QY("There were errors in the header values preventing the headers from being saved. The first error has been selected.")).exec();
  else
    Util::MessageBox::warning(this)->title(QY("Header validation")).text(QY("There were errors in the header values preventing the headers from being saved. The first error has been selected.")).exec();
}

void
Tab::expandAll() {
  expandCollapseAll(true);
}

void
Tab::collapseAll() {
  expandCollapseAll(false);
}

void
Tab::expandCollapseAll(bool expand,
                       QModelIndex const &parentIdx) {
  if (parentIdx.isValid())
    ui->elements->setExpanded(parentIdx, expand);

  for (auto row = 0, numRows = m_model->rowCount(parentIdx); row < numRows; ++row)
    expandCollapseAll(expand, m_model->index(row, 0, parentIdx));
}

void
Tab::showTreeContextMenu(QPoint const &pos) {
  auto selectedPage       = currentlySelectedPage();
  auto isAttachmentsPage  = !!dynamic_cast<AttachmentsPage *>(selectedPage);
  auto isAttachedFilePage = !!dynamic_cast<AttachedFilePage *>(selectedPage);
  auto isAttachments      = isAttachmentsPage || isAttachedFilePage;
  auto actions            = m_treeContextMenu->actions();

  for (auto const &action : actions)
    if (!action->isSeparator())
      m_treeContextMenu->removeAction(action);

  m_treeContextMenu->clear();

  m_treeContextMenu->addAction(m_expandAllAction);
  m_treeContextMenu->addAction(m_collapseAllAction);
  m_treeContextMenu->addSeparator();
  m_treeContextMenu->addAction(m_addAttachmentsAction);

  if (isAttachments) {
    m_treeContextMenu->addAction(m_removeAttachmentAction);
    m_treeContextMenu->addSeparator();
    m_treeContextMenu->addAction(m_saveAttachmentContentAction);
    m_treeContextMenu->addAction(m_replaceAttachmentContentAction);
    m_treeContextMenu->addAction(m_replaceAttachmentContentSetValuesAction);

    m_removeAttachmentAction->setEnabled(isAttachedFilePage);
    m_saveAttachmentContentAction->setEnabled(isAttachedFilePage);
    m_replaceAttachmentContentAction->setEnabled(isAttachedFilePage);
    m_replaceAttachmentContentSetValuesAction->setEnabled(isAttachedFilePage);
  }

  m_treeContextMenu->exec(ui->elements->viewport()->mapToGlobal(pos));
}

void
Tab::selectAttachmentsAndAdd() {
  auto &settings = Util::Settings::get();
  auto fileNames = Util::getOpenFileNames(this, QY("Add attachments"), settings.lastOpenDirPath(), QY("All files") + Q(" (*)"));

  if (fileNames.isEmpty())
    return;

  settings.m_lastOpenDir = QFileInfo{fileNames[0]}.path();
  settings.save();

  addAttachments(fileNames);
}

void
Tab::addAttachment(KaxAttachedPtr const &attachment) {
  if (!attachment)
    return;

  auto page = new AttachedFilePage{*this, *m_attachmentsPage, attachment};
  page->init();
}

void
Tab::addAttachments(QStringList const &fileNames) {
  for (auto const &fileName : fileNames)
    addAttachment(createAttachmentFromFile(fileName));

  ui->elements->setExpanded(m_attachmentsPage->m_pageIdx, true);
}

void
Tab::removeSelectedAttachment() {
  auto selectedPage = dynamic_cast<AttachedFilePage *>(currentlySelectedPage());
  if (!selectedPage)
    return;

  auto idx = m_model->indexFromPage(selectedPage);
  if (idx.isValid())
    m_model->removeRow(idx.row(), idx.parent());

  m_attachmentsPage->m_children.removeAll(selectedPage);
  m_model->deletePage(selectedPage);
}

memory_cptr
Tab::readFileData(QWidget *parent,
                  QString const &fileName) {
  auto info = QFileInfo{fileName};
  if (info.size() > 0x7fffffff) {
    Util::MessageBox::critical(parent)
      ->title(QY("Reading failed"))
      .text(Q("%1 %2")
            .arg(QY("The file (%1) is too big (%2).").arg(fileName).arg(Q(format_file_size(info.size()))))
            .arg(QY("Only files smaller than 2 GiB are supported.")))
      .exec();
    return {};
  }

  try {
    return mm_file_io_c::slurp(to_utf8(fileName));

  } catch (mtx::mm_io::end_of_file_x &) {
    Util::MessageBox::critical(parent)->title(QY("Reading failed")).text(QY("The file you tried to open (%1) could not be read successfully.").arg(fileName)).exec();
  }

  return {};
}

KaxAttachedPtr
Tab::createAttachmentFromFile(QString const &fileName) {
  auto content = readFileData(this, fileName);
  if (!content)
    return {};

  auto mimeType   = guess_mime_type(to_utf8(fileName), true);
  auto uid        = create_unique_number(UNIQUE_ATTACHMENT_IDS);
  auto fileData   = new KaxFileData;
  auto attachment = KaxAttachedPtr{
    mtx::construct::cons<KaxAttached>(new KaxFileName, to_wide(QFileInfo{fileName}.fileName()),
                                      new KaxMimeType, mimeType,
                                      new KaxFileUID,  uid)
  };

  fileData->SetBuffer(content->get_buffer(), content->get_size());
  content->lock();
  attachment->PushElement(*fileData);

  return attachment;
}

void
Tab::saveAttachmentContent() {
  auto page = dynamic_cast<AttachedFilePage *>(currentlySelectedPage());
  if (page)
    page->saveContent();
}

void
Tab::replaceAttachmentContent(bool deriveNameAndMimeType) {
  auto page = dynamic_cast<AttachedFilePage *>(currentlySelectedPage());
  if (page)
    page->replaceContent(deriveNameAndMimeType);
}

void
Tab::handleDroppedFiles(QStringList const &fileNames,
                        Qt::MouseButtons mouseButtons) {
  if (fileNames.isEmpty())
    return;

  auto &settings = Util::Settings::get();
  auto decision  = settings.m_headerEditorDroppedFilesPolicy;

  if (   (Util::Settings::HeaderEditorDroppedFilesPolicy::Ask == decision)
      || ((mouseButtons & Qt::RightButton)                    == Qt::RightButton)) {
    ActionForDroppedFilesDialog dlg{this};
    if (!dlg.exec())
      return;

    decision = dlg.decision();

    if (dlg.alwaysUseThisDecision()) {
      settings.m_headerEditorDroppedFilesPolicy = decision;
      settings.save();
    }
  }

  if (Util::Settings::HeaderEditorDroppedFilesPolicy::Open == decision)
    MainWindow::get()->headerEditorTool()->openFiles(fileNames);

  else
    addAttachments(fileNames);
}

void
Tab::focusPage(PageBase *page) {
  auto idx = m_model->indexFromPage(page);
  if (!idx.isValid())
    return;

  auto selection = QItemSelection{idx.sibling(idx.row(), 0), idx.sibling(idx.row(), m_model->columnCount() - 1)};

  m_ignoreSelectionChanges = true;

  ui->elements->selectionModel()->setCurrentIndex(idx.sibling(idx.row(), 0), QItemSelectionModel::ClearAndSelect);
  ui->elements->selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect);
  ui->pageContainer->setCurrentWidget(page);

  m_ignoreSelectionChanges = false;
}

bool
Tab::isClosingOrReloadingOkIfModified(ModifiedConfirmationMode mode) {
  if (!Util::Settings::get().m_warnBeforeClosingModifiedTabs)
    return true;

  auto modifiedPage = hasBeenModified();
  if (!modifiedPage)
    return true;

  auto tool = MainWindow::headerEditorTool();
  MainWindow::get()->switchToTool(tool);
  tool->showTab(*this);
  focusPage(modifiedPage);

  auto closing  = mode == ModifiedConfirmationMode::Closing;
  auto text     = closing ? QY("The file \"%1\" has been modified. Do you really want to close? All changes will be lost.")
                :           QY("The file \"%1\" has been modified. Do you really want to reload it? All changes will be lost.");
  auto title    = closing ? QY("Close modified file") : QY("Reload modified file");
  auto yesLabel = closing ? QY("&Close file")         : QY("&Reload file");

  auto answer   = Util::MessageBox::question(this)
    ->title(title)
    .text(text.arg(QFileInfo{fileName()}.fileName()))
    .buttonLabel(QMessageBox::Yes, yesLabel)
    .buttonLabel(QMessageBox::No,  QY("Cancel"))
    .exec();

  return answer == QMessageBox::Yes;
}

}}}
