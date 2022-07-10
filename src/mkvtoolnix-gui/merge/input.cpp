#include "common/common_pch.h"

#include <QDebug>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileInfo>
#include <QList>
#include <QMenu>
#include <QMessageBox>
#include <QProcess>
#include <QRegularExpression>
#include <QSettings>
#include <QSignalBlocker>
#include <QString>
#include <QThread>
#include <QTimer>

#include <matroska/KaxSemantic.h>

#include "common/iso639.h"
#include "common/logger.h"
#include "common/qt.h"
#include "common/stereo_mode.h"
#include "mkvtoolnix-gui/app.h"
#include "mkvtoolnix-gui/forms/merge/tab.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/main_window/select_character_set_dialog.h"
#include "mkvtoolnix-gui/merge/enums.h"
#include "mkvtoolnix-gui/merge/executable_location_dialog.h"
#include "mkvtoolnix-gui/merge/tab.h"
#include "mkvtoolnix-gui/merge/tab_p.h"
#include "mkvtoolnix-gui/merge/tool.h"
#include "mkvtoolnix-gui/util/file.h"
#include "mkvtoolnix-gui/util/file_identifier.h"
#include "mkvtoolnix-gui/util/file_dialog.h"
#include "mkvtoolnix-gui/util/file_type_filter.h"
#include "mkvtoolnix-gui/util/header_view_manager.h"
#include "mkvtoolnix-gui/util/message_box.h"
#include "mkvtoolnix-gui/util/model.h"
#include "mkvtoolnix-gui/util/settings.h"
#include "mkvtoolnix-gui/util/widget.h"

