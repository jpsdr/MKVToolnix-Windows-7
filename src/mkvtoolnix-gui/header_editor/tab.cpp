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
#include "common/mm_io_x.h"
#include "common/qt.h"
#include "common/segmentinfo.h"
#include "common/segment_tracks.h"
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
  auto selectedIdx         = ui->elements->selectionModel()->currentIndex();
  selectedIdx              = selectedIdx.isValid()          ? selectedIdx.sibling(selectedIdx.row(), 0) : selectedIdx;
  auto selectedTopLevelRow = !selectedIdx.isValid()         ? -1
                           : selectedIdx.parent().isValid() ? selectedIdx.parent().row()
                           :                                  selectedIdx.row();
  auto selected2ndLevelRow = !selectedIdx.isValid()         ? -1
                           : selectedIdx.parent().isValid() ? selectedIdx.row()
                           :                                  -1;
  auto expansionStatus     = QHash<QString, bool>{};

  for (auto const &page : m_model->topLevelPages()) {
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

  if (!m_analyzer->set_parse_mode(kax_analyzer_c::parse_mode_fast).set_open_mode(MODE_READ).process()) {
    auto text = Q("%1 %2")
      .arg(QY("The file you tried to open (%1) could not be read successfully.").arg(m_fileName))
      .arg(QY("Possible reasons are: the file is not a Matroska file; the file is write-protected; the file is locked by another process; you do not have permission to access the file."));
    Util::MessageBox::critical(this)->title(QY("File parsing failed")).text(text).exec();
    emit removeThisTab();
    return;
  }

  m_fileModificationTime = QFileInfo{m_fileName}.lastModified();

  populateTree();

  m_analyzer->close_file();

  for (auto const &page : m_model->topLevelPages()) {
    auto key = dynamic_cast<TopLevelPage &>(*page).internalIdentifier();
    ui->elements->setExpanded(page->m_pageIdx, expansionStatus[key]);
  }

  Util::resizeViewColumnsToContents(ui->elements);

  if (!selectedIdx.isValid() || (-1 == selectedTopLevelRow))
    return;

  selectedIdx = m_model->index(selectedTopLevelRow, 0);
  if (-1 != selected2ndLevelRow)
    selectedIdx = m_model->index(selected2ndLevelRow, 0, selectedIdx);

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

  if (QFileInfo{m_fileName}.lastModified() != m_fileModificationTime) {
    Util::MessageBox::critical(this)
      ->title(QY("File has been modified"))
      .text(QY("The file has been changed by another program since it was read by the header editor. Therefore you have to re-load it. Unfortunately this means that all of your changes will be lost."))
      .exec();
    return;
  }

  doModifications();

  if (segmentinfoModified && m_eSegmentInfo) {
    auto result = m_analyzer->update_element(m_eSegmentInfo, true);
    if (kax_analyzer_c::uer_success != result)
      QtKaxAnalyzer::displayUpdateElementResult(this, result, QY("Saving the modified segment information header failed."));
  }

  if (tracksModified && m_eTracks) {
    auto result = m_analyzer->update_element(m_eTracks, true);
    if (kax_analyzer_c::uer_success != result)
      QtKaxAnalyzer::displayUpdateElementResult(this, result, QY("Saving the modified track headers failed."));
  }

  if (attachmentsModified) {
    auto attachments = std::make_shared<KaxAttachments>();

    for (auto const &attachedFilePage : m_attachmentsPage->m_children)
      attachments->PushElement(*dynamic_cast<AttachedFilePage &>(*attachedFilePage).m_attachment.get());

    auto result = attachments->ListSize() ? m_analyzer->update_element(attachments.get(), true)
                :                           m_analyzer->remove_elements(KaxAttachments::ClassInfos.GlobalId);

    attachments->RemoveAll();

    if (kax_analyzer_c::uer_success != result)
      QtKaxAnalyzer::displayUpdateElementResult(this, result, QY("Saving the modified attachments failed."));
  }

  m_analyzer->close_file();

  load();

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
  m_removeAttachmentAction->setText(QY("&Remove selected attachment"));
  m_saveAttachmentContentAction->setText(QY("&Save attachment content to a file"));
  m_replaceAttachmentContentAction->setText(QY("Re&place attachment with a new file"));
  m_replaceAttachmentContentSetValuesAction->setText(QY("Replace attachment with a new file and &derive name && MIME type from it"));

  m_addAttachmentsAction->setIcon(QIcon{Q(":/icons/16x16/list-add.png")});
  m_removeAttachmentAction->setIcon(QIcon{Q(":/icons/16x16/list-remove.png")});
  m_saveAttachmentContentAction->setIcon(QIcon{Q(":/icons/16x16/document-save.png")});
  m_replaceAttachmentContentAction->setIcon(QIcon{Q(":/icons/16x16/document-open.png")});

  for (auto const &page : m_model->pages())
    page->retranslateUi();

  m_model->retranslateUi();

  Util::resizeViewColumnsToContents(ui->elements);
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

bool
Tab::hasBeenModified() {
  auto &pages = m_model->topLevelPages();
  for (auto const &page : pages)
    if (page->hasBeenModified())
      return true;

  return false;
}

void
Tab::doModifications() {
  auto &pages = m_model->topLevelPages();
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
Tab::handleSegmentInfo(kax_analyzer_data_c const &data) {
  m_eSegmentInfo = m_analyzer->read_element(data);
  if (!m_eSegmentInfo)
    return;

  auto &info = dynamic_cast<KaxInfo &>(*m_eSegmentInfo.get());
  auto page  = new TopLevelPage{*this, YT("Segment information")};
  page->setInternalIdentifier("segmentInfo");
  page->init();

  (new StringValuePage{*this, *page, info, KaxTitle::ClassInfos,           YT("Title"),                        YT("The title for the whole movie.")})->init();
  (new StringValuePage{*this, *page, info, KaxSegmentFilename::ClassInfos, YT("Segment file name"),            YT("The file name for this segment.")})->init();
  (new StringValuePage{*this, *page, info, KaxPrevFilename::ClassInfos,    YT("Previous file name"),           YT("An escaped file name corresponding to the previous segment.")})->init();
  (new StringValuePage{*this, *page, info, KaxNextFilename::ClassInfos,    YT("Next filename"),                YT("An escaped file name corresponding to the next segment.")})->init();
  (new BitValuePage{   *this, *page, info, KaxSegmentUID::ClassInfos,      YT("Segment unique ID"),            YT("A randomly generated unique ID to identify the current segment between many others (128 bits)."), 128})->init();
  (new BitValuePage{   *this, *page, info, KaxPrevUID::ClassInfos,         YT("Previous segment's unique ID"), YT("A unique ID to identify the previous chained segment (128 bits)."), 128})->init();
  (new BitValuePage{   *this, *page, info, KaxNextUID::ClassInfos,         YT("Next segment's unique ID"),     YT("A unique ID to identify the next chained segment (128 bits)."), 128})->init();

  m_segmentinfoPage = page;
}

void
Tab::handleTracks(kax_analyzer_data_c const &data) {
  m_eTracks = m_analyzer->read_element(data);
  if (!m_eTracks)
    return;

  auto trackIdxMkvmerge = 0u;

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

    (new UnsignedIntegerValuePage{*this, *page, *kTrackEntry, KaxTrackNumber::ClassInfos, YT("Track number"), YT("The track number as used in the Block Header.")})
      ->init();

    (new UnsignedIntegerValuePage{*this, *page, *kTrackEntry, KaxTrackUID::ClassInfos, YT("Track UID"), YT("A unique ID to identify the Track. This should be kept the same when making a direct stream copy "
                                                                                                           "of the Track to another file.")})
      ->init();

    (new BoolValuePage{*this, *page, *kTrackEntry, KaxTrackFlagDefault::ClassInfos, YT("'Default track' flag"), YT("Set if that track (audio, video or subs) SHOULD be used if no language found matches the user preference.")})
      ->init();

    (new BoolValuePage{*this, *page, *kTrackEntry, KaxTrackFlagEnabled::ClassInfos, YT("'Track enabled' flag"), YT("Set if the track is used.")})
      ->init();

    (new BoolValuePage{*this, *page, *kTrackEntry, KaxTrackFlagForced::ClassInfos, YT("'Forced display' flag"),
                       YT("Set if that track MUST be used during playback. "
                          "There can be many forced track for a kind (audio, video or subs). "
                          "The player should select the one whose language matches the user preference or the default + forced track.")})
      ->init();

    (new UnsignedIntegerValuePage{*this, *page, *kTrackEntry, KaxTrackMinCache::ClassInfos, YT("Minimum cache"),
                                  YT("The minimum number of frames a player should be able to cache during playback. "
                                     "If set to 0, the reference pseudo-cache system is not used.")})
      ->init();

    (new UnsignedIntegerValuePage{*this, *page, *kTrackEntry, KaxTrackMaxCache::ClassInfos, YT("Maximum cache"),
                                  YT("The maximum number of frames a player should be able to cache during playback. "
                                     "If set to 0, the reference pseudo-cache system is not used.")})
      ->init();

    (new UnsignedIntegerValuePage{*this, *page, *kTrackEntry, KaxTrackDefaultDuration::ClassInfos, YT("Default duration"), YT("Number of nanoseconds (not scaled) per frame.")})
      ->init();

    (new StringValuePage{*this, *page, *kTrackEntry, KaxTrackName::ClassInfos, YT("Name"), YT("A human-readable track name.")})
      ->init();

    (new LanguageValuePage{*this, *page, *kTrackEntry, KaxTrackLanguage::ClassInfos, YT("Language"), YT("Specifies the language of the track in the Matroska languages form.")})
      ->init();

    (new AsciiStringValuePage{*this, *page, *kTrackEntry, KaxCodecID::ClassInfos, YT("Codec ID"), YT("An ID corresponding to the codec.")})
      ->init();

    (new StringValuePage{*this, *page, *kTrackEntry, KaxCodecName::ClassInfos, YT("Codec name"), YT("A human-readable string specifying the codec.")})
      ->init();

    (new UnsignedIntegerValuePage{*this, *page, *kTrackEntry, KaxCodecDelay::ClassInfos, YT("Codec-inherent delay"), YT("Delay built into the codec during decoding in ns.")})
      ->init();

    if (track_video == trackType) {
      auto &kTrackVideo = GetChild<KaxTrackVideo>(kTrackEntry);

      (new UnsignedIntegerValuePage{*this, *page, kTrackVideo, KaxVideoPixelWidth::ClassInfos, YT("Video pixel width"), YT("Width of the encoded video frames in pixels.")})
        ->init();

      (new UnsignedIntegerValuePage{*this, *page, kTrackVideo, KaxVideoPixelHeight::ClassInfos, YT("Video pixel height"), YT("Height of the encoded video frames in pixels.")})
        ->init();

      (new UnsignedIntegerValuePage{*this, *page, kTrackVideo, KaxVideoDisplayWidth::ClassInfos, YT("Video display width"), YT("Width of the video frames to display.")})
        ->init();

      (new UnsignedIntegerValuePage{*this, *page, kTrackVideo, KaxVideoDisplayHeight::ClassInfos, YT("Video display height"), YT("Height of the video frames to display.")})
        ->init();

      (new UnsignedIntegerValuePage{*this, *page, kTrackVideo, KaxVideoDisplayUnit::ClassInfos, YT("Video display unit"), YT("Type of the unit for DisplayWidth/Height (0: pixels, 1: centimeters, 2: inches, 3: aspect ratio).")})
        ->init();

      (new UnsignedIntegerValuePage{*this, *page, kTrackVideo, KaxVideoPixelCropLeft::ClassInfos, YT("Video crop left"), YT("The number of video pixels to remove on the left of the image.")})
        ->init();

      (new UnsignedIntegerValuePage{*this, *page, kTrackVideo, KaxVideoPixelCropTop::ClassInfos, YT("Video crop top"), YT("The number of video pixels to remove on the top of the image.")})
        ->init();

      (new UnsignedIntegerValuePage{*this, *page, kTrackVideo, KaxVideoPixelCropRight::ClassInfos, YT("Video crop right"), YT("The number of video pixels to remove on the right of the image.")})
        ->init();

      (new UnsignedIntegerValuePage{*this, *page, kTrackVideo, KaxVideoPixelCropBottom::ClassInfos, YT("Video crop bottom"), YT("The number of video pixels to remove on the bottom of the image.")})
        ->init();

      (new UnsignedIntegerValuePage{*this, *page, kTrackVideo, KaxVideoAspectRatio::ClassInfos, YT("Video aspect ratio type"), YT("Specify the possible modifications to the aspect ratio "
                                                                                                                                   "(0: free resizing, 1: keep aspect ratio, 2: fixed).")})
        ->init();

      (new UnsignedIntegerValuePage{*this, *page, kTrackVideo, KaxVideoStereoMode::ClassInfos, YT("Video stereo mode"), YT("Stereo-3D video mode (0 - 11, see documentation).")})
        ->init();

    } else if (track_audio == trackType) {
      auto &kTrackAudio = GetChild<KaxTrackAudio>(kTrackEntry);

      (new FloatValuePage{*this, *page, kTrackAudio, KaxAudioSamplingFreq::ClassInfos, YT("Audio sampling frequency"), YT("Sampling frequency in Hz.")})
        ->init();

      (new FloatValuePage{*this, *page, kTrackAudio, KaxAudioOutputSamplingFreq::ClassInfos, YT("Audio output sampling frequency"), YT("Real output sampling frequency in Hz.")})
        ->init();

      (new UnsignedIntegerValuePage{*this, *page, kTrackAudio, KaxAudioChannels::ClassInfos, YT("Audio channels"), YT("Numbers of channels in the track.")})
        ->init();

      (new UnsignedIntegerValuePage{*this, *page, kTrackAudio, KaxAudioBitDepth::ClassInfos, YT("Audio bit depth"), YT("Bits per sample, mostly used for PCM.")})
        ->init();
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
Tab::expandCollapseAll(bool expand) {
  for (auto const &page : m_model->topLevelPages())
    ui->elements->setExpanded(page->m_pageIdx, expand);
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

}}}
