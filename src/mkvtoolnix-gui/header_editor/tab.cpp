#include "common/common_pch.h"

#include <QCheckBox>
#include <QDir>
#include <QFileInfo>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>

#include <matroska/KaxSemantic.h>

#include "common/construct.h"
#include "common/doc_type_version_handler.h"
#include "common/ebml.h"
#include "common/list_utils.h"
#include "common/mime.h"
#include "common/mm_io_x.h"
#include "common/mm_file_io.h"
#include "common/property_element.h"
#include "common/qt.h"
#include "common/segmentinfo.h"
#include "common/strings/formatting.h"
#include "common/unique_numbers.h"
#include "mkvtoolnix-gui/app.h"
#include "mkvtoolnix-gui/forms/header_editor/tab.h"
#include "mkvtoolnix-gui/header_editor/action_for_dropped_files_dialog.h"
#include "mkvtoolnix-gui/header_editor/ascii_string_value_page.h"
#include "mkvtoolnix-gui/header_editor/attached_file_page.h"
#include "mkvtoolnix-gui/header_editor/attachments_page.h"
#include "mkvtoolnix-gui/header_editor/bit_value_page.h"
#include "mkvtoolnix-gui/header_editor/bool_value_page.h"
#include "mkvtoolnix-gui/header_editor/float_value_page.h"
#include "mkvtoolnix-gui/header_editor/language_ietf_value_page.h"
#include "mkvtoolnix-gui/header_editor/language_value_page.h"
#include "mkvtoolnix-gui/header_editor/page_model.h"
#include "mkvtoolnix-gui/header_editor/string_value_page.h"
#include "mkvtoolnix-gui/header_editor/tab.h"
#include "mkvtoolnix-gui/header_editor/time_value_page.h"
#include "mkvtoolnix-gui/header_editor/tool.h"
#include "mkvtoolnix-gui/header_editor/top_level_page.h"
#include "mkvtoolnix-gui/header_editor/track_name_page.h"
#include "mkvtoolnix-gui/header_editor/track_type_page.h"
#include "mkvtoolnix-gui/header_editor/unsigned_integer_value_page.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/util/basic_tree_view.h"
#include "mkvtoolnix-gui/util/file.h"
#include "mkvtoolnix-gui/util/file_dialog.h"
#include "mkvtoolnix-gui/util/header_view_manager.h"
#include "mkvtoolnix-gui/util/model.h"
#include "mkvtoolnix-gui/util/message_box.h"
#include "mkvtoolnix-gui/util/settings.h"
#include "mkvtoolnix-gui/util/tree.h"
#include "mkvtoolnix-gui/util/widget.h"

using namespace mtx::gui;