namespace mtx::gui::Merge {

using namespace mtx::gui;

void
Tab::setupControlLists() {
  auto &p = *p_func();

  p.typeIndependentControls << p.ui->generalOptionsBox << p.ui->muxThisLabel << p.ui->muxThis << p.ui->miscellaneousBox << p.ui->additionalTrackOptionsLabel << p.ui->additionalTrackOptions;

  p.audioControls << p.ui->trackNameLabel << p.ui->trackName << p.ui->trackLanguageLabel << p.ui->trackLanguage << p.ui->defaultTrackFlagLabel << p.ui->defaultTrackFlag << p.ui->forcedTrackFlagLabel << p.ui->forcedTrackFlag
                  << p.ui->trackEnabledFlagLabel << p.ui->trackEnabledFlag << p.ui->hearingImpairedFlagLabel << p.ui->hearingImpairedFlag << p.ui->visualImpairedFlagLabel << p.ui->visualImpairedFlag
                  << p.ui->originalFlagLabel << p.ui->originalFlag << p.ui->commentaryFlagLabel << p.ui->commentaryFlag
                  << p.ui->compressionLabel << p.ui->compression << p.ui->trackTagsLabel << p.ui->trackTags << p.ui->browseTrackTags << p.ui->timestampsAndDefaultDurationBox
                  << p.ui->delayLabel << p.ui->delay << p.ui->stretchByLabel << p.ui->stretchBy << p.ui->timestampsLabel << p.ui->timestamps << p.ui->browseTimestamps << p.ui->audioPropertiesBox << p.ui->aacIsSBR << p.ui->aacIsSBRLabel
                  << p.ui->cuesLabel << p.ui->cues << p.ui->propertiesLabel << p.ui->generalOptionsBox << p.ui->reduceToAudioCore << p.ui->removeDialogNormalizationGain;

  p.videoControls << p.ui->trackNameLabel << p.ui->trackName << p.ui->trackLanguageLabel << p.ui->trackLanguage << p.ui->defaultTrackFlagLabel << p.ui->defaultTrackFlag << p.ui->forcedTrackFlagLabel << p.ui->forcedTrackFlag
                  << p.ui->trackEnabledFlagLabel << p.ui->trackEnabledFlag << p.ui->hearingImpairedFlagLabel << p.ui->hearingImpairedFlag << p.ui->visualImpairedFlagLabel << p.ui->visualImpairedFlag
                  << p.ui->originalFlagLabel << p.ui->originalFlag << p.ui->commentaryFlagLabel << p.ui->commentaryFlag
                  << p.ui->compressionLabel << p.ui->compression << p.ui->trackTagsLabel << p.ui->trackTags << p.ui->browseTrackTags << p.ui->timestampsAndDefaultDurationBox
                  << p.ui->delayLabel << p.ui->delay << p.ui->stretchByLabel << p.ui->stretchBy << p.ui->defaultDurationLabel << p.ui->defaultDuration << p.ui->timestampsLabel << p.ui->timestamps << p.ui->browseTimestamps
                  << p.ui->videoPropertiesBox << p.ui->setAspectRatio << p.ui->aspectRatio << p.ui->setDisplayWidthHeight << p.ui->displayWidth << p.ui->displayDimensionsXLabel << p.ui->displayHeight << p.ui->stereoscopyLabel
                  << p.ui->stereoscopy << p.ui->croppingLabel << p.ui->cropping << p.ui->cuesLabel << p.ui->cues
                  << p.ui->propertiesLabel << p.ui->generalOptionsBox << p.ui->fixBitstreamTimingInfo

                  << p.ui->colorInformationBox
                  << p.ui->colorMatrixCoefficientsLabel << p.ui->colorMatrixCoefficients << p.ui->bitsPerColorChannelLabel    << p.ui->bitsPerColorChannel    << p.ui->chromaSubsamplingLabel   << p.ui->chromaSubsampling
                  << p.ui->cbSubsamplingLabel           << p.ui->cbSubsampling           << p.ui->chromaSitingLabel           << p.ui->chromaSiting           << p.ui->colorRangeLabel          << p.ui->colorRange
                  << p.ui->transferCharacteristicsLabel << p.ui->transferCharacteristics << p.ui->colorPrimariesLabel         << p.ui->colorPrimaries         << p.ui->maximumContentLightLabel << p.ui->maximumContentLight
                  << p.ui->maximumFrameLightLabel       << p.ui->maximumFrameLight

                  << p.ui->colorMasteringMetaInformationBox
                  << p.ui->chromaticityCoordinatesLabel << p.ui->chromaticityCoordinates << p.ui->whiteColorCoordinatesLabel  << p.ui->whiteColorCoordinates
                  << p.ui->maximumLuminanceLabel        << p.ui->maximumLuminance        << p.ui->minimumLuminanceLabel       << p.ui->minimumLuminance

                  << p.ui->videoProjectionInformationBox
                  << p.ui->projectionTypeLabel          << p.ui->projectionType          << p.ui->projectionSpecificDataLabel << p.ui->projectionSpecificData << p.ui->yawRotationLabel         << p.ui->yawRotation
                  << p.ui->pitchRotationLabel           << p.ui->pitchRotation           << p.ui->rollRotationLabel           << p.ui->rollRotation
    ;

  p.subtitleControls << p.ui->trackNameLabel << p.ui->trackName << p.ui->trackLanguageLabel << p.ui->trackLanguage << p.ui->defaultTrackFlagLabel << p.ui->defaultTrackFlag << p.ui->forcedTrackFlagLabel << p.ui->forcedTrackFlag
                     << p.ui->trackEnabledFlagLabel << p.ui->trackEnabledFlag << p.ui->hearingImpairedFlagLabel << p.ui->hearingImpairedFlag << p.ui->visualImpairedFlagLabel << p.ui->visualImpairedFlag << p.ui->textDescriptionsFlagLabel << p.ui->textDescriptionsFlag
                     << p.ui->originalFlagLabel << p.ui->originalFlag << p.ui->commentaryFlagLabel << p.ui->commentaryFlag
                     << p.ui->compressionLabel << p.ui->compression << p.ui->trackTagsLabel << p.ui->trackTags << p.ui->browseTrackTags << p.ui->timestampsAndDefaultDurationBox
                     << p.ui->delayLabel << p.ui->delay << p.ui->stretchByLabel << p.ui->stretchBy << p.ui->timestampsLabel << p.ui->timestamps << p.ui->browseTimestamps
                     << p.ui->subtitleAndChapterPropertiesBox << p.ui->characterSetLabel << p.ui->subtitleCharacterSet << p.ui->cuesLabel << p.ui->cues
                     << p.ui->propertiesLabel << p.ui->generalOptionsBox;

  p.chapterControls << p.ui->timestampsAndDefaultDurationBox << p.ui->delayLabel << p.ui->delay << p.ui->stretchByLabel << p.ui->stretchBy
                    << p.ui->subtitleAndChapterPropertiesBox << p.ui->characterSetLabel << p.ui->subtitleCharacterSet << p.ui->propertiesLabel << p.ui->generalOptionsBox;

  p.allInputControls << p.ui->muxThisLabel << p.ui->muxThis << p.ui->trackNameLabel << p.ui->trackName << p.ui->trackLanguageLabel << p.ui->trackLanguage << p.ui->defaultTrackFlagLabel << p.ui->defaultTrackFlag
                     << p.ui->forcedTrackFlagLabel << p.ui->forcedTrackFlag << p.ui->trackEnabledFlagLabel << p.ui->trackEnabledFlag
                     << p.ui->hearingImpairedFlagLabel << p.ui->hearingImpairedFlag << p.ui->visualImpairedFlagLabel << p.ui->visualImpairedFlag << p.ui->textDescriptionsFlagLabel << p.ui->textDescriptionsFlag
                     << p.ui->originalFlagLabel << p.ui->originalFlag << p.ui->commentaryFlagLabel << p.ui->commentaryFlag
                     << p.ui->compressionLabel << p.ui->compression << p.ui->trackTagsLabel << p.ui->trackTags << p.ui->browseTrackTags << p.ui->timestampsAndDefaultDurationBox
                     << p.ui->delayLabel << p.ui->delay << p.ui->stretchByLabel << p.ui->stretchBy << p.ui->defaultDurationLabel << p.ui->defaultDuration << p.ui->timestampsLabel << p.ui->timestamps << p.ui->browseTimestamps
                     << p.ui->videoPropertiesBox << p.ui->setAspectRatio << p.ui->aspectRatio << p.ui->setDisplayWidthHeight << p.ui->displayWidth << p.ui->displayDimensionsXLabel << p.ui->displayHeight << p.ui->stereoscopyLabel
                     << p.ui->stereoscopy << p.ui->croppingLabel << p.ui->cropping << p.ui->audioPropertiesBox << p.ui->aacIsSBR << p.ui->subtitleAndChapterPropertiesBox << p.ui->characterSetLabel << p.ui->subtitleCharacterSet
                     << p.ui->miscellaneousBox << p.ui->cuesLabel << p.ui->cues << p.ui->additionalTrackOptionsLabel << p.ui->additionalTrackOptions
                     << p.ui->propertiesLabel << p.ui->generalOptionsBox << p.ui->fixBitstreamTimingInfo << p.ui->reduceToAudioCore << p.ui->removeDialogNormalizationGain

                     << p.ui->colorInformationBox
                     << p.ui->colorMatrixCoefficientsLabel << p.ui->colorMatrixCoefficients << p.ui->bitsPerColorChannelLabel    << p.ui->bitsPerColorChannel    << p.ui->chromaSubsamplingLabel   << p.ui->chromaSubsampling
                     << p.ui->cbSubsamplingLabel           << p.ui->cbSubsampling           << p.ui->chromaSitingLabel           << p.ui->chromaSiting           << p.ui->colorRangeLabel          << p.ui->colorRange
                     << p.ui->transferCharacteristicsLabel << p.ui->transferCharacteristics << p.ui->colorPrimariesLabel         << p.ui->colorPrimaries         << p.ui->maximumContentLightLabel << p.ui->maximumContentLight
                     << p.ui->maximumFrameLightLabel       << p.ui->maximumFrameLight

                     << p.ui->colorMasteringMetaInformationBox
                     << p.ui->chromaticityCoordinatesLabel << p.ui->chromaticityCoordinates << p.ui->whiteColorCoordinatesLabel  << p.ui->whiteColorCoordinates
                     << p.ui->maximumLuminanceLabel        << p.ui->maximumLuminance        << p.ui->minimumLuminanceLabel       << p.ui->minimumLuminance

                     << p.ui->videoProjectionInformationBox
                     << p.ui->projectionTypeLabel          << p.ui->projectionType          << p.ui->projectionSpecificDataLabel << p.ui->projectionSpecificData << p.ui->yawRotationLabel         << p.ui->yawRotation
                     << p.ui->pitchRotationLabel           << p.ui->pitchRotation           << p.ui->rollRotationLabel           << p.ui->rollRotation
    ;

  p.comboBoxControls << p.ui->muxThis << p.ui->defaultTrackFlag << p.ui->forcedTrackFlag << p.ui->trackEnabledFlag << p.ui->compression << p.ui->cues << p.ui->stereoscopy << p.ui->aacIsSBR << p.ui->subtitleCharacterSet
                     << p.ui->hearingImpairedFlag << p.ui->visualImpairedFlag << p.ui->textDescriptionsFlag<< p.ui->originalFlag << p.ui->commentaryFlag;

  p.notIfAppendingControls << p.ui->trackLanguageLabel       << p.ui->trackLanguage           << p.ui->trackNameLabel              << p.ui->trackName          << p.ui->defaultTrackFlagLabel     << p.ui->defaultTrackFlag
                           << p.ui->trackEnabledFlagLabel    << p.ui->trackEnabledFlag
                           << p.ui->forcedTrackFlagLabel     << p.ui->forcedTrackFlag         << p.ui->originalFlagLabel           << p.ui->originalFlag       << p.ui->commentaryFlagLabel       << p.ui->commentaryFlag
                           << p.ui->hearingImpairedFlagLabel << p.ui->hearingImpairedFlag     << p.ui->visualImpairedFlagLabel     << p.ui->visualImpairedFlag << p.ui->textDescriptionsFlagLabel << p.ui->textDescriptionsFlag
                           << p.ui->compressionLabel         << p.ui->compression             << p.ui->trackTagsLabel              << p.ui->trackTags          << p.ui->browseTrackTags
                           << p.ui->defaultDurationLabel     << p.ui->defaultDuration         << p.ui->fixBitstreamTimingInfo      << p.ui->setAspectRatio     << p.ui->setDisplayWidthHeight     << p.ui->aspectRatio
                           << p.ui->displayWidth             << p.ui->displayDimensionsXLabel << p.ui->displayHeight               << p.ui->stereoscopyLabel   << p.ui->stereoscopy
                           << p.ui->croppingLabel            << p.ui->cropping                << p.ui->aacIsSBR
                           << p.ui->cuesLabel                << p.ui->cues                    << p.ui->additionalTrackOptionsLabel << p.ui->additionalTrackOptions

                           << p.ui->colorInformationBox
                           << p.ui->colorMatrixCoefficientsLabel << p.ui->colorMatrixCoefficients << p.ui->bitsPerColorChannelLabel    << p.ui->bitsPerColorChannel    << p.ui->chromaSubsamplingLabel   << p.ui->chromaSubsampling
                           << p.ui->cbSubsamplingLabel           << p.ui->cbSubsampling           << p.ui->chromaSitingLabel           << p.ui->chromaSiting           << p.ui->colorRangeLabel          << p.ui->colorRange
                           << p.ui->transferCharacteristicsLabel << p.ui->transferCharacteristics << p.ui->colorPrimariesLabel         << p.ui->colorPrimaries         << p.ui->maximumContentLightLabel << p.ui->maximumContentLight
                           << p.ui->maximumFrameLightLabel       << p.ui->maximumFrameLight

                           << p.ui->colorMasteringMetaInformationBox
                           << p.ui->chromaticityCoordinatesLabel << p.ui->chromaticityCoordinates << p.ui->whiteColorCoordinatesLabel  << p.ui->whiteColorCoordinates
                           << p.ui->maximumLuminanceLabel        << p.ui->maximumLuminance        << p.ui->minimumLuminanceLabel       << p.ui->minimumLuminance

                           << p.ui->videoProjectionInformationBox
                           << p.ui->projectionTypeLabel          << p.ui->projectionType          << p.ui->projectionSpecificDataLabel << p.ui->projectionSpecificData << p.ui->yawRotationLabel         << p.ui->yawRotation
                           << p.ui->pitchRotationLabel           << p.ui->pitchRotation           << p.ui->rollRotationLabel           << p.ui->rollRotation
    ;
}

void
Tab::setupMoveUpDownButtons() {
  auto &p      = *p_func();
  auto show    = Util::Settings::get().m_showMoveUpDownButtons;
  auto widgets = QList<QWidget *>{} << p.ui->moveFilesButtons << p.ui->moveTracksButtons << p.ui->moveAttachmentsButtons;

  for (auto const &widget : widgets)
    widget->setVisible(show);
}

void
Tab::setupInputLayout() {
  auto const layout = Util::Settings::get().m_mergeTrackPropertiesLayout;

  if (layout == Util::Settings::TrackPropertiesLayout::HorizontalScrollArea)
    setupHorizontalScrollAreaInputLayout();

  else if (layout == Util::Settings::TrackPropertiesLayout::HorizontalTwoColumns)
    setupHorizontalTwoColumnsInputLayout();

  else
    setupVerticalTabWidgetInputLayout();
}

void
Tab::setupHorizontalScrollAreaInputLayout() {
  auto &p = *p_func();

  if (p.ui->wProperties->isVisible() && (0 == p.ui->propertiesStack->currentIndex()))
    return;

  p.ui->twProperties->hide();
  p.ui->wProperties->show();

  auto layout = qobject_cast<QBoxLayout *>(p.ui->scrollAreaWidgetContents->layout());

  Q_ASSERT(!!layout);

  auto widgets = QWidgetList{} << p.ui->generalOptionsBox << p.ui->timestampsAndDefaultDurationBox << p.ui->videoPropertiesBox << p.ui->audioPropertiesBox << p.ui->subtitleAndChapterPropertiesBox << p.ui->miscellaneousBox;
  for (auto const &widget : widgets)
    widget->setParent(p.ui->scrollAreaWidgetContents);

  auto idx = 0;
  layout->insertWidget(idx++, p.ui->generalOptionsBox);
  layout->insertWidget(idx++, p.ui->timestampsAndDefaultDurationBox);
  layout->insertWidget(idx++, p.ui->videoPropertiesBox);
  layout->insertWidget(idx++, p.ui->colorInformationBox);
  layout->insertWidget(idx++, p.ui->colorMasteringMetaInformationBox);
  layout->insertWidget(idx++, p.ui->videoProjectionInformationBox);
  layout->insertWidget(idx++, p.ui->audioPropertiesBox);
  layout->insertWidget(idx++, p.ui->subtitleAndChapterPropertiesBox);
  layout->insertWidget(idx++, p.ui->miscellaneousBox);

  p.ui->propertiesColumn1->updateGeometry();
  p.ui->propertiesColumn2->updateGeometry();

  p.ui->propertiesStack->setCurrentIndex(0);

  Util::autoGroupBoxGridLayout(*p.ui->generalOptionsBox, 1);
}

void
Tab::setupHorizontalTwoColumnsInputLayout() {
  auto &p = *p_func();

  if (p.ui->wProperties->isVisible() && (1 == p.ui->propertiesStack->currentIndex()))
    return;

  p.ui->twProperties->hide();
  p.ui->wProperties->show();
  p.ui->propertiesStack->setCurrentIndex(1);

  auto moveTo = [](QWidget *column, int position, QWidget *widget) {
    auto layout = qobject_cast<QBoxLayout *>(column->layout());
    Q_ASSERT(!!layout);

    widget->setParent(column);
    layout->insertWidget(position, widget);
  };

  auto idx = 0;
  moveTo(p.ui->propertiesColumn1, idx++, p.ui->generalOptionsBox);
  moveTo(p.ui->propertiesColumn1, idx++, p.ui->timestampsAndDefaultDurationBox);
  moveTo(p.ui->propertiesColumn1, idx++, p.ui->audioPropertiesBox);
  moveTo(p.ui->propertiesColumn1, idx++, p.ui->subtitleAndChapterPropertiesBox);
  moveTo(p.ui->propertiesColumn1, idx++, p.ui->miscellaneousBox);

  idx = 0;
  moveTo(p.ui->propertiesColumn2, idx++, p.ui->videoPropertiesBox);
  moveTo(p.ui->propertiesColumn2, idx++, p.ui->colorInformationBox);
  moveTo(p.ui->propertiesColumn2, idx++, p.ui->colorMasteringMetaInformationBox);
  moveTo(p.ui->propertiesColumn2, idx++, p.ui->videoProjectionInformationBox);

  Util::autoGroupBoxGridLayout(*p.ui->generalOptionsBox, 1);
}

void
Tab::setupVerticalTabWidgetInputLayout() {
  auto &p = *p_func();

  if (p.ui->twProperties->isVisible())
    return;

  p.ui->twProperties->show();
  p.ui->wProperties->hide();

  auto moveTo = [](QWidget *page, int position, QWidget *widget) {
    auto layout = qobject_cast<QBoxLayout *>(page->layout());
    Q_ASSERT(!!layout);

    widget->setParent(page);
    layout->insertWidget(position, widget);
  };

  moveTo(p.ui->generalOptionsPage,                 0, p.ui->generalOptionsBox);
  moveTo(p.ui->timestampsAndDefaultDurationPage,   0, p.ui->timestampsAndDefaultDurationBox);
  moveTo(p.ui->videoPropertiesPage,                0, p.ui->videoPropertiesBox);
  moveTo(p.ui->colorInformationPage,              0, p.ui->colorInformationBox);
  moveTo(p.ui->colorMasteringMetaInformationPage, 0, p.ui->colorMasteringMetaInformationBox);
  moveTo(p.ui->videoProjectionInformationPage,     0, p.ui->videoProjectionInformationBox);
  moveTo(p.ui->audioSubtitleChapterPropertiesPage, 0, p.ui->audioPropertiesBox);
  moveTo(p.ui->audioSubtitleChapterPropertiesPage, 1, p.ui->subtitleAndChapterPropertiesBox);
  moveTo(p.ui->miscellaneousPage,                  0, p.ui->miscellaneousBox);

  p.ui->propertiesColumn1->updateGeometry();
  p.ui->propertiesColumn2->updateGeometry();

  Util::autoGroupBoxGridLayout(*p.ui->generalOptionsBox, 2);
}

void
Tab::setupInputControls() {
  auto &p   = *p_func();
  auto &cfg = Util::Settings::get();

  p.ui->twProperties->hide();

  setupControlLists();
  setupMoveUpDownButtons();
  setupInputLayout();

  p.ui->files->setModel(p.filesModel);
  p.ui->tracks->setModel(p.tracksModel);
  p.ui->tracks->enterActivatesAllSelected(true);

  cfg.handleSplitterSizes(p.ui->mergeInputSplitter);
  cfg.handleSplitterSizes(p.ui->mergeFilesTracksSplitter);

  p.ui->trackName->lineEdit()->setClearButtonEnabled(true);
  p.ui->defaultDuration->lineEdit()->setClearButtonEnabled(true);
  p.ui->aspectRatio->lineEdit()->setClearButtonEnabled(true);

  // Track & chapter character set
  p.ui->subtitleCharacterSet->setup(true);
  p.ui->chapterCharacterSet->setup(true);

  // Stereoscopy
  p.ui->stereoscopy->addItem(Q(""), 0);
  for (auto idx = 0u, end = stereo_mode_c::max_index(); idx <= end; ++idx)
    p.ui->stereoscopy->addItem(QString{}, idx + 1);

  p.ui->defaultTrackFlag->addItem(QString{}, true);
  p.ui->defaultTrackFlag->addItem(QString{}, false);

  // Originally the "forced display" flag's options where ordered "off,
  // on"; now they're ordered "yes, no" for consistency with other
  // flags, requiring the values to be reversed.
  p.ui->forcedTrackFlag->addItem(QString{}, 1);
  p.ui->forcedTrackFlag->addItem(QString{}, 0);
  p.ui->forcedTrackFlag->setCurrentIndex(1);

  for (auto &comboBox : std::vector<QComboBox *>{p.ui->muxThis, p.ui->trackEnabledFlag, p.ui->hearingImpairedFlag, p.ui->visualImpairedFlag, p.ui->textDescriptionsFlag, p.ui->originalFlag, p.ui->commentaryFlag})
    for (auto idx = 0; idx < 2; ++idx)
      comboBox->addItem(QString{}, idx == 0);

  for (auto idx = 0; idx < 3; ++idx)
    p.ui->compression->addItem(QString{}, idx);

  for (auto idx = 0; idx < 4; ++idx)
    p.ui->cues->addItem(QString{}, idx);

  for (auto idx = 0; idx < 3; ++idx)
    p.ui->aacIsSBR->addItem(QString{}, idx);

  for (auto const &control : p.comboBoxControls) {
    control->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    Util::fixComboBoxViewWidth(*control);
  }

  // "files" context menu
  p.filesMenu->addAction(p.addFilesAction);
  p.filesMenu->addAction(p.appendFilesAction);
  p.filesMenu->addAction(p.addAdditionalPartsAction);
  p.filesMenu->addSeparator();
  p.filesMenu->addAction(p.removeFilesAction);
  p.filesMenu->addAction(p.removeAllFilesAction);
  p.filesMenu->addSeparator();
  p.filesMenu->addAction(p.setDestinationFileNameAction);
  p.filesMenu->addSeparator();
  p.filesMenu->addAction(p.openFilesInMediaInfoAction);
  p.filesMenu->addSeparator();
  p.filesMenu->addAction(p.selectTracksFromFilesAction);

  p.addFilesAction->setIcon(QIcon::fromTheme(Q("list-add")));
  p.appendFilesAction->setIcon(QIcon::fromTheme(Q("distribute-horizontal-x")));
  p.addAdditionalPartsAction->setIcon(QIcon::fromTheme(Q("distribute-horizontal-margin")));
  p.removeFilesAction->setIcon(QIcon::fromTheme(Q("list-remove")));
  p.openFilesInMediaInfoAction->setIcon(QIcon::fromTheme(Q("documentinfo")));

  // "tracks" context menu
  p.tracksMenu->addAction(p.selectAllTracksAction);
  p.tracksMenu->addMenu(p.selectTracksOfTypeMenu);
  p.tracksMenu->addAction(p.enableAllTracksAction);
  p.tracksMenu->addAction(p.disableAllTracksAction);
  p.tracksMenu->addSeparator();
  p.tracksMenu->addAction(p.openTracksInMediaInfoAction);

  p.selectTracksOfTypeMenu->addAction(p.selectAllVideoTracksAction);
  p.selectTracksOfTypeMenu->addAction(p.selectAllAudioTracksAction);
  p.selectTracksOfTypeMenu->addAction(p.selectAllSubtitlesTracksAction);

  p.selectAllTracksAction->setIcon(QIcon::fromTheme(Q("edit-select-all")));
  p.enableAllTracksAction->setIcon(QIcon::fromTheme(Q("dialog-ok-apply")));
  p.disableAllTracksAction->setIcon(QIcon::fromTheme(Q("dialog-cancel")));
  p.openTracksInMediaInfoAction->setIcon(QIcon::fromTheme(Q("documentinfo")));

  p.selectAllVideoTracksAction->setIcon(QIcon::fromTheme(Q("tool-animator")));
  p.selectAllAudioTracksAction->setIcon(QIcon::fromTheme(Q("audio-headphones")));
  p.selectAllSubtitlesTracksAction->setIcon(QIcon::fromTheme(Q("draw-text")));

  // "add source files" menu
  p.addFilesMenu->addAction(p.addFilesAction2);
  p.addFilesMenu->addAction(p.appendFilesAction2);
  p.addFilesMenu->addAction(p.addAdditionalPartsAction2);
  p.ui->addFiles->setMenu(p.addFilesMenu);

  // "start muxing" menu
  p.startMuxingMenu->addAction(p.startMuxingLeaveAsIs);
  p.startMuxingMenu->addAction(p.startMuxingCreateNewSettings);
  p.startMuxingMenu->addAction(p.startMuxingRemoveInputFiles);
  p.startMuxingMenu->addAction(p.startMuxingCloseSettings);
  p.ui->startMuxing->setMenu(p.startMuxingMenu);

  // "add to job queue" menu
  p.addToJobQueueMenu->addAction(p.addToJobQueueLeaveAsIs);
  p.addToJobQueueMenu->addAction(p.addToJobQueueCreateNewSettings);
  p.addToJobQueueMenu->addAction(p.addToJobQueueRemoveInputFiles);
  p.addToJobQueueMenu->addAction(p.addToJobQueueCloseSettings);
  p.ui->addToJobQueue->setMenu(p.addToJobQueueMenu);

  p.addFilesAction2->setIcon(QIcon::fromTheme(Q("list-add")));
  p.appendFilesAction2->setIcon(QIcon::fromTheme(Q("distribute-horizontal-x")));
  p.addAdditionalPartsAction2->setIcon(QIcon::fromTheme(Q("distribute-horizontal-margin")));

  // Connect Q_SIGNALS & Q_SLOTS.
  auto mw = MainWindow::get();
  using CMSAction = Util::Settings::ClearMergeSettingsAction;

  connect(p.ui->aacIsSBR,                      static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,                       &Tab::onAacIsSBRChanged);
  connect(p.ui->addFiles,                      &QToolButton::clicked,                                                  this,                       &Tab::onAddFiles);
  connect(p.ui->addToJobQueue,                 &QPushButton::clicked,                                                  this,                       [=]() { addToJobQueue(false); });
  connect(p.ui->additionalTrackOptions,        &QLineEdit::textChanged,                                                this,                       &Tab::onAdditionalTrackOptionsChanged);
  connect(p.ui->aspectRatio,                   &QComboBox::currentTextChanged,                                         this,                       &Tab::onAspectRatioChanged);
  connect(p.ui->aspectRatio,                   &QComboBox::editTextChanged,                                            this,                       &Tab::onAspectRatioChanged);
  connect(p.ui->browseTimestamps,              &QPushButton::clicked,                                                  this,                       &Tab::onBrowseTimestamps);
  connect(p.ui->browseTrackTags,               &QPushButton::clicked,                                                  this,                       &Tab::onBrowseTrackTags);
  connect(p.ui->compression,                   static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,                       &Tab::onCompressionChanged);
  connect(p.ui->cropping,                      &QLineEdit::textChanged,                                                this,                       &Tab::onCroppingChanged);
  connect(p.ui->cues,                          static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,                       &Tab::onCuesChanged);
  connect(p.ui->defaultDuration,               &QComboBox::currentTextChanged,                                         this,                       &Tab::onDefaultDurationChanged);
  connect(p.ui->defaultDuration,               &QComboBox::editTextChanged,                                            this,                       &Tab::onDefaultDurationChanged);
  connect(p.ui->defaultTrackFlag,              static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,                       &Tab::onDefaultTrackFlagChanged);
  connect(p.ui->delay,                         &QLineEdit::textChanged,                                                this,                       &Tab::onDelayChanged);
  connect(p.ui->displayHeight,                 &QLineEdit::textChanged,                                                this,                       &Tab::onDisplayHeightChanged);
  connect(p.ui->displayWidth,                  &QLineEdit::textChanged,                                                this,                       &Tab::onDisplayWidthChanged);
  connect(p.ui->editAdditionalOptions,         &QPushButton::clicked,                                                  this,                       &Tab::onEditAdditionalOptions);
  connect(p.ui->files,                         &Util::BasicTreeView::ctrlDownPressed,                                  this,                       &Tab::onMoveFilesDown);
  connect(p.ui->files,                         &Util::BasicTreeView::ctrlUpPressed,                                    this,                       &Tab::onMoveFilesUp);
  connect(p.ui->files,                         &Util::BasicTreeView::customContextMenuRequested,                       this,                       &Tab::showFilesContextMenu);
  connect(p.ui->files,                         &Util::BasicTreeView::deletePressed,                                    this,                       &Tab::onRemoveFiles);
  connect(p.ui->files,                         &Util::BasicTreeView::insertPressed,                                    this,                       &Tab::onAddFiles);
  connect(p.ui->files->selectionModel(),       &QItemSelectionModel::selectionChanged,                                 p.filesModel,               &SourceFileModel::updateSelectionStatus);
  connect(p.ui->files->selectionModel(),       &QItemSelectionModel::selectionChanged,                                 this,                       &Tab::enableMoveFilesButtons);
  connect(p.ui->fixBitstreamTimingInfo,        &QCheckBox::toggled,                                                    this,                       &Tab::onFixBitstreamTimingInfoChanged);
  connect(p.ui->forcedTrackFlag,               static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,                       &Tab::onForcedTrackFlagChanged);
  connect(p.ui->trackEnabledFlag,              static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,                       &Tab::onTrackEnabledFlagChanged);
  connect(p.ui->hearingImpairedFlag,           static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,                       &Tab::onHearingImpairedFlagChanged);
  connect(p.ui->visualImpairedFlag,            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,                       &Tab::onVisualImpairedFlagChanged);
  connect(p.ui->textDescriptionsFlag,          static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,                       &Tab::onTextDescriptionsFlagChanged);
  connect(p.ui->originalFlag,                  static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,                       &Tab::onOriginalFlagChanged);
  connect(p.ui->commentaryFlag,                static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,                       &Tab::onCommentaryFlagChanged);
  connect(p.ui->moveFilesDown,                 &QPushButton::clicked,                                                  this,                       &Tab::onMoveFilesDown);
  connect(p.ui->moveFilesUp,                   &QPushButton::clicked,                                                  this,                       &Tab::onMoveFilesUp);
  connect(p.ui->moveTracksDown,                &QPushButton::clicked,                                                  this,                       &Tab::onMoveTracksDown);
  connect(p.ui->moveTracksUp,                  &QPushButton::clicked,                                                  this,                       &Tab::onMoveTracksUp);
  connect(p.ui->muxThis,                       static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,                       &Tab::onMuxThisChanged);
  connect(p.ui->reduceToAudioCore,             &QCheckBox::toggled,                                                    this,                       &Tab::onReduceAudioToCoreChanged);
  connect(p.ui->removeDialogNormalizationGain, &QCheckBox::toggled,                                                    this,                       &Tab::onRemoveDialogNormalizationGainChanged);
  connect(p.ui->setAspectRatio,                &QPushButton::clicked,                                                  this,                       &Tab::onSetAspectRatio);
  connect(p.ui->setDisplayWidthHeight,         &QPushButton::clicked,                                                  this,                       &Tab::onSetDisplayDimensions);
  connect(p.ui->startMuxing,                   &QPushButton::clicked,                                                  this,                       [=]() { addToJobQueue(true); });
  connect(p.ui->stereoscopy,                   static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,                       &Tab::onStereoscopyChanged);
  connect(p.ui->stretchBy,                     &QLineEdit::textChanged,                                                this,                       &Tab::onStretchByChanged);
  connect(p.ui->subtitleCharacterSet,          static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,                       &Tab::onSubtitleCharacterSetChanged);
  connect(p.ui->subtitleCharacterSetPreview,   &QPushButton::clicked,                                                  this,                       &Tab::onPreviewSubtitleCharacterSet);
  connect(p.ui->timestamps,                    &QLineEdit::textChanged,                                                this,                       &Tab::onTimestampsChanged);
  connect(p.ui->trackLanguage,                 &Util::LanguageDisplayWidget::languageChanged,                          this,                       &Tab::onTrackLanguageChanged);
  connect(p.ui->trackName,                     &QComboBox::editTextChanged,                                            this,                       &Tab::onTrackNameChanged);
  connect(p.ui->trackTags,                     &QLineEdit::textChanged,                                                this,                       &Tab::onTrackTagsChanged);
  connect(p.ui->colorMatrixCoefficients,       &QLineEdit::textChanged,                                                this,                       &Tab::onColorMatrixCoefficientsChanged);
  connect(p.ui->bitsPerColorChannel,           &QLineEdit::textChanged,                                                this,                       &Tab::onBitsPerColorChannelChanged);
  connect(p.ui->chromaSubsampling,             &QLineEdit::textChanged,                                                this,                       &Tab::onChromaSubsamplingChanged);
  connect(p.ui->cbSubsampling,                 &QLineEdit::textChanged,                                                this,                       &Tab::onCbSubsamplingChanged);
  connect(p.ui->chromaSiting,                  &QLineEdit::textChanged,                                                this,                       &Tab::onChromaSitingChanged);
  connect(p.ui->colorRange,                    &QLineEdit::textChanged,                                                this,                       &Tab::onColorRangeChanged);
  connect(p.ui->transferCharacteristics,       &QLineEdit::textChanged,                                                this,                       &Tab::onTransferCharacteristicsChanged);
  connect(p.ui->colorPrimaries,                &QLineEdit::textChanged,                                                this,                       &Tab::onColorPrimariesChanged);
  connect(p.ui->maximumContentLight,           &QLineEdit::textChanged,                                                this,                       &Tab::onMaximumContentLightChanged);
  connect(p.ui->maximumFrameLight,             &QLineEdit::textChanged,                                                this,                       &Tab::onMaximumFrameLightChanged);
  connect(p.ui->chromaticityCoordinates,       &QLineEdit::textChanged,                                                this,                       &Tab::onChromaticityCoordinatesChanged);
  connect(p.ui->whiteColorCoordinates,         &QLineEdit::textChanged,                                                this,                       &Tab::onWhiteColorCoordinatesChanged);
  connect(p.ui->maximumLuminance,              &QLineEdit::textChanged,                                                this,                       &Tab::onMaximumLuminanceChanged);
  connect(p.ui->minimumLuminance,              &QLineEdit::textChanged,                                                this,                       &Tab::onMinimumLuminanceChanged);
  connect(p.ui->projectionType,                &QLineEdit::textChanged,                                                this,                       &Tab::onProjectionTypeChanged);
  connect(p.ui->projectionSpecificData,        &QLineEdit::textChanged,                                                this,                       &Tab::onProjectionSpecificDataChanged);
  connect(p.ui->yawRotation,                   &QLineEdit::textChanged,                                                this,                       &Tab::onYawRotationChanged);
  connect(p.ui->pitchRotation,                 &QLineEdit::textChanged,                                                this,                       &Tab::onPitchRotationChanged);
  connect(p.ui->rollRotation,                  &QLineEdit::textChanged,                                                this,                       &Tab::onRollRotationChanged);
  connect(p.ui->tracks,                        &QTreeView::doubleClicked,                                              this,                       &Tab::toggleMuxThisForSelectedTracks);
  connect(p.ui->tracks,                        &Util::BasicTreeView::allSelectedActivated,                             this,                       &Tab::toggleMuxThisForSelectedTracks);
  connect(p.ui->tracks,                        &Util::BasicTreeView::ctrlDownPressed,                                  this,                       &Tab::onMoveTracksDown);
  connect(p.ui->tracks,                        &Util::BasicTreeView::ctrlUpPressed,                                    this,                       &Tab::onMoveTracksUp);
  connect(p.ui->tracks,                        &Util::BasicTreeView::customContextMenuRequested,                       this,                       &Tab::showTracksContextMenu);
  connect(p.ui->tracks->selectionModel(),      &QItemSelectionModel::selectionChanged,                                 p.tracksModel,              &TrackModel::updateSelectionStatus);
  connect(p.ui->tracks->selectionModel(),      &QItemSelectionModel::selectionChanged,                                 this,                       &Tab::onTrackSelectionChanged);

  connect(p.addFilesAction,                    &QAction::triggered,                                                    this,                       &Tab::onAddFiles);
  connect(p.addFilesAction2,                   &QAction::triggered,                                                    this,                       &Tab::onAddFiles);
  connect(p.appendFilesAction,                 &QAction::triggered,                                                    this,                       &Tab::onAppendFiles);
  connect(p.appendFilesAction2,                &QAction::triggered,                                                    this,                       &Tab::onAppendFiles);
  connect(p.addAdditionalPartsAction,          &QAction::triggered,                                                    this,                       &Tab::onAddAdditionalParts);
  connect(p.addAdditionalPartsAction2,         &QAction::triggered,                                                    this,                       &Tab::onAddAdditionalParts);
  connect(p.removeFilesAction,                 &QAction::triggered,                                                    this,                       &Tab::onRemoveFiles);
  connect(p.removeAllFilesAction,              &QAction::triggered,                                                    this,                       &Tab::onRemoveAllFiles);
  connect(p.setDestinationFileNameAction,      &QAction::triggered,                                                    this,                       &Tab::setDestinationFileNameFromSelectedFile);
  connect(p.openFilesInMediaInfoAction,        &QAction::triggered,                                                    this,                       &Tab::onOpenFilesInMediaInfo);
  connect(p.openTracksInMediaInfoAction,       &QAction::triggered,                                                    this,                       &Tab::onOpenTracksInMediaInfo);

  connect(p.selectAllTracksAction,             &QAction::triggered,                                                    this,                       &Tab::selectAllTracks);
  connect(p.selectAllVideoTracksAction,        &QAction::triggered,                                                    this,                       &Tab::selectAllVideoTracks);
  connect(p.selectAllAudioTracksAction,        &QAction::triggered,                                                    this,                       &Tab::selectAllAudioTracks);
  connect(p.selectAllSubtitlesTracksAction,    &QAction::triggered,                                                    this,                       &Tab::selectAllSubtitlesTracks);
  connect(p.selectTracksFromFilesAction,       &QAction::triggered,                                                    this,                       &Tab::selectAllTracksFromSelectedFiles);
  connect(p.enableAllTracksAction,             &QAction::triggered,                                                    this,                       &Tab::enableAllTracks);
  connect(p.disableAllTracksAction,            &QAction::triggered,                                                    this,                       &Tab::disableAllTracks);

  connect(p.startMuxingLeaveAsIs,              &QAction::triggered,                                                    this,                       [=]() { addToJobQueue(true,  CMSAction::None); });
  connect(p.startMuxingCreateNewSettings,      &QAction::triggered,                                                    this,                       [=]() { addToJobQueue(true,  CMSAction::NewSettings); });
  connect(p.startMuxingCloseSettings,          &QAction::triggered,                                                    this,                       [=]() { addToJobQueue(true,  CMSAction::CloseSettings); });
  connect(p.startMuxingRemoveInputFiles,       &QAction::triggered,                                                    this,                       [=]() { addToJobQueue(true,  CMSAction::RemoveInputFiles); });

  connect(p.addToJobQueueLeaveAsIs,            &QAction::triggered,                                                    this,                       [=]() { addToJobQueue(false, CMSAction::None); });
  connect(p.addToJobQueueCreateNewSettings,    &QAction::triggered,                                                    this,                       [=]() { addToJobQueue(false, CMSAction::NewSettings); });
  connect(p.addToJobQueueCloseSettings,        &QAction::triggered,                                                    this,                       [=]() { addToJobQueue(false, CMSAction::CloseSettings); });
  connect(p.addToJobQueueRemoveInputFiles,     &QAction::triggered,                                                    this,                       [=]() { addToJobQueue(false, CMSAction::RemoveInputFiles); });

  connect(p.addFilesMenu,                      &QMenu::aboutToShow,                                                    this,                       &Tab::enableFilesActions);

  connect(p.filesModel,                        &SourceFileModel::rowsInserted,                                         this,                       &Tab::onFileRowsInserted);
  connect(p.tracksModel,                       &TrackModel::rowsInserted,                                              this,                       &Tab::onTrackRowsInserted);
  connect(p.tracksModel,                       &TrackModel::itemChanged,                                               this,                       &Tab::onTrackItemChanged);

  connect(mw,                                  &MainWindow::preferencesChanged,                                        this,                       &Tab::setupMoveUpDownButtons);
  connect(mw,                                  &MainWindow::preferencesChanged,                                        this,                       &Tab::setupInputLayout);
  connect(mw,                                  &MainWindow::preferencesChanged,                                        this,                       &Tab::setupPredefinedTrackNames);
  connect(mw,                                  &MainWindow::preferencesChanged,                                        p.ui->subtitleCharacterSet, &Util::ComboBoxBase::reInitialize);
  connect(mw,                                  &MainWindow::preferencesChanged,                                        p.ui->chapterCharacterSet,  &Util::ComboBoxBase::reInitialize);

  enableMoveFilesButtons();
  onTrackSelectionChanged();

  Util::HeaderViewManager::create(*p.ui->files,  "Merge::Files") .setDefaultSizes({ { Q("fileName"), 200 }, { Q("container"), 100 }, { Q("fileSize"),  60 } });
  Util::HeaderViewManager::create(*p.ui->tracks, "Merge::Tracks").setDefaultSizes({ { Q("codec"),    150 }, { Q("type"),       80 }, { Q("name"),     150 }, { Q("properties"), 150 } });
}

void
Tab::setupInputToolTips() {
  auto &p = *p_func();

  Util::setToolTip(p.ui->files,     QY("Right-click to add, append and remove files"));
  Util::setToolTip(p.ui->tracks,    QY("Right-click for actions for all items"));

  Util::setToolTip(p.ui->muxThis,   QY("If set to 'no' then the selected tracks will not be copied to the destination file."));
  Util::setToolTip(p.ui->trackName, QY("A name for this track that players can display helping the user choose the right track to play, e.g. \"director's comments\"."));
  Util::setToolTip(p.ui->trackLanguage, QY("The language for this track that players can use for automatic track selection and display for the user."));
  Util::setToolTip(p.ui->defaultTrackFlag,
                   Q("%1 %2")
                   .arg(QY("Make this track eligible to be played by default."))
                   .arg(QY("Players should prefer tracks with the default track flag set while taking into account user preferences such as the track's language.")));
  Util::setToolTip(p.ui->forcedTrackFlag,
                   Q("%1 %2")
                   .arg(QY("Mark this track as \"forced display\"."))
                   .arg(QY("Use this for tracks containing onscreen text or foreign-language dialogue.")));
  Util::setToolTip(p.ui->trackEnabledFlag,
                   Q("%1 %2")
                   .arg(QY("Mark this track as \"enabled\" (the default) or \"disabled\"."))
                   .arg(QY("Players should only consider enabled tracks for playback.")));
  Util::setToolTip(p.ui->hearingImpairedFlag,  QY("Can be set if the track is suitable for users with hearing impairments."));
  Util::setToolTip(p.ui->visualImpairedFlag,   QY("Can be set if the track is suitable for users with visual impairments."));
  Util::setToolTip(p.ui->textDescriptionsFlag, QY("Can be set if the track contains textual descriptions of video content suitable for playback via a text-to-speech system for a visually-impaired user."));
  Util::setToolTip(p.ui->originalFlag,         QY("Can be set if the track is in the content's original language (not a translation)."));
  Util::setToolTip(p.ui->commentaryFlag,       QY("Can be set if the track contains commentary."));
  Util::setToolTip(p.ui->compression,
                   Q("%1 %2 %3")
                   .arg(QY("Sets the lossless compression algorithm to be used for this track."))
                   .arg(QY("If set to 'determine automatically' then mkvmerge will decide whether or not to compress and which algorithm to use based on the track type."))
                   .arg(QY("Currently only certain subtitle formats are compressed with the zlib algorithm.")));
  Util::setToolTip(p.ui->delay,
                   Q("<p>%1 %2 %3</p><p>%4</p>")
                   .arg(QYH("Delay this track's timestamps by a couple of ms."))
                   .arg(QYH("The value can be negative, but keep in mind that any frame whose timestamp is negative after this calculation is dropped."))
                   .arg(QYH("This works with all track types."))
                   .arg(QYH("This option can also be used for chapters.")));
  Util::setToolTip(p.ui->stretchBy,
                   Q("<p>%1 %2</p><p>%3</p><p>%4</p>")
                   .arg(QYH("Multiply this track's timestamps with a factor."))
                   .arg(QYH("The value can be given either as a floating point number (e.g. 12.345) or a fraction of numbers (e.g. 123/456.78)."))
                   .arg(QYH("This works well for video and subtitle tracks but should not be used with audio tracks."))
                   .arg(QYH("This option can also be used for chapters.")));
  Util::setToolTip(p.ui->defaultDuration,
                   Q("%1 %2 %3 %4")
                   .arg(QY("Forces the default duration or number of frames per second for a track."))
                   .arg(QY("The value can be given either as a floating point number (e.g. 12.345) or a fraction of integer values (e.g. 123/456)."))
                   .arg(QY("You can specify one of the units 's', 'ms', 'us', 'ns', 'fps', 'i' or 'p'."))
                   .arg(QY("If no unit is given, 'fps' will be used.")));
  Util::setToolTip(p.ui->fixBitstreamTimingInfo,
                   Q("%1 %2 %3")
                   .arg(QY("Normally mkvmerge does not change the timing information (frame/field rate) stored in the video bitstream."))
                   .arg(QY("With this option that information is adjusted to match the container's timing information."))
                   .arg(QY("There are several potential sources for the container's timing information: a value given on the command line with the '--default-duration' option, "
                           "the source container or the video bitstream.")));
  Util::setToolTip(p.ui->aspectRatio,
                   Q("<p>%1 %2 %3</p><p>%4</p>")
                   .arg(QYH("The Matroska container format can store the display width/height for a video track."))
                   .arg(QYH("This option tells mkvmerge the display aspect ratio to use when it calculates the display width/height."))
                   .arg(QYH("Note that many players don't use the display width/height values directly but only use the ratio given by these values when setting the initial window size."))
                   .arg(QYH("The value can be given either as a floating point number (e.g. 12.345) or a fraction of integer values (e.g. 123/456).")));
  Util::setToolTip(p.ui->displayWidth,
                   Q("<p>%1 %2</p><p>%3</p>")
                   .arg(QYH("The Matroska container format can store the display width/height for a video track."))
                   .arg(QYH("This parameter is the display width in pixels."))
                   .arg(QYH("Note that many players don't use the display width/height values directly but only use the ratio given by these values when setting the initial window size.")));
  Util::setToolTip(p.ui->displayHeight,
                   Q("<p>%1 %2</p><p>%3</p>")
                   .arg(QYH("The Matroska container format can store the display width/height for a video track."))
                   .arg(QYH("This parameter is the display height in pixels."))
                   .arg(QYH("Note that many players don't use the display width/height values directly but only use the ratio given by these values when setting the initial window size.")));
  Util::setToolTip(p.ui->cropping,
                   Q("<p>%1 %2</p><p>%3 %4</p><p>%5</p>")
                   .arg(QYH("Sets the cropping parameters which tell a player to omit a certain number of pixels on the four sides during playback."))
                   .arg(QYH("This must be a comma-separated list of four numbers for the cropping to be used at the left, top, right and bottom, e.g. '0,20,0,20'."))
                   .arg(QYH("Note that the video content is not modified by this option."))
                   .arg(QYH("The values are only stored in the track headers."))
                   .arg(QYH("Note also that there are not a lot of players that support the cropping parameters.")));
  Util::setToolTip(p.ui->stereoscopy,
                   Q("%1 %2")
                   .arg(QY("Sets the stereo mode of the video track to this value."))
                   .arg(QY("If left empty then the track's original stereo mode will be kept or, if it didn't have one, none will be set at all.")));
  Util::setToolTip(p.ui->aacIsSBR,
                   Q("%1 %2 %3")
                   .arg(QY("This track contains SBR AAC/HE-AAC/AAC+ data."))
                   .arg(QY("Only needed for AAC source files as SBR AAC cannot be detected automatically for these files."))
                   .arg(QY("Not needed for AAC tracks read from other container formats like MP4 or Matroska files.")));
  Util::setToolTip(p.ui->reduceToAudioCore,
                   Q("%1 %2")
                   .arg(QY("Drops all HD extensions from an audio track and keeps only its lossy core."))
                   .arg(QY("This only works with DTS audio tracks.")));
  Util::setToolTip(p.ui->removeDialogNormalizationGain,
                   Q("%1 %2")
                   .arg(QY("Removes or at least minimizes the dialog normalization gain by modifying audio headers."))
                   .arg(QY("This only works with AC-3, DTS & TrueHD audio tracks.")));
  Util::setToolTip(p.ui->subtitleCharacterSet,
                   Q("<p>%1 %2</p><p><ol><li>%3</li><li>%4</li></p>")
                   .arg(QYH("Selects the character set a subtitle file or chapter information was written with."))
                   .arg(QYH("Only needed in certain situations:"))
                   .arg(QYH("for subtitle files that do not use a byte order marker (BOM) and that are not encoded in the system's current character set (%1)").arg(Q(g_cc_local_utf8->get_charset())))
                   .arg(QYH("for files with chapter information (e.g. OGM, MP4) for which mkvmerge does not detect the encoding correctly")));
  Util::setToolTip(p.ui->cues,
                   Q("%1 %2")
                   .arg(QY("Selects for which blocks mkvmerge will produce index entries ( = cue entries)."))
                   .arg(QY("\"Determine automatically\" is a good choice for almost all situations.")));
  Util::setToolTip(p.ui->additionalTrackOptions,
                   Q("%1 %2 %3")
                   .arg(QY("Free-form edit field for user defined options for this track."))
                   .arg(QY("What you input here is added after all the other options the GUI adds so that you could overwrite any of the GUI's options for this track."))
                   .arg(QY("All occurrences of the string \"<TID>\" will be replaced by the track's track ID.")));
}

void
Tab::setupPredefinedTrackNames() {
  auto &p = *p_func();

  QSignalBlocker const blocker{p.ui->trackName};

  auto name = p.ui->trackName->currentText();
  QMap<TrackType, bool> haveType;

  Util::withSelectedIndexes(p.ui->tracks, [&](QModelIndex const &idx) {
    auto track = p.tracksModel->fromIndex(idx);
    if (track)
      haveType[track->m_type] = true;
  });

  QStringList items;
  QMap<QString, bool> haveItem;

  auto addItemsMaybe = [&items, &haveItem](QStringList const &newItems) {
    for (auto const &newItem : newItems)
      if (!haveItem[newItem]) {
        haveItem[newItem] = true;
        items += newItem;
      }
  };

  auto &settings = Util::Settings::get();

  if (haveType[TrackType::Audio])
    addItemsMaybe(settings.m_mergePredefinedAudioTrackNames);
  if (haveType[TrackType::Video])
    addItemsMaybe(settings.m_mergePredefinedVideoTrackNames);
  if (haveType[TrackType::Subtitles])
    addItemsMaybe(settings.m_mergePredefinedSubtitleTrackNames);

  p.ui->trackName->clear();
  p.ui->trackName->addItems(items);
  p.ui->trackName->setCurrentText(name);
}

void
Tab::onFileRowsInserted(QModelIndex const &parentIdx,
                        int,
                        int) {
  if (parentIdx.isValid())
    p_func()->ui->files->setExpanded(parentIdx, true);
}

void
Tab::onTrackRowsInserted(QModelIndex const &parentIdx,
                         int,
                         int) {
  if (parentIdx.isValid())
    p_func()->ui->tracks->setExpanded(parentIdx, true);
}

void
Tab::onTrackSelectionChanged() {
  auto &p = *p_func();

  Util::enableWidgets(p.allInputControls, false);
  p.ui->moveTracksUp->setEnabled(false);
  p.ui->moveTracksDown->setEnabled(false);
  p.ui->subtitleCharacterSetPreview->setEnabled(false);

  auto selection = p.ui->tracks->selectionModel()->selection();
  auto numRows   = Util::numSelectedRows(selection);
  if (!numRows) {
    clearInputControlValues();
    return;
  }

  p.ui->moveTracksUp->setEnabled(true);
  p.ui->moveTracksDown->setEnabled(true);

  if (1 < numRows) {
    setInputControlValues(nullptr);
    setupPredefinedTrackNames();
    Util::enableWidgets(p.allInputControls, true);
    return;
  }

  Util::enableWidgets(p.typeIndependentControls, true);

  auto idxs = selection[0].indexes();
  if (idxs.isEmpty() || !idxs[0].isValid())
    return;

  auto track = p.tracksModel->fromIndex(idxs[0]);
  if (!track)
    return;

  setInputControlValues(track);
  setupPredefinedTrackNames();

  if (track->isAudio()) {
    Util::enableWidgets(p.audioControls, true);
    Util::enableWidgets({ p.ui->aacIsSBRLabel, p.ui->aacIsSBR }, track->canSetAacToSbr());
    p.ui->reduceToAudioCore->setEnabled(track->canReduceToAudioCore());
    p.ui->removeDialogNormalizationGain->setEnabled(track->canRemoveDialogNormalizationGain());

  } else if (track->isVideo())
    Util::enableWidgets(p.videoControls, true);

  else if (track->isSubtitles()) {
    Util::enableWidgets(p.subtitleControls, true);
    if (!track->canChangeSubCharset())
      Util::enableWidgets(QList<QWidget *>{} << p.ui->characterSetLabel << p.ui->subtitleCharacterSet, false);

    else if (track->m_file->isTextSubtitleContainer())
      p.ui->subtitleCharacterSetPreview->setEnabled(true);

  } else if (track->isChapters()) {
    Util::enableWidgets(p.chapterControls, true);

    if (!track->canChangeSubCharset())
      Util::enableWidgets(QList<QWidget *>{} << p.ui->characterSetLabel << p.ui->subtitleCharacterSet, false);
  }

  if (track->isAppended())
    Util::enableWidgets(p.notIfAppendingControls, false);
}

void
Tab::addOrRemoveEmptyComboBoxItem(bool add) {
  for (auto &comboBox : p_func()->comboBoxControls)
    if (add && comboBox->itemData(0).isValid())
      comboBox->insertItem(0, QY("<Do not change>"));
    else if (!add && !comboBox->itemData(0).isValid())
      comboBox->removeItem(0);
}

void
Tab::clearInputControlValues() {
  auto &p = *p_func();

  p.ui->trackLanguage->setLanguage({});

  for (auto comboBox : p.comboBoxControls)
    comboBox->setCurrentIndex(0);

  for (auto control : std::vector<QLineEdit *>{p.ui->trackTags, p.ui->delay, p.ui->stretchBy, p.ui->timestamps, p.ui->displayWidth, p.ui->displayHeight, p.ui->cropping, p.ui->additionalTrackOptions})
    control->setText(Q(""));

  for (auto control : std::vector<QComboBox *>{p.ui->trackName, p.ui->defaultDuration, p.ui->aspectRatio})
    control->setEditText(Q(""));

  p.ui->setAspectRatio->setChecked(false);
  p.ui->setDisplayWidthHeight->setChecked(false);
}

void
Tab::setInputControlValues(Track *track) {
  auto &p = *p_func();

  p.currentlySettingInputControlValues = true;

  addOrRemoveEmptyComboBoxItem(!track);

  auto additionalLanguages     = QSet<QString>{};
  auto additionalCharacterSets = QSet<QString>{};

  for (auto const &sourceFile : p.config.m_files)
    for (auto const &sourceTrack : sourceFile->m_tracks) {
      additionalLanguages     << Q(sourceTrack->m_language.format());
      additionalCharacterSets << sourceTrack->m_characterSet;
    }

  p.ui->trackLanguage->setAdditionalLanguages(additionalLanguages.values());
  p.ui->subtitleCharacterSet->setAdditionalItems(additionalCharacterSets.values()).reInitializeIfNecessary();

  if (!track) {
    clearInputControlValues();
    p.currentlySettingInputControlValues = false;
    return;
  }

  Util::setComboBoxIndexIf(p.ui->muxThis,              [&track](auto const &, auto const &data) { return data.isValid() && (data.toBool() == track->m_muxThis);                                });
  Util::setComboBoxIndexIf(p.ui->defaultTrackFlag,     [&track](auto const &, auto const &data) { return data.isValid() && (data.toBool() == track->m_defaultTrackFlag);                       });
  Util::setComboBoxIndexIf(p.ui->forcedTrackFlag,      [&track](auto const &, auto const &data) { return data.isValid() && (data.toUInt() == track->m_forcedTrackFlag);                        });
  Util::setComboBoxIndexIf(p.ui->trackEnabledFlag,     [&track](auto const &, auto const &data) { return data.isValid() && (data.toUInt() == track->m_trackEnabledFlag);                       });
  Util::setComboBoxIndexIf(p.ui->hearingImpairedFlag,  [&track](auto const &, auto const &data) { return data.isValid() && (data.toBool() == track->m_hearingImpairedFlag);                    });
  Util::setComboBoxIndexIf(p.ui->visualImpairedFlag,   [&track](auto const &, auto const &data) { return data.isValid() && (data.toBool() == track->m_visualImpairedFlag);                     });
  Util::setComboBoxIndexIf(p.ui->textDescriptionsFlag, [&track](auto const &, auto const &data) { return data.isValid() && (data.toBool() == track->m_textDescriptionsFlag);                   });
  Util::setComboBoxIndexIf(p.ui->originalFlag,         [&track](auto const &, auto const &data) { return data.isValid() && (data.toBool() == track->m_originalFlag);                           });
  Util::setComboBoxIndexIf(p.ui->commentaryFlag,       [&track](auto const &, auto const &data) { return data.isValid() && (data.toBool() == track->m_commentaryFlag);                         });
  Util::setComboBoxIndexIf(p.ui->compression,          [&track](auto const &, auto const &data) { return data.isValid() && (data.toUInt() == static_cast<unsigned int>(track->m_compression)); });
  Util::setComboBoxIndexIf(p.ui->cues,                 [&track](auto const &, auto const &data) { return data.isValid() && (data.toUInt() == track->m_cues);                                   });
  Util::setComboBoxIndexIf(p.ui->stereoscopy,          [&track](auto const &, auto const &data) { return data.isValid() && (data.toUInt() == track->m_stereoscopy);                            });
  Util::setComboBoxIndexIf(p.ui->aacIsSBR,             [&track](auto const &, auto const &data) { return data.isValid() && (data.toUInt() == track->m_aacIsSBR);                               });

  p.ui->trackLanguage->setLanguage(track->m_language);
  p.ui->subtitleCharacterSet->setCurrentByData(track->m_characterSet);

  p.ui->trackName->setEditText(                    track->m_name);
  p.ui->trackTags->setText(                        track->m_tags);
  p.ui->delay->setText(                            track->m_delay);
  p.ui->stretchBy->setText(                        track->m_stretchBy);
  p.ui->timestamps->setText(                       track->m_timestamps);
  p.ui->displayWidth->setText(                     track->m_displayWidth);
  p.ui->displayHeight->setText(                    track->m_displayHeight);
  p.ui->cropping->setText(                         track->m_cropping);
  p.ui->additionalTrackOptions->setText(           track->m_additionalOptions);
  p.ui->defaultDuration->setEditText(              track->m_defaultDuration);
  p.ui->aspectRatio->setEditText(                  track->m_aspectRatio);

  p.ui->setAspectRatio->setChecked(                track->m_setAspectRatio);
  p.ui->setDisplayWidthHeight->setChecked(        !track->m_setAspectRatio);
  p.ui->fixBitstreamTimingInfo->setChecked(        track->m_fixBitstreamTimingInfo);
  p.ui->reduceToAudioCore->setChecked(             track->m_reduceAudioToCore);
  p.ui->removeDialogNormalizationGain->setChecked( track->m_removeDialogNormalizationGain);

  p.ui->colorMatrixCoefficients->setText(          track->m_colorMatrixCoefficients);
  p.ui->bitsPerColorChannel->setText(              track->m_bitsPerColorChannel);
  p.ui->chromaSubsampling->setText(                track->m_chromaSubsampling);
  p.ui->cbSubsampling->setText(                    track->m_cbSubsampling);
  p.ui->chromaSiting->setText(                     track->m_chromaSiting);
  p.ui->colorRange->setText(                       track->m_colorRange);
  p.ui->transferCharacteristics->setText(          track->m_transferCharacteristics);
  p.ui->colorPrimaries->setText(                   track->m_colorPrimaries);
  p.ui->maximumContentLight->setText(              track->m_maximumContentLight);
  p.ui->maximumFrameLight->setText(                track->m_maximumFrameLight);

  p.ui->chromaticityCoordinates->setText(          track->m_chromaticityCoordinates);
  p.ui->whiteColorCoordinates->setText(            track->m_whiteColorCoordinates);
  p.ui->maximumLuminance->setText(                 track->m_maximumLuminance);
  p.ui->minimumLuminance->setText(                 track->m_minimumLuminance);

  p.ui->projectionType->setText(                   track->m_projectionType);
  p.ui->projectionSpecificData->setText(           track->m_projectionSpecificData);
  p.ui->yawRotation->setText(                      track->m_yawRotation);
  p.ui->pitchRotation->setText(                    track->m_pitchRotation);
  p.ui->rollRotation->setText(                     track->m_rollRotation);

  p.currentlySettingInputControlValues = false;
}

QList<SourceFile *>
Tab::selectedSourceFiles()
  const {
  auto &p          = *p_func();
  auto sourceFiles = QList<SourceFile *>{};
  Util::withSelectedIndexes(p.ui->files, [&sourceFiles, &p](QModelIndex const &idx) {
      auto sourceFile = p.filesModel->fromIndex(idx);
      if (sourceFile)
        sourceFiles << sourceFile.get();
  });

  return sourceFiles;
}

QList<Track *>
Tab::selectedTracks()
  const {
  auto &p     = *p_func();
  auto tracks = QList<Track *>{};
  Util::withSelectedIndexes(p.ui->tracks, [&tracks, &p](QModelIndex const &idx) {
      auto track = p.tracksModel->fromIndex(idx);
      if (track)
        tracks << track;
    });

  return tracks;
}

void
Tab::withSelectedTracks(std::function<void(Track &)> code,
                        bool notIfAppending,
                        QWidget *widget) {
  auto &p = *p_func();

  if (p.currentlySettingInputControlValues)
    return;

  auto tracks = selectedTracks();
  if (tracks.isEmpty())
    return;

  if (!widget)
    widget = static_cast<QWidget *>(QObject::sender());

  bool withAudio     = p.audioControls.contains(widget);
  bool withVideo     = p.videoControls.contains(widget);
  bool withSubtitles = p.subtitleControls.contains(widget);
  bool withChapters  = p.chapterControls.contains(widget);
  bool withAll       = p.typeIndependentControls.contains(widget);

  for (auto &track : tracks) {
    if (track->m_appendedTo && notIfAppending)
      continue;

    if (   withAll
        || (track->isAudio()     && withAudio)
        || (track->isVideo()     && withVideo)
        || (track->isSubtitles() && withSubtitles)
        || (track->isChapters()  && withChapters)) {
      code(*track);
      p.tracksModel->trackUpdated(track);
    }
  }
}

bool
Tab::hasSelectedNotAppendedRegularTracks()
  const {
  auto &p     = *p_func();
  auto result = false;

  Util::withSelectedIndexes(p.ui->tracks, [&p, &result](QModelIndex const &idx) {
    auto track = p.tracksModel->fromIndex(idx);
    if (track && !track->isAppended() && track->isRegular())
      result = true;
  });

  return result;
}

void
Tab::onTrackNameChanged(QString newValue) {
  withSelectedTracks([&newValue](auto &track) { track.m_name = newValue; }, true);
}

void
Tab::onTrackItemChanged(QStandardItem *item) {
  auto &p = *p_func();

  if (!item)
    return;

  auto idx = p.tracksModel->indexFromItem(item);
  if (idx.column())
    return;

  auto track = p.tracksModel->fromIndex(idx);
  if (!track)
    return;

  auto newWantedMuxThis = (item->checkState() == Qt::Checked);
  auto newMuxThis       = newWantedMuxThis && (!track->m_appendedTo || track->m_appendedTo->m_muxThis);

  if (newWantedMuxThis != newMuxThis)
    item->setCheckState(newMuxThis ? Qt::Checked : Qt::Unchecked);

  if (newMuxThis == track->m_muxThis)
    return;

  track->m_muxThis = newMuxThis;
  p.tracksModel->trackUpdated(track);

  for (auto appendedTrack : track->m_appendedTracks) {
    appendedTrack->m_muxThis = newMuxThis;
    p.tracksModel->trackUpdated(appendedTrack);
  }

  auto selected = selectedTracks();
  if ((1 == selected.count()) && selected.contains(track))
    Util::setComboBoxIndexIf(p.ui->muxThis, [newMuxThis](QString const &, QVariant const &data) { return data.isValid() && (data.toBool() == newMuxThis); });

  setOutputFileNameMaybe();
}

void
Tab::onMuxThisChanged(int selected) {
  auto &p   = *p_func();
  auto data = p.ui->muxThis->itemData(selected);
  if (!data.isValid())
    return;

  auto muxThis = data.toBool();
  QList<Track *> appendedTracks;

  withSelectedTracks([muxThis, &appendedTracks](auto &track) {
    if (!track.m_appendedTo || track.m_appendedTo->m_muxThis)
      track.m_muxThis = muxThis;
    appendedTracks += track.m_appendedTracks;
  });

  for (auto track : appendedTracks) {
    track->m_muxThis = muxThis;
    p.tracksModel->trackUpdated(track);
  }

  setOutputFileNameMaybe();

  auto tracks = selectedTracks();
  if (1 == tracks.count())
    Util::setComboBoxIndexIf(p.ui->muxThis, [&tracks](QString const &, QVariant const &eltData) { return eltData.isValid() && (eltData.toBool() == tracks[0]->m_muxThis); });
}

void
Tab::toggleMuxThisForSelectedTracks() {
  auto &p             = *p_func();
  auto allEnabled     = true;
  auto tracksSelected = false;

  withSelectedTracks([&allEnabled, &tracksSelected](auto &track) {
    tracksSelected = true;

    if (!track.m_muxThis)
      allEnabled = false;
  }, false, p.ui->muxThis);

  if (!tracksSelected)
    return;

  auto newEnabled = !allEnabled;
  QList<Track *> appendedTracks;

  withSelectedTracks([newEnabled, &appendedTracks](auto &track) {
    if (!track.m_appendedTo || track.m_appendedTo->m_muxThis)
      track.m_muxThis = newEnabled;
    appendedTracks += track.m_appendedTracks;
  }, false, p.ui->muxThis);

  for (auto track : appendedTracks) {
    track->m_muxThis = newEnabled;
    p.tracksModel->trackUpdated(track);
  }

  Util::setComboBoxIndexIf(p.ui->muxThis, [newEnabled](auto const &, auto const &data) { return data.isValid() && (data.toBool() == newEnabled); });
}

void
Tab::onTrackLanguageChanged(mtx::bcp47::language_c const &newLanguage) {
  withSelectedTracks([&newLanguage](auto &track) { track.m_language = newLanguage; }, true, p_func()->ui->trackLanguage);
}

void
Tab::onDefaultTrackFlagChanged(int newValue) {
  auto &p   = *p_func();
  auto data = p.ui->defaultTrackFlag->itemData(newValue);

  if (!data.isValid())
    return;

  newValue = data.toBool();

  withSelectedTracks([newValue](auto &track) { track.m_defaultTrackFlag = newValue; }, true);
}

void
Tab::onForcedTrackFlagChanged(int newValue) {
  auto &p   = *p_func();
  auto data = p.ui->forcedTrackFlag->itemData(newValue);

  if (!data.isValid())
    return;

  newValue = data.toInt();

  withSelectedTracks([&newValue](auto &track) { track.m_forcedTrackFlag = newValue; }, true);
}

void
Tab::onTrackEnabledFlagChanged(int newValue) {
  auto &p   = *p_func();
  auto data = p.ui->trackEnabledFlag->itemData(newValue);

  if (!data.isValid())
    return;

  newValue = data.toInt();

  withSelectedTracks([&newValue](auto &track) { track.m_trackEnabledFlag = newValue; }, true);
}

void
Tab::onHearingImpairedFlagChanged(int newValue) {
  auto &p   = *p_func();
  auto data = p.ui->hearingImpairedFlag->itemData(newValue);

  if (!data.isValid())
    return;

  newValue = data.toBool();

  withSelectedTracks([&newValue](auto &track) { track.m_hearingImpairedFlag = newValue; }, true);
}

void
Tab::onVisualImpairedFlagChanged(int newValue) {
  auto &p   = *p_func();
  auto data = p.ui->visualImpairedFlag->itemData(newValue);

  if (!data.isValid())
    return;

  newValue = data.toBool();

  withSelectedTracks([&newValue](auto &track) { track.m_visualImpairedFlag = newValue; }, true);
}

void
Tab::onTextDescriptionsFlagChanged(int newValue) {
  auto &p   = *p_func();
  auto data = p.ui->textDescriptionsFlag->itemData(newValue);

  if (!data.isValid())
    return;

  newValue = data.toBool();

  withSelectedTracks([&newValue](auto &track) { track.m_textDescriptionsFlag = newValue; }, true);
}

void
Tab::onOriginalFlagChanged(int newValue) {
  auto &p   = *p_func();
  auto data = p.ui->originalFlag->itemData(newValue);

  if (!data.isValid())
    return;

  newValue = data.toBool();

  withSelectedTracks([&newValue](auto &track) { track.m_originalFlag = newValue; }, true);
}

void
Tab::onCommentaryFlagChanged(int newValue) {
  auto &p   = *p_func();
  auto data = p.ui->commentaryFlag->itemData(newValue);

  if (!data.isValid())
    return;

  newValue = data.toBool();

  withSelectedTracks([&newValue](auto &track) { track.m_commentaryFlag = newValue; }, true);
}

void
Tab::onCompressionChanged(int newValue) {
  auto data = p_func()->ui->compression->itemData(newValue);
  if (!data.isValid())
    return;
  newValue = data.toInt();

  auto compression = 1 == newValue ? TrackCompression::None
                   : 2 == newValue ? TrackCompression::Zlib
                   :                 TrackCompression::Default;

  withSelectedTracks([compression](auto &track) { track.m_compression = compression; }, true);
}

void
Tab::onTrackTagsChanged(QString newValue) {
  withSelectedTracks([&newValue](auto &track) { track.m_tags = newValue; }, true);
}

void
Tab::onDelayChanged(QString newValue) {
  withSelectedTracks([&newValue](auto &track) { track.m_delay = newValue; });
}

void
Tab::onStretchByChanged(QString newValue) {
  withSelectedTracks([&newValue](auto &track) { track.m_stretchBy = newValue; });
}

void
Tab::onDefaultDurationChanged(QString newValue) {
  withSelectedTracks([&newValue](auto &track) { track.m_defaultDuration = newValue; }, true);
}

void
Tab::onColorMatrixCoefficientsChanged(QString newValue) {
  withSelectedTracks([&newValue](auto &track) { track.m_colorMatrixCoefficients = newValue; }, true);
}

void
Tab::onBitsPerColorChannelChanged(QString newValue) {
  withSelectedTracks([&newValue](auto &track) { track.m_bitsPerColorChannel = newValue; }, true);
}

void
Tab::onChromaSubsamplingChanged(QString newValue) {
  withSelectedTracks([&newValue](auto &track) { track.m_chromaSubsampling = newValue; }, true);
}

void
Tab::onCbSubsamplingChanged(QString newValue) {
  withSelectedTracks([&newValue](auto &track) { track.m_cbSubsampling = newValue; }, true);
}

void
Tab::onChromaSitingChanged(QString newValue) {
  withSelectedTracks([&newValue](auto &track) { track.m_chromaSiting = newValue; }, true);
}

void
Tab::onColorRangeChanged(QString newValue) {
  withSelectedTracks([&newValue](auto &track) { track.m_colorRange = newValue; }, true);
}

void
Tab::onTransferCharacteristicsChanged(QString newValue) {
  withSelectedTracks([&newValue](auto &track) { track.m_transferCharacteristics = newValue; }, true);
}

void
Tab::onColorPrimariesChanged(QString newValue) {
  withSelectedTracks([&newValue](auto &track) { track.m_colorPrimaries = newValue; }, true);
}

void
Tab::onMaximumContentLightChanged(QString newValue) {
  withSelectedTracks([&newValue](auto &track) { track.m_maximumContentLight = newValue; }, true);
}

void
Tab::onMaximumFrameLightChanged(QString newValue) {
  withSelectedTracks([&newValue](auto &track) { track.m_maximumFrameLight = newValue; }, true);
}

void
Tab::onChromaticityCoordinatesChanged(QString newValue) {
  withSelectedTracks([&newValue](auto &track) { track.m_chromaticityCoordinates = newValue; }, true);
}

void
Tab::onWhiteColorCoordinatesChanged(QString newValue) {
  withSelectedTracks([&newValue](auto &track) { track.m_whiteColorCoordinates = newValue; }, true);
}

void
Tab::onMaximumLuminanceChanged(QString newValue) {
  withSelectedTracks([&newValue](auto &track) { track.m_maximumLuminance = newValue; }, true);
}

void
Tab::onMinimumLuminanceChanged(QString newValue) {
  withSelectedTracks([&newValue](auto &track) { track.m_minimumLuminance = newValue; }, true);
}

void
Tab::onProjectionTypeChanged(QString newValue) {
  withSelectedTracks([&newValue](auto &track) { track.m_projectionType = newValue; }, true);
}

void
Tab::onProjectionSpecificDataChanged(QString newValue) {
  withSelectedTracks([&newValue](auto &track) { track.m_projectionSpecificData = newValue; }, true);
}

void
Tab::onYawRotationChanged(QString newValue) {
  withSelectedTracks([&newValue](auto &track) { track.m_yawRotation = newValue; }, true);
}

void
Tab::onPitchRotationChanged(QString newValue) {
  withSelectedTracks([&newValue](auto &track) { track.m_pitchRotation = newValue; }, true);
}

void
Tab::onRollRotationChanged(QString newValue) {
  withSelectedTracks([&newValue](auto &track) { track.m_rollRotation = newValue; }, true);
}

void
Tab::onTimestampsChanged(QString newValue) {
  withSelectedTracks([&newValue](auto &track) { track.m_timestamps = newValue; });
}

void
Tab::onBrowseTimestamps() {
  auto fileName = getOpenFileName(QY("Select timestamp file"), QY("Text files") + Q(" (*.txt)"), p_func()->ui->timestamps);
  if (!fileName.isEmpty())
    withSelectedTracks([&fileName](auto &track) { track.m_timestamps = fileName; });
}

void
Tab::onFixBitstreamTimingInfoChanged(bool newValue) {
  withSelectedTracks([&newValue](auto &track) { track.m_fixBitstreamTimingInfo = newValue; }, true);
}

void
Tab::onBrowseTrackTags() {
  auto fileName = getOpenFileName(QY("Select tags file"), QY("XML tag files") + Q(" (*.xml)"), p_func()->ui->trackTags);
  if (!fileName.isEmpty())
    withSelectedTracks([&fileName](auto &track) { track.m_tags = fileName; }, true);
}

void
Tab::onSetAspectRatio() {
  withSelectedTracks([](auto &track) { track.m_setAspectRatio = true; }, true);
}

void
Tab::onSetDisplayDimensions() {
  withSelectedTracks([](auto &track) { track.m_setAspectRatio = false; }, true);
}

void
Tab::onAspectRatioChanged(QString newValue) {
  if (!newValue.isEmpty()) {
    p_func()->ui->setAspectRatio->setChecked(true);
    onSetAspectRatio();
  }

  withSelectedTracks([&newValue](auto &track) { track.m_aspectRatio = newValue; }, true);
}

void
Tab::onDisplayWidthChanged(QString newValue) {
  if (!newValue.isEmpty()) {
    p_func()->ui->setDisplayWidthHeight->setChecked(true);
    onSetDisplayDimensions();
  }

  withSelectedTracks([&newValue](auto &track) { track.m_displayWidth = newValue; }, true);
}

void
Tab::onDisplayHeightChanged(QString newValue) {
  if (!newValue.isEmpty()) {
    p_func()->ui->setDisplayWidthHeight->setChecked(true);
    onSetDisplayDimensions();
  }

  withSelectedTracks([&newValue](auto &track) { track.m_displayHeight = newValue; }, true);
}

void
Tab::onStereoscopyChanged(int newValue) {
  auto data = p_func()->ui->stereoscopy->itemData(newValue);
  if (!data.isValid())
    return;
  newValue = data.toInt();

  withSelectedTracks([&newValue](auto &track) { track.m_stereoscopy = newValue; }, true);
}

void
Tab::onCroppingChanged(QString newValue) {
  withSelectedTracks([&newValue](auto &track) { track.m_cropping = newValue; }, true);
}

void
Tab::onAacIsSBRChanged(int newValue) {
  auto data = p_func()->ui->aacIsSBR->itemData(newValue);
  if (!data.isValid())
    return;
  newValue = data.toInt();

  withSelectedTracks([&newValue](auto &track) { track.m_aacIsSBR = newValue; }, true);
}

void
Tab::onReduceAudioToCoreChanged(bool newValue) {
  withSelectedTracks([&newValue](auto &track) {
    if (track.canReduceToAudioCore())
      track.m_reduceAudioToCore = newValue;
  });
}

void
Tab::onRemoveDialogNormalizationGainChanged(bool newValue) {
  withSelectedTracks([&newValue](auto &track) {
    if (track.canRemoveDialogNormalizationGain())
      track.m_removeDialogNormalizationGain = newValue;
  });
}

void
Tab::onSubtitleCharacterSetChanged(int newValue) {
  auto data = p_func()->ui->subtitleCharacterSet->itemData(newValue);
  if (!data.isValid())
    return;

  withSelectedTracks([&data](auto &track) {
    if (track.canChangeSubCharset())
      track.m_characterSet = data.toString();
  });
}

void
Tab::onCuesChanged(int newValue) {
  auto data = p_func()->ui->cues->itemData(newValue);
  if (!data.isValid())
    return;
  newValue = data.toInt();

  withSelectedTracks([&newValue](auto &track) { track.m_cues = newValue; }, true);
}

void
Tab::onAdditionalTrackOptionsChanged(QString newValue) {
  withSelectedTracks([&newValue](auto &track) { track.m_additionalOptions = newValue; }, true);
}

void
Tab::onAddFiles() {
  selectFilesAndIdentifyForAddingOrAppending(IdentificationPack::AddMode::Add);
}

void
Tab::onAppendFiles() {
  selectFilesAndIdentifyForAddingOrAppending(IdentificationPack::AddMode::Append);
}

void
Tab::selectFilesAndIdentifyForAddingOrAppending(IdentificationPack::AddMode addMode) {
  auto fileNames = selectFilesToAdd(addMode == IdentificationPack::AddMode::Append ? QY("Append media files") : QY("Add media files"));
  if (fileNames.isEmpty())
    return;

  IdentificationPack pack;
  pack.m_tabId         = reinterpret_cast<uint64_t>(this);
  pack.m_addMode       = addMode;
  pack.m_fileNames     = fileNames;
  // pack.m_sourceFileIdx = sourceFileIdx;
  pack.m_sourceFileIdx = selectedSourceFile();

  Tool::identifier().addPackToIdentify(pack);
}

void
Tab::addOrAppendIdentifiedFiles(QVector<SourceFilePtr> const &identifiedFiles,
                                QModelIndex const &fileModelIdx,
                                IdentificationPack::AddMode addMode) {
  auto &p = *p_func();

  if (identifiedFiles.isEmpty())
    return;

  if (p.debugTrackModel) {
    log_it(fmt::format("### BEFORE adding/appending ###\n"));
    p.config.debugDumpFileList();
    p.config.debugDumpTrackList();
  }

  setDefaultsFromSettingsForAddedFiles(identifiedFiles);

  p.filesModel->addOrAppendFilesAndTracks(identifiedFiles, fileModelIdx, addMode == IdentificationPack::AddMode::Append);

  if (p.debugTrackModel) {
    log_it(fmt::format("### AFTER adding/appending ###\n"));
    p.config.debugDumpFileList();
    p.config.debugDumpTrackList();
  }

  reinitFilesTracksControls();

  if (p.config.m_firstInputFileName.isEmpty())
    p.config.m_firstInputFileName = p.config.determineFirstInputFileName(identifiedFiles);

  setTitleMaybe(identifiedFiles);
  addDataFromIdentifiedBlurayFiles(identifiedFiles);
  setOutputFileNameMaybe();

  p.config.verifyStructure();
}

void
Tab::addIdentifiedFilesAsAdditionalParts(QVector<SourceFilePtr> const &identifiedFiles,
                                         QModelIndex const &fileModelIdx) {
  QStringList fileNames;

  for (auto const &identifiedFile : identifiedFiles)
    fileNames << identifiedFile->m_fileName;

  p_func()->filesModel->addAdditionalParts(fileNames, fileModelIdx);
}

void
Tab::addDataFromIdentifiedBlurayFiles(QVector<SourceFilePtr> const &files) {
  for (auto const &file : files) {
    if (!file->m_discLibraryInfoToAdd)
      continue;

    auto &info = *file->m_discLibraryInfoToAdd;

    if (!info.m_title.empty())
      setTitleMaybe(Q(info.m_title));

    if (Util::Settings::get().m_mergeAddBlurayCovers)
      addAttachmentsFromIdentifiedBluray(info);
  }
}

void
Tab::setDefaultsFromSettingsForAddedFiles(QVector<SourceFilePtr> const &files) {
  auto &cfg = Util::Settings::get();

  for (auto const &file : files)
    for (auto const &track : file->m_tracks) {
      if (cfg.m_disableCompressionForAllTrackTypes)
        track->m_compression = TrackCompression::None;

      if (cfg.m_disableDefaultTrackForSubtitles && track->isSubtitles())
        track->m_defaultTrackFlag = false;
    }
}

QStringList
Tab::selectFilesToAdd(QString const &title) {
  auto &settings = Util::Settings::get();
  auto fileNames = Util::getOpenFileNames(this, title, settings.lastOpenDirPath(), Util::FileTypeFilter::get().join(Q(";;")), nullptr, QFileDialog::HideNameFilterDetails);

  if (!fileNames.isEmpty()) {
    settings.m_lastOpenDir.setPath(QFileInfo{fileNames[0]}.path());
    settings.save();
  }

  return fileNames;
}

void
Tab::onAddAdditionalParts() {
  auto &p         = *p_func();
  auto currentIdx = selectedSourceFile();
  auto sourceFile = p.filesModel->fromIndex(currentIdx);

  if (sourceFile && !sourceFile->m_tracks.size()) {
    Util::MessageBox::critical(this)->title(QY("Unable to append files")).text(QY("You cannot add additional parts to files that don't contain tracks.")).exec();
    return;
  }

  p.filesModel->addAdditionalParts(selectFilesToAdd(QY("Add media files as additional parts")), currentIdx);
}

void
Tab::onRemoveFiles() {
  auto &p            = *p_func();
  auto selectedFiles = selectedSourceFiles();

  if (selectedFiles.isEmpty())
    return;

  p.filesModel->removeFiles(selectedFiles);

  reinitFilesTracksControls();

  if (!p.filesModel->rowCount()) {
    p.config.m_firstInputFileName.clear();
    clearDestinationMaybe();
    clearTitleMaybe();
  }
}

void
Tab::onRemoveAllFiles() {
  auto &p = *p_func();

  if (p.config.m_files.isEmpty())
    return;

  p.attachedFilesModel->reset();
  p.filesModel->removeRows(0, p.filesModel->rowCount());
  p.tracksModel->removeRows(0, p.tracksModel->rowCount());
  p.config.m_files.clear();
  p.config.m_tracks.clear();
  p.config.m_firstInputFileName.clear();

  reinitFilesTracksControls();
  clearDestinationMaybe();
  clearTitleMaybe();
}

void
Tab::reinitFilesTracksControls() {
  onTrackSelectionChanged();
}

void
Tab::enableMoveFilesButtons() {
  auto &p          = *p_func();
  auto hasSelected = !p.ui->files->selectionModel()->selection().isEmpty();

  p.ui->moveFilesUp->setEnabled(hasSelected);
  p.ui->moveFilesDown->setEnabled(hasSelected);
}

void
Tab::enableFilesActions() {
  auto &p              = *p_func();
  int numSelected      = selectedSourceFiles().size();
  bool hasRegularTrack = false;

  if (1 == numSelected)
    hasRegularTrack = p.config.m_files.end() != std::find_if(p.config.m_files.begin(), p.config.m_files.end(), [](SourceFilePtr const &file) { return file->hasRegularTrack(); });

  p.addFilesAction->setEnabled(true);
  p.addFilesAction2->setEnabled(true);
  p.appendFilesAction->setEnabled((1 == numSelected) && hasRegularTrack);
  p.appendFilesAction2->setEnabled((1 == numSelected) && hasRegularTrack);
  p.addAdditionalPartsAction->setEnabled(1 == numSelected);
  p.addAdditionalPartsAction2->setEnabled(1 == numSelected);
  p.removeFilesAction->setEnabled(0 < numSelected);
  p.removeAllFilesAction->setEnabled(!p.config.m_files.isEmpty());
  p.setDestinationFileNameAction->setEnabled(1 == numSelected);
  p.openFilesInMediaInfoAction->setEnabled(0 < numSelected);
  p.selectTracksFromFilesAction->setEnabled(0 < numSelected);

  p.removeFilesAction->setText(QNY("&Remove file", "&Remove files", numSelected));
  p.openFilesInMediaInfoAction->setText(QNY("Open file in &MediaInfo", "Open files in &MediaInfo", numSelected));
  p.selectTracksFromFilesAction->setText(QNY("Select all &items from selected file", "Select all &items from selected files", numSelected));
}

void
Tab::enableTracksActions() {
  auto &p         = *p_func();
  int numSelected = selectedTracks().size();
  bool hasTracks  = !!p.tracksModel->rowCount();

  p.selectAllTracksAction->setEnabled(hasTracks);
  p.selectTracksOfTypeMenu->setEnabled(hasTracks);
  p.enableAllTracksAction->setEnabled(hasTracks);
  p.disableAllTracksAction->setEnabled(hasTracks);

  p.selectAllVideoTracksAction->setEnabled(hasTracks);
  p.selectAllAudioTracksAction->setEnabled(hasTracks);
  p.selectAllSubtitlesTracksAction->setEnabled(hasTracks);

  p.openTracksInMediaInfoAction->setEnabled(0 < numSelected);

  p.openTracksInMediaInfoAction->setText(QNY("Open corresponding file in &MediaInfo", "Open corresponding files in &MediaInfo", numSelected));
}

void
Tab::retranslateInputUI() {
  auto &p = *p_func();

  p.ui->trackLanguage->setClearTitle(QY("<Do not change>"));

  p.filesModel->retranslateUi();
  p.tracksModel->retranslateUi();

  p.addFilesAction ->setText(QY("&Add files"));
  p.addFilesAction2->setText(QY("&Add files"));
  p.appendFilesAction ->setText(QY("A&ppend files"));
  p.appendFilesAction2->setText(QY("A&ppend files"));
  p.addAdditionalPartsAction ->setText(QY("Add files as a&dditional parts"));
  p.addAdditionalPartsAction2->setText(QY("Add files as a&dditional parts"));

  p.removeAllFilesAction->setText(QY("Remove a&ll files"));

  p.setDestinationFileNameAction->setText(QY("Set destination &file name from selected file's name"));

  p.selectAllTracksAction->setText(QY("&Select all items"));
  p.selectTracksOfTypeMenu->setTitle(QY("Select all tracks of specific &type"));
  p.enableAllTracksAction->setText(QY("&Enable all items"));
  p.disableAllTracksAction->setText(QY("&Disable all items"));

  p.selectAllVideoTracksAction->setText(QY("&Video"));
  p.selectAllAudioTracksAction->setText(QY("&Audio"));
  p.selectAllSubtitlesTracksAction->setText(QY("&Subtitles"));

  p.startMuxingLeaveAsIs->setText(QY("Afterwards &leave the settings as they are."));
  p.startMuxingCreateNewSettings->setText(QY("Afterwards create &new multiplex settings and close the current ones."));
  p.startMuxingCloseSettings->setText(QY("Afterwards &close the current multiplex settings."));
  p.startMuxingRemoveInputFiles->setText(QY("Afterwards &remove all source files."));

  p.addToJobQueueLeaveAsIs->setText(QY("Afterwards &leave the settings as they are."));
  p.addToJobQueueCreateNewSettings->setText(QY("Afterwards create &new multiplex settings and close the current ones."));
  p.addToJobQueueCloseSettings->setText(QY("Afterwards &close the current multiplex settings."));
  p.addToJobQueueRemoveInputFiles->setText(QY("Afterwards &remove all source files."));

  for (auto idx = 0u, end = stereo_mode_c::max_index(); idx <= end; ++idx)
    p.ui->stereoscopy->setItemText(idx + 1, to_qs(stereo_mode_c::translate(idx)));

  Util::fixComboBoxViewWidth(*p.ui->stereoscopy);

  for (auto &comboBox : p.comboBoxControls)
    if (comboBox->count() && !comboBox->itemData(0).isValid())
      comboBox->setItemText(0, QY("<Do not change>"));

  for (auto &comboBox : std::vector<QComboBox *>{p.ui->muxThis, p.ui->forcedTrackFlag, p.ui->trackEnabledFlag, p.ui->hearingImpairedFlag, p.ui->visualImpairedFlag, p.ui->textDescriptionsFlag, p.ui->originalFlag, p.ui->commentaryFlag})
    Util::setComboBoxTexts(comboBox, QStringList{} << QY("Yes") << QY("No"));

  Util::setComboBoxTexts(p.ui->defaultTrackFlag, QStringList{}                                  << QY("Yes")                  << QY("No"));
  Util::setComboBoxTexts(p.ui->compression,      QStringList{} << QY("Determine automatically") << QY("No extra compression") << Q("zlib"));
  Util::setComboBoxTexts(p.ui->cues,             QStringList{} << QY("Determine automatically") << QY("Only for I frames")    << QY("For all frames") << QY("No cues"));
  Util::setComboBoxTexts(p.ui->aacIsSBR,         QStringList{} << QY("Determine automatically") << QY("Yes")                  << QY("No"));

  setupInputToolTips();
}

QModelIndex
Tab::selectedSourceFile()
  const {
  auto &p  = *p_func();
  auto idx = p.ui->files->selectionModel()->currentIndex();
  return p.filesModel->index(idx.row(), 0, idx.parent());
}

void
Tab::setTitleMaybe(QVector<SourceFilePtr> const &files) {
  for (auto const &file : files) {
    setTitleMaybe(file->m_properties.value("title").toString());

    if (mtx::file_type_e::ogm != file->m_type)
      continue;

    for (auto const &track : file->m_tracks)
      if (track->isVideo() && !track->m_name.isEmpty()) {
        setTitleMaybe(track->m_name);
        break;
      }
  }
}

void
Tab::setTitleMaybe(QString const &title) {
  auto &p = *p_func();

  if (!Util::Settings::get().m_autoSetFileTitle || title.isEmpty() || !p.config.m_title.isEmpty())
    return;

  p.ui->title->setText(title);
  p.config.m_title = title;
}

QString
Tab::suggestOutputFileNameExtension()
  const {
  auto &p        = *p_func();
  auto hasTracks = false, hasVideo = false, hasAudio = false, hasStereoscopy = false;

  for (auto const &t : p.config.m_tracks) {
    if (!t->m_muxThis)
      continue;

    hasTracks = true;

    if (t->isVideo()) {
      hasVideo = true;
      if (t->m_stereoscopy >= 2)
        hasStereoscopy = true;

    } else if (t->isAudio())
      hasAudio = true;
  }

  return p.config.m_webmMode ? "webm"
       : hasStereoscopy      ? "mk3d"
       : hasVideo            ? "mkv"
       : hasAudio            ? "mka"
       : hasTracks           ? "mks"
       :                       "mkv";
}

void
Tab::setOutputFileNameMaybe(bool force) {
  auto &p        = *p_func();
  auto &settings = Util::Settings::get();
  auto policy    = settings.m_outputFileNamePolicy;

  if (   !force
      && (   (Util::Settings::DontSetOutputFileName == policy)
          || p.config.m_firstInputFileName.isEmpty()))
    return;

  auto currentOutput = p.ui->output->text();
  QDir outputDir;

  // Don't override custom changes to the destination file name.
  if (   !force
      && !currentOutput.isEmpty()
      && !p.config.m_destinationAuto.isEmpty()
      && (QDir::toNativeSeparators(currentOutput) != QDir::toNativeSeparators(p.config.m_destinationAuto)))
    return;

  if (Util::Settings::ToPreviousDirectory == policy)
    outputDir = settings.m_lastOutputDir;

  else if (Util::Settings::ToFixedDirectory == policy)
    outputDir = settings.m_fixedOutputDir;

  else if (Util::Settings::ToRelativeOfFirstInputFile == policy)
    outputDir = QDir{ QFileInfo{p.config.m_firstInputFileName}.absoluteDir().path() + Q("/") + settings.m_relativeOutputDir.path() };

  else if (   (Util::Settings::ToSameAsFirstInputFile == policy)
           || force)
    outputDir = QFileInfo{p.config.m_firstInputFileName}.absoluteDir();

  else
    Q_ASSERT_X(false, "setOutputFileNameMaybe", "Untested destination file name policy");

  auto cleanedTitle          = Util::replaceInvalidFileNameCharacters(p.config.m_title);
  auto firstInputBaseName    = QFileInfo{ p.config.m_firstInputFileName }.completeBaseName();
  auto baseName              = !settings.m_mergeSetDestinationFromTitle ? firstInputBaseName
                             : !cleanedTitle.isEmpty()                  ? cleanedTitle
                             :                                            firstInputBaseName;
  p.config.m_destinationAuto = generateUniqueOutputFileName(baseName, outputDir);

  p.ui->output->setText(p.config.m_destinationAuto);
  setDestination(p.config.m_destinationAuto);
}

QString
Tab::generateUniqueOutputFileName(QString const &baseName,
                                  QDir const &outputDir,
                                  bool removeUniquenessSuffix) {
  auto &p              = *p_func();
  auto &settings       = Util::Settings::get();
  auto cleanedBaseName = baseName;
  auto suffix          = suggestOutputFileNameExtension();
  auto needToRemove    =    removeUniquenessSuffix
                         && !p.config.m_destinationUniquenessSuffix.isEmpty()
                         && cleanedBaseName.endsWith(p.config.m_destinationUniquenessSuffix);

  qDebug() << "generateUniqueOutputFileName: baseName" << baseName << "suffix" << suffix << "destinationUniquenessSuffix" << p.config.m_destinationUniquenessSuffix
           << "removeUniquenessSuffix" << removeUniquenessSuffix << "needToRemove" << needToRemove;

  if (needToRemove)
    cleanedBaseName.remove(cleanedBaseName.length() - p.config.m_destinationUniquenessSuffix.length(), p.config.m_destinationUniquenessSuffix.length());

  auto idx = 0;

  while (true) {
    auto uniquenessSuffix = idx ? QString{" (%1)"}.arg(idx) : QString{};
    auto currentBaseName  = QString{"%1%2.%3"}.arg(cleanedBaseName).arg(uniquenessSuffix).arg(suffix);
    currentBaseName       = Util::removeInvalidPathCharacters(currentBaseName);
    auto outputFileName   = QFileInfo{outputDir, currentBaseName};

    if (!settings.m_uniqueOutputFileNames || !outputFileName.exists()) {
      p.config.m_destinationUniquenessSuffix = uniquenessSuffix;
      return QDir::toNativeSeparators(outputFileName.absoluteFilePath());
    }

    ++idx;
  }
}

void
Tab::setDestinationFileNameFromSelectedFile() {
  auto &p            = *p_func();
  auto selectedFiles = selectedSourceFiles();
  if (selectedFiles.isEmpty())
    return;

  auto selectedFileName = selectedFiles[0]->m_fileName;

  if (!p.config.m_destination.isEmpty()) {
    p.config.m_destinationUniquenessSuffix.clear();

    auto baseName       = QFileInfo{selectedFileName}.completeBaseName();
    auto destinationDir = QDir{ QFileInfo{ p.config.m_destination }.path() };
    auto newFileName    = generateUniqueOutputFileName(baseName, destinationDir);

    p.ui->output->setText(QDir::toNativeSeparators(newFileName));

    return;
  }

  p.config.m_destinationAuto.clear();
  p.config.m_firstInputFileName = QDir::toNativeSeparators(selectedFileName);

  setOutputFileNameMaybe(true);
}

void
Tab::selectAllTracksOfType(std::optional<TrackType> type) {
  auto &p      = *p_func();
  auto numRows = p.tracksModel->rowCount();
  if (!numRows)
    return;

  auto lastColumn = p.tracksModel->columnCount() - 1;
  auto selection  = QItemSelection{};

  for (auto row = 0; row < numRows; ++row) {
    auto &track      = *p.config.m_tracks[row];
    auto numAppended = track.m_appendedTracks.count();

    if (!type || (track.m_type == *type))
      selection.select(p.tracksModel->index(row, 0), p.tracksModel->index(row, lastColumn));

    if (!numAppended)
      continue;

    auto rowIdx = p.tracksModel->index(row, 0);

    for (auto appendedRow = 0; appendedRow < numAppended; ++appendedRow) {
      auto &appendedTrack = *track.m_appendedTracks[appendedRow];
      if (!type || (appendedTrack.m_type == *type))
        selection.select(p.tracksModel->index(appendedRow, 0, rowIdx), p.tracksModel->index(appendedRow, lastColumn, rowIdx));
    }
  }

  p.ui->tracks->selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect);
}

