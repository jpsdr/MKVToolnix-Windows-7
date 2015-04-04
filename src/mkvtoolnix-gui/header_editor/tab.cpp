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
#include "mkvtoolnix-gui/header_editor/ascii_string_value_page.h"
#include "mkvtoolnix-gui/header_editor/bit_value_page.h"
#include "mkvtoolnix-gui/header_editor/bool_value_page.h"
#include "mkvtoolnix-gui/header_editor/float_value_page.h"
#include "mkvtoolnix-gui/header_editor/language_value_page.h"
#include "mkvtoolnix-gui/header_editor/page_model.h"
#include "mkvtoolnix-gui/header_editor/string_value_page.h"
#include "mkvtoolnix-gui/header_editor/tab.h"
#include "mkvtoolnix-gui/header_editor/top_level_page.h"
#include "mkvtoolnix-gui/header_editor/track_type_page.h"
#include "mkvtoolnix-gui/header_editor/unsigned_integer_value_page.h"

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

  connect(ui->elements->selectionModel(), &QItemSelectionModel::currentChanged, this, &Tab::selectionChanged);
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
Tab::selectionChanged(QModelIndex const &current,
                      QModelIndex const &) {
  auto selectedPage = m_model->selectedPage(current);
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
  auto page  = new TopLevelPage{*this, YT("Segment information")};
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

  auto trackIdxMkvmerge = 0u;

  for (auto const &element : static_cast<EbmlMaster &>(*m_eTracks)) {
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
Tab::validate() {
  auto pageIdx = m_model->validate();

  if (!pageIdx.isValid()) {
    QMessageBox::information(this, QY("Header validation"), QY("All header values are OK."));
    return;
  }

  ui->elements->selectionModel()->select(pageIdx, QItemSelectionModel::ClearAndSelect);
  selectionChanged(pageIdx, QModelIndex{});

  QMessageBox::warning(this, QY("Header validation"), QY("There were errors in the header values preventing the headers from being saved. The first error has been selected."));
}

}}}