namespace mtx::gui::HeaderEditor {

Tab::Tab(QWidget *parent,
         QString const &fileName)
  : QWidget{parent}
  , ui{new Ui::Tab}
  , m_fileName{fileName}
  , m_model{new PageModel{this}}
  , m_treeContextMenu{new QMenu{this}}
  , m_modifySelectedTrackMenu{new QMenu{this}}
  , m_languageShortcutsMenu{new QMenu{this}}
  , m_expandAllAction{new QAction{this}}
  , m_collapseAllAction{new QAction{this}}
  , m_addAttachmentsAction{new QAction{this}}
  , m_removeAttachmentAction{new QAction{this}}
  , m_removeAllAttachmentsAction{new QAction{this}}
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
  m_tracksReordered = false;
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
    Q_EMIT removeThisTab();
    return;
  }

  m_analyzer = std::make_unique<Util::KaxAnalyzer>(this, m_fileName);
  bool ok    = false;
  QString error;

  try {
    ok = m_analyzer->set_parse_mode(kax_analyzer_c::parse_mode_fast)
      .set_open_mode(libebml::MODE_READ)
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
    Q_EMIT removeThisTab();
    return;
  }

  populateTree();

  m_analyzer->close_file();

  for (auto const &page : m_model->allExpandablePages()) {
    auto key = dynamic_cast<TopLevelPage &>(*page).internalIdentifier();
    ui->elements->setExpanded(page->m_pageIdx, expansionStatus[key]);
  }

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

  if (!segmentinfoModified && !tracksModified && !attachmentsModified && !m_tracksReordered) {
    Util::MessageBox::information(this)->title(QY("File has not been modified")).text(QY("The header values have not been modified. There is nothing to save.")).exec();
    return;
  }

  auto pageIdx = m_model->validate();
  if (pageIdx.isValid()) {
    reportValidationFailure(false, pageIdx);
    return;
  }

  auto trackUIDChanges = determineTrackUIDChanges();

  doModifications();

  bool ok = true;

  try {
    mtx::doc_type_version_handler_c doc_type_version_handler;

    m_analyzer->set_doc_type_version_handler(&doc_type_version_handler);

    if (segmentinfoModified && m_eSegmentInfo) {
      auto result = m_analyzer->update_element(m_eSegmentInfo, true);
      if (kax_analyzer_c::uer_success != result) {
        Util::KaxAnalyzer::displayUpdateElementResult(this, result, QY("Saving the modified segment information header failed."));
        ok = false;
      }
    }

    if (ok && m_eTracks && (tracksModified || m_tracksReordered)) {
      updateTracksElementToMatchTrackOrder();

      auto result = m_analyzer->update_element(m_eTracks, true);
      if (kax_analyzer_c::uer_success != result) {
        Util::KaxAnalyzer::displayUpdateElementResult(this, result, QY("Saving the modified track headers failed."));
        ok = false;
      }
    }

    if (ok && attachmentsModified) {
      auto attachments = std::make_shared<libmatroska::KaxAttachments>();

      for (auto const &attachedFilePage : m_attachmentsPage->m_children)
        attachments->PushElement(*dynamic_cast<AttachedFilePage &>(*attachedFilePage).m_attachment.get());

      auto result = attachments->ListSize() ? m_analyzer->update_element(attachments.get(), true)
                  :                           m_analyzer->remove_elements(EBML_ID(libmatroska::KaxAttachments));

      attachments->RemoveAll();

      if (kax_analyzer_c::uer_success != result) {
        Util::KaxAnalyzer::displayUpdateElementResult(this, result, QY("Saving the modified attachments failed."));
        ok = false;
      }
    }

    if (ok && !trackUIDChanges.empty()) {
      auto result = m_analyzer->update_uid_referrals(trackUIDChanges);

      if (kax_analyzer_c::uer_success != result) {
        Util::KaxAnalyzer::displayUpdateElementResult(this, result, QY("Saving the modified attachments failed."));
        ok = false;
      }
    }

    if (ok) {
      auto result = doc_type_version_handler.update_ebml_head(m_analyzer->get_file());
      if (!mtx::included_in(result, mtx::doc_type_version_handler_c::update_result_e::ok_updated, mtx::doc_type_version_handler_c::update_result_e::ok_no_update_needed)) {
        ok           = false;
        auto details = mtx::doc_type_version_handler_c::update_result_e::err_no_head_found    == result ? QY("No 'EBML head' element was found.")
                     : mtx::doc_type_version_handler_c::update_result_e::err_not_enough_space == result ? QY("There's not enough space at the beginning of the file to fit the updated 'EBML head' element in.")
                     :                                                                                    QY("A generic read or write failure occurred.");
        auto message = Q("%1 %2").arg(QY("Updating the 'document type version' or 'document type read version' header fields failed.")).arg(details);

        QMessageBox::warning(this, QY("Error writing Matroska file"), message);
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
  setupModifyTracksMenu();

  Util::Settings::get().handleSplitterSizes(ui->headerEditorSplitter);

  auto info = QFileInfo{m_fileName};
  ui->fileName->setText(info.fileName());
  ui->directory->setText(QDir::toNativeSeparators(info.path()));

  ui->elements->setModel(m_model);
  ui->elements->acceptDroppedFiles(true);

  Util::HeaderViewManager::create(*ui->elements, "HeaderEditor::Elements").setDefaultSizes({ { Q("type"), 250 }, { Q("codec"), 100 }, { Q("language"), 120 }, { Q("properties"), 120 } });
  Util::preventScrollingWithoutFocus(this);

  auto &mts = m_modifyTracksSubmenu;

  connect(ui->elements,                              &Util::BasicTreeView::customContextMenuRequested,    this, &Tab::showTreeContextMenu);
  connect(ui->elements,                              &Util::BasicTreeView::filesDropped,                  this, &Tab::handleDroppedFiles);
  connect(ui->elements,                              &Util::BasicTreeView::deletePressed,                 this, &Tab::removeSelectedAttachment);
  connect(ui->elements,                              &Util::BasicTreeView::insertPressed,                 this, &Tab::selectAttachmentsAndAdd);
  connect(ui->elements,                              &Util::BasicTreeView::ctrlDownPressed,               this, [this]() { moveElementUpOrDown(false); });
  connect(ui->elements,                              &Util::BasicTreeView::ctrlUpPressed,                 this, [this]() { moveElementUpOrDown(true); });
  connect(ui->elements->selectionModel(),            &QItemSelectionModel::currentChanged,                this, &Tab::selectionChanged);
  connect(m_expandAllAction,                         &QAction::triggered,                                 this, &Tab::expandAll);
  connect(m_collapseAllAction,                       &QAction::triggered,                                 this, &Tab::collapseAll);
  connect(m_addAttachmentsAction,                    &QAction::triggered,                                 this, &Tab::selectAttachmentsAndAdd);
  connect(m_removeAttachmentAction,                  &QAction::triggered,                                 this, &Tab::removeSelectedAttachment);
  connect(m_removeAllAttachmentsAction,              &QAction::triggered,                                 this, &Tab::removeAllAttachments);
  connect(m_saveAttachmentContentAction,             &QAction::triggered,                                 this, &Tab::saveAttachmentContent);
  connect(mts.m_toggleTrackEnabledFlag,              &QAction::triggered,                                 this, &Tab::toggleTrackFlag);
  connect(mts.m_toggleDefaultTrackFlag,              &QAction::triggered,                                 this, &Tab::toggleTrackFlag);
  connect(mts.m_toggleForcedDisplayFlag,             &QAction::triggered,                                 this, &Tab::toggleTrackFlag);
  connect(mts.m_toggleCommentaryFlag,                &QAction::triggered,                                 this, &Tab::toggleTrackFlag);
  connect(mts.m_toggleOriginalFlag,                  &QAction::triggered,                                 this, &Tab::toggleTrackFlag);
  connect(mts.m_toggleHearingImpairedFlag,           &QAction::triggered,                                 this, &Tab::toggleTrackFlag);
  connect(mts.m_toggleVisualImpairedFlag,            &QAction::triggered,                                 this, &Tab::toggleTrackFlag);
  connect(mts.m_toggleTextDescriptionsFlag,          &QAction::triggered,                                 this, &Tab::toggleTrackFlag);
  connect(&mts,                                      &Util::ModifyTracksSubmenu::languageChangeRequested, this, &Tab::changeTrackLanguage);
  connect(m_replaceAttachmentContentAction,          &QAction::triggered,                                 [this]() { replaceAttachmentContent(false); });
  connect(m_replaceAttachmentContentSetValuesAction, &QAction::triggered,                                 [this]() { replaceAttachmentContent(true); });
  connect(m_model,                                   &PageModel::attachmentsReordered,                    [this]() { m_attachmentsPage->rereadChildren(*m_model); });
  connect(m_model,                                   &PageModel::tracksReordered,                         this, &Tab::handleReorderedTracks);
  connect(MainWindow::get(),                         &MainWindow::preferencesChanged,                     [this]() { m_modifyTracksSubmenu.setupLanguage(*m_languageShortcutsMenu); });
}

void
Tab::setupModifyTracksMenu() {
  m_modifySelectedTrackMenu->addMenu(m_languageShortcutsMenu);
  m_modifySelectedTrackMenu->addSeparator();

  m_modifyTracksSubmenu.setupTrack(*m_modifySelectedTrackMenu);
  m_modifyTracksSubmenu.setupLanguage(*m_languageShortcutsMenu);
}

void
Tab::handleReorderedTracks() {
  m_tracksReordered = true;
  m_model->rereadTopLevelPageIndexes();
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
  m_removeAttachmentAction->setText(QY("&Remove selected attachment"));
  m_removeAllAttachmentsAction->setText(QY("Remove a&ll attachments"));
  m_saveAttachmentContentAction->setText(QY("&Save attachment content to a file"));
  m_replaceAttachmentContentAction->setText(QY("Re&place attachment with a new file"));
  m_replaceAttachmentContentSetValuesAction->setText(QY("Replace attachment with a new file and &derive name && MIME type from it"));
  m_modifySelectedTrackMenu->setTitle(QY("Modif&y selected track"));

  m_addAttachmentsAction->setIcon(QIcon::fromTheme(Q("list-add")));
  m_removeAttachmentAction->setIcon(QIcon::fromTheme(Q("list-remove")));
  m_saveAttachmentContentAction->setIcon(QIcon::fromTheme(Q("document-save")));
  m_replaceAttachmentContentAction->setIcon(QIcon::fromTheme(Q("document-open")));

  m_modifyTracksSubmenu.retranslateUi();
  m_languageShortcutsMenu->setTitle(QY("Set &language"));

  setupToolTips();

  for (auto const &page : m_model->pages())
    page->retranslateUi();

  m_model->retranslateUi();
}

void
Tab::setupToolTips() {
  Util::setToolTip(ui->elements, QY("Right-click for actions for header elements and attachments"));
}

void
Tab::populateTree() {
  m_analyzer->with_elements(EBML_ID(libmatroska::KaxInfo), [this](kax_analyzer_data_c const &data) {
    handleSegmentInfo(data);
  });

  m_analyzer->with_elements(EBML_ID(libmatroska::KaxTracks), [this](kax_analyzer_data_c const &data) {
    handleTracks(data);
  });

  handleAttachments();

  for (auto const &page : m_model->topLevelPages())
    if (dynamic_cast<TrackTypePage *>(page))
      static_cast<TrackTypePage *>(page)->updateModelItems();
}

void
Tab::selectionChanged(QModelIndex const &current,
                      QModelIndex const &) {
  if (m_ignoreSelectionChanges)
    return;

  m_model->rememberLastSelectedIndex(current);

  auto selectedPage = m_model->selectedPage(current);
  if (selectedPage)
    ui->pageContainer->setCurrentWidget(selectedPage);

  MainWindow::get()->headerEditorTool()->enableMenuActions();
}

bool
Tab::isTrackSelected() {
  auto topLevelIdx = ui->elements->selectionModel()->currentIndex();

  while (topLevelIdx.parent().isValid())
    topLevelIdx = topLevelIdx.parent();

  return !!dynamic_cast<TrackTypePage *>(m_model->selectedPage(topLevelIdx));
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
  for (auto const &page : m_model->topLevelPages()) {
    auto modifiedPage = page->hasBeenModified();
    if (modifiedPage)
      return modifiedPage;
  }

  return nullptr;
}

void
Tab::pruneEmptyMastersForTrack(TrackTypePage &page) {
  auto trackType = find_child_value<libmatroska::KaxTrackType>(page.m_master);

  if (!mtx::included_in(trackType, track_video, track_audio))
    return;

  std::unordered_map<libebml::EbmlMaster *, bool> handled;

  if (trackType == track_video) {
    auto trackVideo           = &get_child_empty_if_new<libmatroska::KaxTrackVideo>(page.m_master);
    auto videoColor           = &get_child_empty_if_new<libmatroska::KaxVideoColour>(trackVideo);
    auto videoColorMasterMeta = &get_child_empty_if_new<libmatroska::KaxVideoColourMasterMeta>(videoColor);
    auto videoProjection      = &get_child_empty_if_new<libmatroska::KaxVideoProjection>(trackVideo);

    remove_master_from_parent_if_empty_or_only_defaults(videoColor,     videoColorMasterMeta, handled);
    remove_master_from_parent_if_empty_or_only_defaults(trackVideo,     videoColor,           handled);
    remove_master_from_parent_if_empty_or_only_defaults(trackVideo,     videoProjection,      handled);
    remove_master_from_parent_if_empty_or_only_defaults(&page.m_master, trackVideo,           handled);

  } else
    // trackType is track_audio
    remove_master_from_parent_if_empty_or_only_defaults(&page.m_master, &get_child_empty_if_new<libmatroska::KaxTrackAudio>(page.m_master), handled);
}

void
Tab::pruneEmptyMastersForAllTracks() {
  for (auto const &page : m_model->topLevelPages())
    if (dynamic_cast<TrackTypePage *>(page))
      pruneEmptyMastersForTrack(static_cast<TrackTypePage &>(*page));
}

std::unordered_map<uint64_t, uint64_t>
Tab::determineTrackUIDChanges() {
  std::unordered_map<uint64_t, uint64_t> changes;

  for (auto const &topLevelPage : m_model->topLevelPages()) {
    for (auto const &childPage : topLevelPage->m_children) {
      auto uiValuePage = dynamic_cast<UnsignedIntegerValuePage *>(childPage);
      if (!uiValuePage)
        continue;

      if (uiValuePage->m_callbacks.ClassId() != EBML_ID(libmatroska::KaxTrackUID))
        continue;

      if (uiValuePage->m_cbAddOrRemove->isChecked())
        continue;

      auto currentValue = uiValuePage->m_leValue->text().toULongLong();
      if (uiValuePage->m_originalValue != currentValue)
        changes[uiValuePage->m_originalValue] = currentValue;
    }
  }

  return changes;
}

void
Tab::doModifications() {
  for (auto const &page : m_model->topLevelPages())
    page->doModifications();

  pruneEmptyMastersForAllTracks();

  if (m_eSegmentInfo) {
    fix_mandatory_elements(m_eSegmentInfo.get());
    m_eSegmentInfo->UpdateSize(render_should_write_arg(true), true);
  }

  if (m_eTracks) {
    fix_mandatory_elements(m_eTracks.get());
    m_eTracks->UpdateSize(render_should_write_arg(true), true);
  }
}

ValuePage *
Tab::createValuePage(TopLevelPage &parentPage,
                     libebml::EbmlMaster &parentMaster,
                     property_element_c const &element) {
  ValuePage *page{};
  auto const type = element.m_type;

  page = element.m_callbacks == &EBML_INFO(libmatroska::KaxTrackLanguage)      ? new LanguageValuePage{       *this, parentPage, parentMaster, *element.m_callbacks, element.m_title, element.m_description}
       : element.m_callbacks == &EBML_INFO(libmatroska::KaxLanguageIETF)       ? new LanguageIETFValuePage{   *this, parentPage, parentMaster, *element.m_callbacks, element.m_title, element.m_description}
       : element.m_callbacks == &EBML_INFO(libmatroska::KaxTrackName)          ? new TrackNamePage{           *this, parentPage, parentMaster, *element.m_callbacks, element.m_title, element.m_description}
       : type                == property_element_c::EBMLT_BOOL    ? new BoolValuePage{           *this, parentPage, parentMaster, *element.m_callbacks, element.m_title, element.m_description}
       : type                == property_element_c::EBMLT_BINARY  ? new BitValuePage{            *this, parentPage, parentMaster, *element.m_callbacks, element.m_title, element.m_description, element.m_bit_length}
       : type                == property_element_c::EBMLT_FLOAT   ? new FloatValuePage{          *this, parentPage, parentMaster, *element.m_callbacks, element.m_title, element.m_description}
       : type                == property_element_c::EBMLT_INT     ? new UnsignedIntegerValuePage{*this, parentPage, parentMaster, *element.m_callbacks, element.m_title, element.m_description}
       : type                == property_element_c::EBMLT_UINT    ? new UnsignedIntegerValuePage{*this, parentPage, parentMaster, *element.m_callbacks, element.m_title, element.m_description}
       : type                == property_element_c::EBMLT_STRING  ? new AsciiStringValuePage{    *this, parentPage, parentMaster, *element.m_callbacks, element.m_title, element.m_description}
       : type                == property_element_c::EBMLT_USTRING ? new StringValuePage{         *this, parentPage, parentMaster, *element.m_callbacks, element.m_title, element.m_description}
       : type                == property_element_c::EBMLT_DATE    ? new TimeValuePage{           *this, parentPage, parentMaster, *element.m_callbacks, element.m_title, element.m_description}
       :                                                            static_cast<ValuePage *>(nullptr);

  if (page) {
    page->init();
    if (dynamic_cast<TrackTypePage *>(&parentPage))
      connect(page, &ValuePage::valueChanged, static_cast<TrackTypePage *>(&parentPage), &TrackTypePage::updateModelItems);
  }

  return page;
}

void
Tab::handleSegmentInfo(kax_analyzer_data_c const &data) {
  m_eSegmentInfo = m_analyzer->read_element(data);
  if (!m_eSegmentInfo)
    return;

  auto &info = dynamic_cast<libmatroska::KaxInfo &>(*m_eSegmentInfo.get());
  auto page  = new TopLevelPage{*this, YT("Segment information")};
  page->setInternalIdentifier("segmentInfo");
  page->init();

  auto &propertyElements = property_element_c::get_table_for(EBML_INFO(libmatroska::KaxInfo), nullptr, true);
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
  auto &propertyElements = property_element_c::get_table_for(EBML_INFO(libmatroska::KaxTracks), nullptr, true);

  for (auto const &element : dynamic_cast<libebml::EbmlMaster &>(*m_eTracks)) {
    auto kTrackEntry = dynamic_cast<libmatroska::KaxTrackEntry *>(element);
    if (!kTrackEntry)
      continue;

    auto kTrackType = find_child<libmatroska::KaxTrackType>(kTrackEntry);
    if (!kTrackType)
      continue;

    auto trackType = kTrackType->GetValue();
    auto page      = new TrackTypePage{*this, *kTrackEntry, trackIdxMkvmerge++};
    page->init();

    QHash<libebml::EbmlCallbacks const *, libebml::EbmlMaster *> parentMastersByCallback;
    QHash<libebml::EbmlCallbacks const *, TopLevelPage *> parentPagesByCallback;

    parentMastersByCallback[nullptr] = kTrackEntry;
    parentPagesByCallback[nullptr]   = page;

    if (track_video == trackType) {
      auto colorPage = new TopLevelPage{*this, YT("Color information")};
      colorPage->setInternalIdentifier(Q("videoColor %1").arg(trackIdxMkvmerge - 1));
      colorPage->setParentPage(*page);
      colorPage->init();

      auto colorMasterMetaPage = new TopLevelPage{*this, YT("Color mastering meta information")};
      colorMasterMetaPage->setInternalIdentifier(Q("videoColorMasterMeta %1").arg(trackIdxMkvmerge - 1));
      colorMasterMetaPage->setParentPage(*page);
      colorMasterMetaPage->init();

      auto projectionPage = new TopLevelPage{*this, YT("Video projection information")};
      projectionPage->setInternalIdentifier(Q("videoProjection %1").arg(trackIdxMkvmerge - 1));
      projectionPage->setParentPage(*page);
      projectionPage->init();

      parentMastersByCallback[&EBML_INFO(libmatroska::KaxTrackVideo)]            = &get_child_empty_if_new<libmatroska::KaxTrackVideo>(kTrackEntry);
      parentMastersByCallback[&EBML_INFO(libmatroska::KaxVideoColour)]           = &get_child_empty_if_new<libmatroska::KaxVideoColour>(parentMastersByCallback[&EBML_INFO(libmatroska::KaxTrackVideo)]);
      parentMastersByCallback[&EBML_INFO(libmatroska::KaxVideoColourMasterMeta)] = &get_child_empty_if_new<libmatroska::KaxVideoColourMasterMeta>(parentMastersByCallback[&EBML_INFO(libmatroska::KaxVideoColour)]);
      parentMastersByCallback[&EBML_INFO(libmatroska::KaxVideoProjection)]       = &get_child_empty_if_new<libmatroska::KaxVideoProjection>(parentMastersByCallback[&EBML_INFO(libmatroska::KaxTrackVideo)]);

      parentPagesByCallback[&EBML_INFO(libmatroska::KaxTrackVideo)]              = page;
      parentPagesByCallback[&EBML_INFO(libmatroska::KaxVideoColour)]             = colorPage;
      parentPagesByCallback[&EBML_INFO(libmatroska::KaxVideoColourMasterMeta)]   = colorMasterMetaPage;
      parentPagesByCallback[&EBML_INFO(libmatroska::KaxVideoProjection)]         = projectionPage;

    } else if (track_audio == trackType) {
      parentMastersByCallback[&EBML_INFO(libmatroska::KaxTrackAudio)]            = &get_child_empty_if_new<libmatroska::KaxTrackAudio>(kTrackEntry);
      parentPagesByCallback[&EBML_INFO(libmatroska::KaxTrackAudio)]              = page;
    }

    for (auto const &propElement : propertyElements) {
      auto parentMasterCallbacks = propElement.m_sub_sub_sub_master_callbacks ? propElement.m_sub_sub_sub_master_callbacks
                                 : propElement.m_sub_sub_master_callbacks     ? propElement.m_sub_sub_master_callbacks
                                 :                                              propElement.m_sub_master_callbacks;
      auto parentPage            = parentPagesByCallback[parentMasterCallbacks];
      auto parentMaster          = parentMastersByCallback[parentMasterCallbacks];

      if (parentPage && parentMaster)
        createValuePage(*parentPage, *parentMaster, propElement);
    }

    page->deriveLanguageIETFFromLegacyIfNotPresent();
  }
}

void
Tab::handleAttachments() {
  auto attachments = KaxAttachedList{};

  m_analyzer->with_elements(EBML_ID(libmatroska::KaxAttachments), [this, &attachments](kax_analyzer_data_c const &data) {
    auto master = std::dynamic_pointer_cast<libmatroska::KaxAttachments>(m_analyzer->read_element(data));
    if (!master)
      return;

    auto idx = 0u;
    while (idx < master->ListSize()) {
      auto attached = dynamic_cast<libmatroska::KaxAttached *>((*master)[idx]);
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
  Util::expandCollapseAll(ui->elements, true);
}

void
Tab::collapseAll() {
  Util::expandCollapseAll(ui->elements, false);
}

void
Tab::showTreeContextMenu(QPoint const &pos) {
  auto selectedPage       = currentlySelectedPage();
  auto isAttachmentsPage  = !!dynamic_cast<AttachmentsPage *>(selectedPage);
  auto isAttachedFilePage = !!dynamic_cast<AttachedFilePage *>(selectedPage);
  auto isAttachments      = isAttachmentsPage || isAttachedFilePage;
  auto isTrack            = isTrackSelected();
  auto actions            = m_treeContextMenu->actions();

  for (auto const &action : actions)
    if (!action->isSeparator())
      m_treeContextMenu->removeAction(action);

  m_treeContextMenu->clear();

  m_treeContextMenu->addAction(m_expandAllAction);
  m_treeContextMenu->addAction(m_collapseAllAction);

  if (isTrack) {
    m_treeContextMenu->addSeparator();
    m_treeContextMenu->addMenu(m_modifySelectedTrackMenu);
  }

  m_treeContextMenu->addSeparator();
  m_treeContextMenu->addAction(m_addAttachmentsAction);

  if (isAttachments) {
    m_treeContextMenu->addAction(m_removeAttachmentAction);
    m_treeContextMenu->addAction(m_removeAllAttachmentsAction);
    m_treeContextMenu->addSeparator();
    m_treeContextMenu->addAction(m_saveAttachmentContentAction);
    m_treeContextMenu->addAction(m_replaceAttachmentContentAction);
    m_treeContextMenu->addAction(m_replaceAttachmentContentSetValuesAction);

    m_removeAttachmentAction->setEnabled(isAttachedFilePage);
    m_removeAllAttachmentsAction->setEnabled(!m_attachmentsPage->m_children.isEmpty());
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

  settings.m_lastOpenDir.setPath(QFileInfo{fileNames[0]}.path());
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

void
Tab::removeAllAttachments() {
  auto attachmentsItem = m_model->itemFromIndex(m_attachmentsPage->m_pageIdx);

  m_model->removeRows(0, attachmentsItem->rowCount(), m_attachmentsPage->m_pageIdx);

  for (auto const &attachmentPage : m_attachmentsPage->m_children)
    m_model->deletePage(attachmentPage);

  m_attachmentsPage->m_children.clear();
}

memory_cptr
Tab::readFileData(QWidget *parent,
                  QString const &fileName) {
  auto info = QFileInfo{fileName};
  if (info.size() > 0x7fffffff) {
    Util::MessageBox::critical(parent)
      ->title(QY("Reading failed"))
      .text(Q("%1 %2")
            .arg(QY("The file (%1) is too big (%2).").arg(fileName).arg(Q(mtx::string::format_file_size(info.size()))))
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

  auto mimeType   = Util::detectMIMEType(fileName);
  auto uid        = create_unique_number(UNIQUE_ATTACHMENT_IDS);
  auto fileData   = new libmatroska::KaxFileData;
  auto attachment = KaxAttachedPtr{
    mtx::construct::cons<libmatroska::KaxAttached>(new libmatroska::KaxFileName, to_wide(QFileInfo{fileName}.fileName()),
                                                   new libmatroska::KaxMimeType, to_utf8(mimeType),
                                                   new libmatroska::KaxFileUID,  uid)
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
  if (!modifiedPage && !m_tracksReordered)
    return true;

  auto tool = MainWindow::headerEditorTool();
  MainWindow::get()->switchToTool(tool);
  tool->showTab(*this);

  if (modifiedPage)
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

void
Tab::updateTracksElementToMatchTrackOrder() {
  auto &tracks = static_cast<libebml::EbmlMaster &>(*m_eTracks);

  remove_children<libmatroska::KaxTrackEntry>(tracks);

  for (auto page : m_model->topLevelPages())
    if (dynamic_cast<TrackTypePage *>(page))
      tracks.PushElement(static_cast<TrackTypePage &>(*page).m_master);
}

void
Tab::walkPagesOfSelectedTopLevelNode(std::function<bool(PageBase *)> worker) {
  auto topLevelIdx = ui->elements->selectionModel()->currentIndex();

  if (!topLevelIdx.isValid())
    return;

  topLevelIdx = topLevelIdx.sibling(topLevelIdx.row(), 0);

  while (topLevelIdx.parent().isValid())
    topLevelIdx = topLevelIdx.parent();

  std::function<void(QModelIndex const &)> walkTree;
  walkTree = [this, &worker, &walkTree](QModelIndex const &parentIdx) {
    for (auto row = 0, numRows = m_model->rowCount(parentIdx); row < numRows; ++row) {
      auto idx  = m_model->index(row, 0, parentIdx);

      if (!worker(m_model->selectedPage(idx)))
        return;

      walkTree(idx);
    }
  };

  walkTree(topLevelIdx);
}

void
Tab::toggleSpecificTrackFlag(unsigned int wantedId) {
  walkPagesOfSelectedTopLevelNode([wantedId](auto *pageBase) -> bool {
    auto page = dynamic_cast<BoolValuePage *>(pageBase);

    if (page && (page->m_callbacks.ClassId().GetValue() == wantedId)) {
      page->toggleFlag();
      return false;
    }

    return true;
  });

  updateSelectedTopLevelPageModelItems();
}

void
Tab::toggleTrackFlag() {
  auto action = dynamic_cast<QAction *>(sender());

  if (action)
    toggleSpecificTrackFlag(action->data().toUInt());
}

void
Tab::changeTrackLanguage(QString const &formattedLanguage,
                         QString const &trackName) {
  auto language = mtx::bcp47::language_c::parse(to_utf8(formattedLanguage));
  if (!language.is_valid())
    return;

  auto languageFound  = false,
    languageIETFFound = false;

  walkPagesOfSelectedTopLevelNode([&language, &languageFound, &languageIETFFound](auto *pageBase) -> bool {
    auto languagePage = dynamic_cast<LanguageValuePage *>(pageBase);

    if (languagePage) {
      languagePage->setLanguage(language);
      languageFound = true;
    }

    auto languageIETFPage = dynamic_cast<LanguageIETFValuePage *>(pageBase);

    if (languageIETFPage) {
      languageIETFPage->setLanguage(language);
      languageIETFFound = true;
    }

    return !languageFound || !languageIETFFound;
  });

  if (trackName.isEmpty()) {
    updateSelectedTopLevelPageModelItems();
    return;
  }

  walkPagesOfSelectedTopLevelNode([&trackName](auto *pageBase) -> bool {
    auto trackNamePage = dynamic_cast<TrackNamePage *>(pageBase);

    if (!trackNamePage)
      return true;

    trackNamePage->setString(trackName);

    return false;
  });

  updateSelectedTopLevelPageModelItems();
}

void
Tab::moveElementUpOrDown(bool up) {
  auto focus        = App::instance()->focusWidget();
  auto selectedIdx  = ui->elements->selectionModel()->currentIndex();
  auto selectedPage = m_model->selectedPage(selectedIdx);

  if (!selectedIdx.isValid() || !selectedPage)
    return;

  auto idxToMove = m_model->trackOrAttachedFileIndexForSelectedIndex(selectedIdx);
  if (!idxToMove.isValid())
    return;

  auto wasExpanded = ui->elements->isExpanded(selectedIdx);

  m_model->moveElementUpOrDown(idxToMove, up);

  auto newIdx = m_model->indexFromPage(selectedPage);

  ui->elements->setExpanded(newIdx, wasExpanded);

  auto expandIdx = newIdx.parent();

  while (expandIdx.isValid()) {
    ui->elements->setExpanded(expandIdx, true);
    expandIdx = expandIdx.parent();
  }

  auto selection = QItemSelection{newIdx, newIdx.sibling(newIdx.row(), m_model->columnCount() - 1)};
  ui->elements->selectionModel()->setCurrentIndex(newIdx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Current);
  ui->elements->selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Current);

  if (focus)
    focus->setFocus();
}

void
Tab::updateSelectedTopLevelPageModelItems() {
  auto topLevelIdx = ui->elements->selectionModel()->currentIndex();

  if (!topLevelIdx.isValid())
    return;

  while (topLevelIdx.parent().isValid())
    topLevelIdx = topLevelIdx.parent();

  auto page = m_model->selectedPage(topLevelIdx);
  if (dynamic_cast<TrackTypePage *>(page))
    static_cast<TrackTypePage *>(page)->updateModelItems();
}

}