void
Tab::selectAllTracks() {
  selectAllTracksOfType({});
}

void
Tab::selectAllVideoTracks() {
  selectAllTracksOfType(TrackType::Video);
}

void
Tab::selectAllAudioTracks() {
  selectAllTracksOfType(TrackType::Audio);
}

void
Tab::selectAllSubtitlesTracks() {
  selectAllTracksOfType(TrackType::Subtitles);
}

void
Tab::selectAllTracksFromSelectedFiles() {
  if (!p_func()->tracksModel->rowCount())
    return;

  auto tracksToSelect = QList<Track *>{};
  for (auto const &sourceFile : selectedSourceFiles())
    for (auto const &track : sourceFile->m_tracks)
      tracksToSelect << track.get();

  selectTracks(tracksToSelect);
}

void
Tab::selectTracks(QList<Track *> const &tracks) {
  auto &p         = *p_func();
  auto numColumns = p.tracksModel->columnCount() - 1;
  auto selection  = QItemSelection{};

  for (auto const &track : tracks) {
    auto idx = p.tracksModel->indexFromTrack(track);
    selection.select(idx.sibling(idx.row(), 0), idx.sibling(idx.row(), numColumns));
  }

  p.ui->tracks->selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect);
}

void
Tab::selectSourceFiles(QList<SourceFile *> const &files) {
  auto &p         = *p_func();
  auto numColumns = p.filesModel->columnCount() - 1;
  auto selection  = QItemSelection{};

  for (auto const &file : files) {
    auto idx = p.filesModel->indexFromSourceFile(file);
    selection.select(idx.sibling(idx.row(), 0), idx.sibling(idx.row(), numColumns));
  }

  p.ui->files->selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect);
}

void
Tab::enableAllTracks() {
  enableDisableAllTracks(true);
}

void
Tab::disableAllTracks() {
  enableDisableAllTracks(false);
}

void
Tab::enableDisableAllTracks(bool enable) {
  auto &p = *p_func();

  for (auto row = 0, numRows = p.tracksModel->rowCount(); row < numRows; ++row) {
    auto track       = p.tracksModel->fromIndex(p.tracksModel->index(row, 0));
    track->m_muxThis = enable;
    p.tracksModel->trackUpdated(track);

    for (auto const &appendedTrack : track->m_appendedTracks) {
      appendedTrack->m_muxThis = enable;
      p.tracksModel->trackUpdated(appendedTrack);
    }
  }

  auto base = p.ui->muxThis->itemData(0).isValid() ? 0 : 1;
  p.ui->muxThis->setCurrentIndex(base + (enable ? 0 : 1));
}

void
Tab::moveSourceFilesUpOrDown(bool up) {
  auto &p    = *p_func();
  auto focus = App::instance()->focusWidget();
  auto files = selectedSourceFiles();

  p.filesModel->moveSourceFilesUpOrDown(files, up);

  for (auto const &file : files)
    if (file->isRegular())
      p.ui->files->setExpanded(p.filesModel->indexFromSourceFile(file), true);

  selectSourceFiles(files);

  if (focus)
    focus->setFocus();

  p.ui->files->scrollToFirstSelected();
}

void
Tab::onMoveFilesUp() {
  moveSourceFilesUpOrDown(true);
}

void
Tab::onMoveFilesDown() {
  moveSourceFilesUpOrDown(false);
}

void
Tab::moveTracksUpOrDown(bool up) {
  auto &p     = *p_func();
  auto focus  = App::instance()->focusWidget();
  auto tracks = selectedTracks();

  p.tracksModel->moveTracksUpOrDown(tracks, up);

  for (auto const &track : tracks)
    if (track->isRegular() && !track->m_appendedTo)
      p.ui->tracks->setExpanded(p.tracksModel->indexFromTrack(track), true);

  selectTracks(tracks);

  if (focus)
    focus->setFocus();

  p.ui->tracks->scrollToFirstSelected();
}

void
Tab::onMoveTracksUp() {
  moveTracksUpOrDown(true);
}

void
Tab::onMoveTracksDown() {
  moveTracksUpOrDown(false);
}

void
Tab::showFilesContextMenu(QPoint const &pos) {
  auto &p = *p_func();

  enableFilesActions();
  p.filesMenu->exec(p.ui->files->viewport()->mapToGlobal(pos));
}

void
Tab::showTracksContextMenu(QPoint const &pos) {
  auto &p = *p_func();

  enableTracksActions();
  p.tracksMenu->exec(p.ui->tracks->viewport()->mapToGlobal(pos));
}

void
Tab::onOpenFilesInMediaInfo() {
  auto fileNames = QStringList{};
  for (auto const &sourceFile : selectedSourceFiles())
    fileNames << sourceFile->m_fileName;

  openFilesInMediaInfo(fileNames);
}

void
Tab::onOpenTracksInMediaInfo() {
  auto fileNames = QStringList{};
  for (auto const &track : selectedTracks()) {
    auto const &fileName = track->m_file->m_fileName;
    if (!fileNames.contains(fileName))
      fileNames << fileName;
  }

  openFilesInMediaInfo(fileNames);
}

QString
Tab::mediaInfoLocation() {
  auto &cfg = Util::Settings::get();
  auto exe  = Util::Settings::determineMediaInfoExePath();

  if (!exe.isEmpty() && QFileInfo{exe}.exists())
    return exe;

  ExecutableLocationDialog dlg{this};
  auto result = dlg
    .setInfo(QY("Executable not found"),
             Q("<p>%1 %2 %3</p><p>%4</p>")
             .arg(QYH("This function requires the application %1.").arg("MediaInfo"))
             .arg(QYH("Its installation location could not be determined automatically."))
             .arg(QYH("Please select its location below."))
             .arg(QYH("You can download the application from the following URL:")))
    .setURL(Q("https://mediaarea.net/en/MediaInfo"))
    .exec();

  if (QDialog::Rejected == result)
    return {};

  exe = dlg.executable();
  if (exe.isEmpty() || !QFileInfo{exe}.exists())
    return {};

  cfg.m_mediaInfoExe = exe;

  return exe;
}

void
Tab::openFilesInMediaInfo(QStringList const &fileNames) {
  if (fileNames.isEmpty())
    return;

  auto exe = mediaInfoLocation();
  if (!exe.isEmpty())
    QProcess::startDetached(exe, fileNames);
}

void
Tab::onPreviewSubtitleCharacterSet() {
  auto selection = selectedTracks();
  auto track     = selection.count() ? selection[0] : nullptr;

  if ((selection.count() != 1) || !track->m_file->isTextSubtitleContainer())
    return;

  auto additionalCharacterSets = QSet<QString>{};

  for (auto const &sourceFile : p_func()->config.m_files)
    for (auto const &sourceTrack : sourceFile->m_tracks)
      additionalCharacterSets << sourceTrack->m_characterSet;

  auto dlg = new SelectCharacterSetDialog{this, track->m_file->m_fileName, track->m_characterSet, additionalCharacterSets.values()};
  dlg->setUserData(reinterpret_cast<qulonglong>(track));

  connect(dlg, &SelectCharacterSetDialog::characterSetSelected, this, &Tab::setSubtitleCharacterSet);

  dlg->show();
}

void
Tab::setSubtitleCharacterSet(QString const &characterSet) {
  auto &p  = *p_func();
  auto dlg = qobject_cast<SelectCharacterSetDialog *>(QObject::sender());
  if (!dlg)
    return;

  auto track = reinterpret_cast<Track *>(dlg->userData().toULongLong());

  if (!p.config.m_tracks.contains(track))
    return;

  track->m_characterSet = characterSet;
  auto selection        = selectedTracks();

  if ((selection.count() == 1) && (selection[0] == track))
    Util::setComboBoxTextByData(p.ui->subtitleCharacterSet, characterSet);
}

bool
Tab::hasSourceFiles()
  const {
  return !p_func()->config.m_files.isEmpty();
}

void
Tab::toggleSpecificTrackFlag(unsigned int wantedId) {
  auto &p       = *p_func();
  auto comboBox = wantedId == libmatroska::KaxTrackFlagDefault    ::ClassInfos.GlobalId.GetValue() ? p.ui->defaultTrackFlag
                : wantedId == libmatroska::KaxTrackFlagForced     ::ClassInfos.GlobalId.GetValue() ? p.ui->forcedTrackFlag
                : wantedId == libmatroska::KaxTrackFlagEnabled    ::ClassInfos.GlobalId.GetValue() ? p.ui->trackEnabledFlag
                : wantedId == libmatroska::KaxFlagCommentary      ::ClassInfos.GlobalId.GetValue() ? p.ui->commentaryFlag
                : wantedId == libmatroska::KaxFlagOriginal        ::ClassInfos.GlobalId.GetValue() ? p.ui->originalFlag
                : wantedId == libmatroska::KaxFlagHearingImpaired ::ClassInfos.GlobalId.GetValue() ? p.ui->hearingImpairedFlag
                : wantedId == libmatroska::KaxFlagVisualImpaired  ::ClassInfos.GlobalId.GetValue() ? p.ui->visualImpairedFlag
                : wantedId == libmatroska::KaxFlagTextDescriptions::ClassInfos.GlobalId.GetValue() ? p.ui->textDescriptionsFlag
                :                                                                                    static_cast<QComboBox *>(nullptr);

  if (!comboBox || !comboBox->isEnabled())
    return;

  auto newIndex = (comboBox->currentIndex() + 1) % comboBox->count();
  if (!comboBox->itemData(newIndex).isValid())
    ++newIndex;

  comboBox->setCurrentIndex(newIndex);
}

void
Tab::changeTrackLanguage(QString const &formattedLanguage,
                         QString const &trackName) {
  auto &p = *p_func();

  auto language = mtx::bcp47::language_c::parse(to_utf8(formattedLanguage));

  if (!language.is_valid() || !p.ui->trackLanguage->isEnabled())
    return;

  p.ui->trackLanguage->setLanguage(language);
  onTrackLanguageChanged(language);

  if (trackName.isEmpty() || !p.ui->trackName->isEnabled())
    return;

  p.ui->trackName->setCurrentText(trackName);
  onTrackNameChanged(trackName);
}

}
