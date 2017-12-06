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
#include <QProgressDialog>
#include <QRegularExpression>
#include <QSettings>
#include <QString>
#include <QThread>
#include <QTimer>

#include "common/extern_data.h"
#include "common/iso639.h"
#include "common/logger.h"
#include "common/qt.h"
#include "common/stereo_mode.h"
#include "mkvtoolnix-gui/app.h"
#include "mkvtoolnix-gui/forms/merge/tab.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/main_window/select_character_set_dialog.h"
#include "mkvtoolnix-gui/merge/adding_appending_files_dialog.h"
#include "mkvtoolnix-gui/merge/ask_scan_for_playlists_dialog.h"
#include "mkvtoolnix-gui/merge/executable_location_dialog.h"
#include "mkvtoolnix-gui/merge/file_identification_thread.h"
#include "mkvtoolnix-gui/merge/select_playlist_dialog.h"
#include "mkvtoolnix-gui/merge/tab.h"
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

namespace mtx { namespace gui { namespace Merge {

using namespace mtx::gui;

void
Tab::setupControlLists() {
  m_typeIndependentControls << ui->generalOptionsBox << ui->muxThisLabel << ui->muxThis << ui->miscellaneousBox << ui->additionalTrackOptionsLabel << ui->additionalTrackOptions;

  m_audioControls << ui->trackNameLabel << ui->trackName << ui->trackLanguageLabel << ui->trackLanguage << ui->defaultTrackFlagLabel << ui->defaultTrackFlag << ui->forcedTrackFlagLabel << ui->forcedTrackFlag
                  << ui->compressionLabel << ui->compression << ui->trackTagsLabel << ui->trackTags << ui->browseTrackTags << ui->timestampsAndDefaultDurationBox
                  << ui->delayLabel << ui->delay << ui->stretchByLabel << ui->stretchBy << ui->timestampsLabel << ui->timestamps << ui->browseTimestamps << ui->audioPropertiesBox << ui->aacIsSBR << ui->aacIsSBRLabel
                  << ui->cuesLabel << ui->cues << ui->propertiesLabel << ui->generalOptionsBox << ui->reduceToAudioCore;

  m_videoControls << ui->trackNameLabel << ui->trackName << ui->trackLanguageLabel << ui->trackLanguage << ui->defaultTrackFlagLabel << ui->defaultTrackFlag
                  << ui->forcedTrackFlagLabel << ui->forcedTrackFlag << ui->compressionLabel << ui->compression << ui->trackTagsLabel << ui->trackTags << ui->browseTrackTags << ui->timestampsAndDefaultDurationBox
                  << ui->delayLabel << ui->delay << ui->stretchByLabel << ui->stretchBy << ui->defaultDurationLabel << ui->defaultDuration << ui->timestampsLabel << ui->timestamps << ui->browseTimestamps
                  << ui->videoPropertiesBox << ui->setAspectRatio << ui->aspectRatio << ui->setDisplayWidthHeight << ui->displayWidth << ui->displayDimensionsXLabel << ui->displayHeight << ui->stereoscopyLabel
                  << ui->stereoscopy << ui->naluSizeLengthLabel << ui->naluSizeLength << ui->croppingLabel << ui->cropping << ui->cuesLabel << ui->cues
                  << ui->propertiesLabel << ui->generalOptionsBox << ui->fixBitstreamTimingInfo;

  m_subtitleControls << ui->trackNameLabel << ui->trackName << ui->trackLanguageLabel << ui->trackLanguage << ui->defaultTrackFlagLabel << ui->defaultTrackFlag
                     << ui->forcedTrackFlagLabel << ui->forcedTrackFlag << ui->compressionLabel << ui->compression << ui->trackTagsLabel << ui->trackTags << ui->browseTrackTags << ui->timestampsAndDefaultDurationBox
                     << ui->delayLabel << ui->delay << ui->stretchByLabel << ui->stretchBy << ui->timestampsLabel << ui->timestamps << ui->browseTimestamps
                     << ui->subtitleAndChapterPropertiesBox << ui->characterSetLabel << ui->subtitleCharacterSet << ui->cuesLabel << ui->cues
                     << ui->propertiesLabel << ui->generalOptionsBox;

  m_chapterControls << ui->subtitleAndChapterPropertiesBox << ui->characterSetLabel << ui->subtitleCharacterSet << ui->propertiesLabel << ui->generalOptionsBox;

  m_allInputControls << ui->muxThisLabel << ui->muxThis << ui->trackNameLabel << ui->trackName << ui->trackLanguageLabel << ui->trackLanguage << ui->defaultTrackFlagLabel << ui->defaultTrackFlag
                     << ui->forcedTrackFlagLabel << ui->forcedTrackFlag << ui->compressionLabel << ui->compression << ui->trackTagsLabel << ui->trackTags << ui->browseTrackTags << ui->timestampsAndDefaultDurationBox
                     << ui->delayLabel << ui->delay << ui->stretchByLabel << ui->stretchBy << ui->defaultDurationLabel << ui->defaultDuration << ui->timestampsLabel << ui->timestamps << ui->browseTimestamps
                     << ui->videoPropertiesBox << ui->setAspectRatio << ui->aspectRatio << ui->setDisplayWidthHeight << ui->displayWidth << ui->displayDimensionsXLabel << ui->displayHeight << ui->stereoscopyLabel
                     << ui->stereoscopy << ui->croppingLabel << ui->cropping << ui->audioPropertiesBox << ui->aacIsSBR << ui->subtitleAndChapterPropertiesBox << ui->characterSetLabel << ui->subtitleCharacterSet
                     << ui->miscellaneousBox << ui->cuesLabel << ui->cues << ui->additionalTrackOptionsLabel << ui->additionalTrackOptions
                     << ui->propertiesLabel << ui->generalOptionsBox << ui->fixBitstreamTimingInfo << ui->reduceToAudioCore << ui->naluSizeLengthLabel << ui->naluSizeLength;

  m_comboBoxControls << ui->muxThis << ui->trackLanguage << ui->defaultTrackFlag << ui->forcedTrackFlag << ui->compression << ui->cues << ui->stereoscopy << ui->naluSizeLength << ui->aacIsSBR << ui->subtitleCharacterSet;

  m_notIfAppendingControls << ui->trackLanguageLabel   << ui->trackLanguage           << ui->trackNameLabel              << ui->trackName        << ui->defaultTrackFlagLabel << ui->defaultTrackFlag
                           << ui->forcedTrackFlagLabel << ui->forcedTrackFlag         << ui->compressionLabel            << ui->compression      << ui->trackTagsLabel        << ui->trackTags         << ui->browseTrackTags
                           << ui->defaultDurationLabel << ui->defaultDuration         << ui->fixBitstreamTimingInfo      << ui->setAspectRatio   << ui->setDisplayWidthHeight << ui->aspectRatio
                           << ui->displayWidth         << ui->displayDimensionsXLabel << ui->displayHeight               << ui->stereoscopyLabel << ui->stereoscopy
                           << ui->naluSizeLengthLabel  << ui->naluSizeLength          << ui->croppingLabel               << ui->cropping         << ui->aacIsSBR              << ui->characterSetLabel << ui->subtitleCharacterSet
                           << ui->cuesLabel            << ui->cues                    << ui->additionalTrackOptionsLabel << ui->additionalTrackOptions;
}

void
Tab::setupMoveUpDownButtons() {
  auto show    = Util::Settings::get().m_showMoveUpDownButtons;
  auto widgets = QList<QWidget *>{} << ui->moveFilesButtons << ui->moveTracksButtons << ui->moveAttachmentsButtons;

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
  if (ui->wProperties->isVisible() && (0 == ui->propertiesStack->currentIndex()))
    return;

  ui->twProperties->hide();
  ui->wProperties->show();

  auto layout = qobject_cast<QBoxLayout *>(ui->scrollAreaWidgetContents->layout());

  Q_ASSERT(!!layout);

  auto widgets = QWidgetList{} << ui->generalOptionsBox << ui->timestampsAndDefaultDurationBox << ui->videoPropertiesBox << ui->audioPropertiesBox << ui->subtitleAndChapterPropertiesBox << ui->miscellaneousBox;
  for (auto const &widget : widgets)
    widget->setParent(ui->scrollAreaWidgetContents);

  layout->insertWidget(0, ui->generalOptionsBox);
  layout->insertWidget(1, ui->timestampsAndDefaultDurationBox);
  layout->insertWidget(2, ui->videoPropertiesBox);
  layout->insertWidget(3, ui->audioPropertiesBox);
  layout->insertWidget(4, ui->subtitleAndChapterPropertiesBox);
  layout->insertWidget(5, ui->miscellaneousBox);

  ui->propertiesColumn1->updateGeometry();
  ui->propertiesColumn2->updateGeometry();

  ui->propertiesStack->setCurrentIndex(0);
}

void
Tab::setupHorizontalTwoColumnsInputLayout() {
  if (ui->wProperties->isVisible() && (1 == ui->propertiesStack->currentIndex()))
    return;

  ui->twProperties->hide();
  ui->wProperties->show();
  ui->propertiesStack->setCurrentIndex(1);

  auto moveTo = [](QWidget *column, int position, QWidget *widget) {
    auto layout = qobject_cast<QBoxLayout *>(column->layout());
    Q_ASSERT(!!layout);

    widget->setParent(column);
    layout->insertWidget(position, widget);
  };

  moveTo(ui->propertiesColumn1, 0, ui->generalOptionsBox);
  moveTo(ui->propertiesColumn1, 1, ui->timestampsAndDefaultDurationBox);
  moveTo(ui->propertiesColumn2, 0, ui->videoPropertiesBox);
  moveTo(ui->propertiesColumn2, 1, ui->audioPropertiesBox);
  moveTo(ui->propertiesColumn2, 2, ui->subtitleAndChapterPropertiesBox);
  moveTo(ui->propertiesColumn2, 3, ui->miscellaneousBox);
}

void
Tab::setupVerticalTabWidgetInputLayout() {
  if (ui->twProperties->isVisible())
    return;

  ui->twProperties->show();
  ui->wProperties->hide();

  auto moveTo = [](QWidget *page, int position, QWidget *widget) {
    auto layout = qobject_cast<QBoxLayout *>(page->layout());
    Q_ASSERT(!!layout);

    widget->setParent(page);
    layout->insertWidget(position, widget);
  };

  moveTo(ui->generalOptionsPage,                 0, ui->generalOptionsBox);
  moveTo(ui->timestampsAndDefaultDurationPage,   0, ui->timestampsAndDefaultDurationBox);
  moveTo(ui->videoPropertiesPage,                0, ui->videoPropertiesBox);
  moveTo(ui->audioSubtitleChapterPropertiesPage, 0, ui->audioPropertiesBox);
  moveTo(ui->audioSubtitleChapterPropertiesPage, 1, ui->subtitleAndChapterPropertiesBox);
  moveTo(ui->miscellaneousPage,                  0, ui->miscellaneousBox);

  ui->propertiesColumn1->updateGeometry();
  ui->propertiesColumn2->updateGeometry();
}

void
Tab::setupInputControls() {
  auto &cfg = Util::Settings::get();

  ui->twProperties->hide();

  setupControlLists();
  setupMoveUpDownButtons();
  setupInputLayout();

  ui->files->setModel(m_filesModel);
  ui->tracks->setModel(m_tracksModel);
  ui->tracks->enterActivatesAllSelected(true);

  cfg.handleSplitterSizes(ui->mergeInputSplitter);
  cfg.handleSplitterSizes(ui->mergeFilesTracksSplitter);

  // Track & chapter language
  ui->trackLanguage->setup();
  ui->chapterLanguage->setup(true);

  // Track & chapter character set
  ui->subtitleCharacterSet->setup(true);
  ui->chapterCharacterSet->setup(true);

  ui->muxThis->addItem(QString{}, true);
  ui->muxThis->addItem(QString{}, false);

  // Stereoscopy
  ui->stereoscopy->addItem(Q(""), 0);
  for (auto idx = 0u, end = stereo_mode_c::max_index(); idx <= end; ++idx)
    ui->stereoscopy->addItem(QString{}, idx + 1);

  // NALU size length
  for (auto idx = 0; idx < 3; ++idx)
    ui->naluSizeLength->addItem(QString{}, idx * 2);

  for (auto idx = 0; idx < 3; ++idx)
    ui->defaultTrackFlag->addItem(QString{}, idx);

  // Originally the "forced track" flag's options where ordered "off,
  // on"; now they're ordered "yes, no" for consistency with other
  // flags, requiring the values to be reversed.
  ui->forcedTrackFlag->addItem(QString{}, 1);
  ui->forcedTrackFlag->addItem(QString{}, 0);
  ui->forcedTrackFlag->setCurrentIndex(1);

  for (auto idx = 0; idx < 3; ++idx)
    ui->compression->addItem(QString{}, idx);

  for (auto idx = 0; idx < 4; ++idx)
    ui->cues->addItem(QString{}, idx);

  for (auto idx = 0; idx < 3; ++idx)
    ui->aacIsSBR->addItem(QString{}, idx);

  for (auto const &control : m_comboBoxControls) {
    control->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    Util::fixComboBoxViewWidth(*control);
  }

  // "files" context menu
  m_filesMenu->addAction(m_addFilesAction);
  m_filesMenu->addAction(m_appendFilesAction);
  m_filesMenu->addAction(m_addAdditionalPartsAction);
  m_filesMenu->addSeparator();
  m_filesMenu->addAction(m_removeFilesAction);
  m_filesMenu->addAction(m_removeAllFilesAction);
  m_filesMenu->addSeparator();
  m_filesMenu->addAction(m_setDestinationFileNameAction);
  m_filesMenu->addSeparator();
  m_filesMenu->addAction(m_openFilesInMediaInfoAction);
  m_filesMenu->addSeparator();
  m_filesMenu->addAction(m_selectTracksFromFilesAction);

  m_addFilesAction->setIcon(QIcon{Q(":/icons/16x16/list-add.png")});
  m_appendFilesAction->setIcon(QIcon{Q(":/icons/16x16/distribute-horizontal-x.png")});
  m_addAdditionalPartsAction->setIcon(QIcon{Q(":/icons/16x16/distribute-horizontal-margin.png")});
  m_removeFilesAction->setIcon(QIcon{Q(":/icons/16x16/list-remove.png")});
  m_openFilesInMediaInfoAction->setIcon(QIcon{Q(":/icons/16x16/documentinfo.png")});

  // "tracks" context menu
  m_tracksMenu->addAction(m_selectAllTracksAction);
  m_tracksMenu->addMenu(m_selectTracksOfTypeMenu);
  m_tracksMenu->addAction(m_enableAllTracksAction);
  m_tracksMenu->addAction(m_disableAllTracksAction);
  m_tracksMenu->addSeparator();
  m_tracksMenu->addAction(m_openTracksInMediaInfoAction);

  m_selectTracksOfTypeMenu->addAction(m_selectAllVideoTracksAction);
  m_selectTracksOfTypeMenu->addAction(m_selectAllAudioTracksAction);
  m_selectTracksOfTypeMenu->addAction(m_selectAllSubtitlesTracksAction);

  m_selectAllTracksAction->setIcon(QIcon{Q(":/icons/16x16/edit-select-all.png")});
  m_enableAllTracksAction->setIcon(QIcon{Q(":/icons/16x16/checkbox.png")});
  m_disableAllTracksAction->setIcon(QIcon{Q(":/icons/16x16/checkbox-unchecked.png")});
  m_openTracksInMediaInfoAction->setIcon(QIcon{Q(":/icons/16x16/documentinfo.png")});

  m_selectAllVideoTracksAction->setIcon(QIcon{Q(":/icons/16x16/tool-animator.png")});
  m_selectAllAudioTracksAction->setIcon(QIcon{Q(":/icons/16x16/knotify.png")});
  m_selectAllSubtitlesTracksAction->setIcon(QIcon{Q(":/icons/16x16/subtitles.png")});

  // "add source files" menu
  m_addFilesMenu->addAction(m_addFilesAction2);
  m_addFilesMenu->addAction(m_appendFilesAction2);
  m_addFilesMenu->addAction(m_addAdditionalPartsAction2);
  ui->addFiles->setMenu(m_addFilesMenu);

  // "start muxing" menu
  m_startMuxingMenu->addAction(m_startMuxingLeaveAsIs);
  m_startMuxingMenu->addAction(m_startMuxingCreateNewSettings);
  m_startMuxingMenu->addAction(m_startMuxingRemoveInputFiles);
  m_startMuxingMenu->addAction(m_startMuxingCloseSettings);
  ui->startMuxing->setMenu(m_startMuxingMenu);

  // "add to job queue" menu
  m_addToJobQueueMenu->addAction(m_addToJobQueueLeaveAsIs);
  m_addToJobQueueMenu->addAction(m_addToJobQueueCreateNewSettings);
  m_addToJobQueueMenu->addAction(m_addToJobQueueRemoveInputFiles);
  m_addToJobQueueMenu->addAction(m_addToJobQueueCloseSettings);
  ui->addToJobQueue->setMenu(m_addToJobQueueMenu);

  m_addFilesAction2->setIcon(QIcon{Q(":/icons/16x16/list-add.png")});
  m_appendFilesAction2->setIcon(QIcon{Q(":/icons/16x16/distribute-horizontal-x.png")});
  m_addAdditionalPartsAction2->setIcon(QIcon{Q(":/icons/16x16/distribute-horizontal-margin.png")});

  // Connect signals & slots.
  auto mw = MainWindow::get();
  using CMSAction = Util::Settings::ClearMergeSettingsAction;

  connect(ui->aacIsSBR,                     static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),                           this,                     &Tab::onAacIsSBRChanged);
  connect(ui->addFiles,                     &QToolButton::clicked,                                                                            this,                     &Tab::onAddFiles);
  connect(ui->addToJobQueue,                &QPushButton::clicked,                                                                            this,                     [=]() { addToJobQueue(false); });
  connect(ui->additionalTrackOptions,       &QLineEdit::textChanged,                                                                          this,                     &Tab::onAdditionalTrackOptionsChanged);
  connect(ui->aspectRatio,                  static_cast<void (QComboBox::*)(QString const &)>(&QComboBox::currentIndexChanged),               this,                     &Tab::onAspectRatioChanged);
  connect(ui->aspectRatio,                  &QComboBox::editTextChanged,                                                                      this,                     &Tab::onAspectRatioChanged);
  connect(ui->browseTimestamps,             &QPushButton::clicked,                                                                            this,                     &Tab::onBrowseTimestamps);
  connect(ui->browseTrackTags,              &QPushButton::clicked,                                                                            this,                     &Tab::onBrowseTrackTags);
  connect(ui->compression,                  static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),                           this,                     &Tab::onCompressionChanged);
  connect(ui->cropping,                     &QLineEdit::textChanged,                                                                          this,                     &Tab::onCroppingChanged);
  connect(ui->cues,                         static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),                           this,                     &Tab::onCuesChanged);
  connect(ui->defaultDuration,              static_cast<void (QComboBox::*)(QString const &)>(&QComboBox::currentIndexChanged),               this,                     &Tab::onDefaultDurationChanged);
  connect(ui->defaultDuration,              &QComboBox::editTextChanged,                                                                      this,                     &Tab::onDefaultDurationChanged);
  connect(ui->defaultTrackFlag,             static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),                           this,                     &Tab::onDefaultTrackFlagChanged);
  connect(ui->delay,                        &QLineEdit::textChanged,                                                                          this,                     &Tab::onDelayChanged);
  connect(ui->displayHeight,                &QLineEdit::textChanged,                                                                          this,                     &Tab::onDisplayHeightChanged);
  connect(ui->displayWidth,                 &QLineEdit::textChanged,                                                                          this,                     &Tab::onDisplayWidthChanged);
  connect(ui->editAdditionalOptions,        &QPushButton::clicked,                                                                            this,                     &Tab::onEditAdditionalOptions);
  connect(ui->files,                        &Util::BasicTreeView::ctrlDownPressed,                                                            this,                     &Tab::onMoveFilesDown);
  connect(ui->files,                        &Util::BasicTreeView::ctrlUpPressed,                                                              this,                     &Tab::onMoveFilesUp);
  connect(ui->files,                        &Util::BasicTreeView::customContextMenuRequested,                                                 this,                     &Tab::showFilesContextMenu);
  connect(ui->files,                        &Util::BasicTreeView::deletePressed,                                                              this,                     &Tab::onRemoveFiles);
  connect(ui->files,                        &Util::BasicTreeView::insertPressed,                                                              this,                     &Tab::onAddFiles);
  connect(ui->files,                        &Util::BasicTreeView::filesDropped,                                                               this,                     &Tab::addOrAppendDroppedFiles);
  connect(ui->files->selectionModel(),      &QItemSelectionModel::selectionChanged,                                                           m_filesModel,             &SourceFileModel::updateSelectionStatus);
  connect(ui->files->selectionModel(),      &QItemSelectionModel::selectionChanged,                                                           this,                     &Tab::enableMoveFilesButtons);
  connect(ui->fixBitstreamTimingInfo,       &QCheckBox::toggled,                                                                              this,                     &Tab::onFixBitstreamTimingInfoChanged);
  connect(ui->forcedTrackFlag,              static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),                           this,                     &Tab::onForcedTrackFlagChanged);
  connect(ui->moveFilesDown,                &QPushButton::clicked,                                                                            this,                     &Tab::onMoveFilesDown);
  connect(ui->moveFilesUp,                  &QPushButton::clicked,                                                                            this,                     &Tab::onMoveFilesUp);
  connect(ui->moveTracksDown,               &QPushButton::clicked,                                                                            this,                     &Tab::onMoveTracksDown);
  connect(ui->moveTracksUp,                 &QPushButton::clicked,                                                                            this,                     &Tab::onMoveTracksUp);
  connect(ui->muxThis,                      static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),                           this,                     &Tab::onMuxThisChanged);
  connect(ui->naluSizeLength,               static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),                           this,                     &Tab::onNaluSizeLengthChanged);
  connect(ui->reduceToAudioCore,            &QCheckBox::toggled,                                                                              this,                     &Tab::onReduceAudioToCoreChanged);
  connect(ui->setAspectRatio,               &QPushButton::clicked,                                                                            this,                     &Tab::onSetAspectRatio);
  connect(ui->setDisplayWidthHeight,        &QPushButton::clicked,                                                                            this,                     &Tab::onSetDisplayDimensions);
  connect(ui->startMuxing,                  &QPushButton::clicked,                                                                            this,                     [=]() { addToJobQueue(true); });
  connect(ui->stereoscopy,                  static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),                           this,                     &Tab::onStereoscopyChanged);
  connect(ui->stretchBy,                    &QLineEdit::textChanged,                                                                          this,                     &Tab::onStretchByChanged);
  connect(ui->subtitleCharacterSet,         static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),                           this,                     &Tab::onSubtitleCharacterSetChanged);
  connect(ui->subtitleCharacterSetPreview,  &QPushButton::clicked,                                                                            this,                     &Tab::onPreviewSubtitleCharacterSet);
  connect(ui->timestamps,                   &QLineEdit::textChanged,                                                                          this,                     &Tab::onTimestampsChanged);
  connect(ui->trackLanguage,                static_cast<void (Util::LanguageComboBox::*)(int)>(&Util::LanguageComboBox::currentIndexChanged), this,                     &Tab::onTrackLanguageChanged);
  connect(ui->trackName,                    &QLineEdit::textChanged,                                                                          this,                     &Tab::onTrackNameChanged);
  connect(ui->trackTags,                    &QLineEdit::textChanged,                                                                          this,                     &Tab::onTrackTagsChanged);
  connect(ui->tracks,                       &QTreeView::doubleClicked,                                                                        this,                     &Tab::toggleMuxThisForSelectedTracks);
  connect(ui->tracks,                       &Util::BasicTreeView::allSelectedActivated,                                                       this,                     &Tab::toggleMuxThisForSelectedTracks);
  connect(ui->tracks,                       &Util::BasicTreeView::ctrlDownPressed,                                                            this,                     &Tab::onMoveTracksDown);
  connect(ui->tracks,                       &Util::BasicTreeView::ctrlUpPressed,                                                              this,                     &Tab::onMoveTracksUp);
  connect(ui->tracks,                       &Util::BasicTreeView::customContextMenuRequested,                                                 this,                     &Tab::showTracksContextMenu);
  connect(ui->tracks->selectionModel(),     &QItemSelectionModel::selectionChanged,                                                           m_tracksModel,            &TrackModel::updateSelectionStatus);
  connect(ui->tracks->selectionModel(),     &QItemSelectionModel::selectionChanged,                                                           this,                     &Tab::onTrackSelectionChanged);

  connect(m_addFilesAction,                 &QAction::triggered,                                                                              this,                     &Tab::onAddFiles);
  connect(m_addFilesAction2,                &QAction::triggered,                                                                              this,                     &Tab::onAddFiles);
  connect(m_appendFilesAction,              &QAction::triggered,                                                                              this,                     &Tab::onAppendFiles);
  connect(m_appendFilesAction2,             &QAction::triggered,                                                                              this,                     &Tab::onAppendFiles);
  connect(m_addAdditionalPartsAction,       &QAction::triggered,                                                                              this,                     &Tab::onAddAdditionalParts);
  connect(m_addAdditionalPartsAction2,      &QAction::triggered,                                                                              this,                     &Tab::onAddAdditionalParts);
  connect(m_removeFilesAction,              &QAction::triggered,                                                                              this,                     &Tab::onRemoveFiles);
  connect(m_removeAllFilesAction,           &QAction::triggered,                                                                              this,                     &Tab::onRemoveAllFiles);
  connect(m_setDestinationFileNameAction,   &QAction::triggered,                                                                              this,                     &Tab::setDestinationFileNameFromSelectedFile);
  connect(m_openFilesInMediaInfoAction,     &QAction::triggered,                                                                              this,                     &Tab::onOpenFilesInMediaInfo);
  connect(m_openTracksInMediaInfoAction,    &QAction::triggered,                                                                              this,                     &Tab::onOpenTracksInMediaInfo);

  connect(m_selectAllTracksAction,          &QAction::triggered,                                                                              this,                     &Tab::selectAllTracks);
  connect(m_selectAllVideoTracksAction,     &QAction::triggered,                                                                              this,                     &Tab::selectAllVideoTracks);
  connect(m_selectAllAudioTracksAction,     &QAction::triggered,                                                                              this,                     &Tab::selectAllAudioTracks);
  connect(m_selectAllSubtitlesTracksAction, &QAction::triggered,                                                                              this,                     &Tab::selectAllSubtitlesTracks);
  connect(m_selectTracksFromFilesAction,    &QAction::triggered,                                                                              this,                     &Tab::selectAllTracksFromSelectedFiles);
  connect(m_enableAllTracksAction,          &QAction::triggered,                                                                              this,                     &Tab::enableAllTracks);
  connect(m_disableAllTracksAction,         &QAction::triggered,                                                                              this,                     &Tab::disableAllTracks);

  connect(m_startMuxingLeaveAsIs,           &QAction::triggered,                                                                              this,                     [=]() { addToJobQueue(true,  CMSAction::None); });
  connect(m_startMuxingCreateNewSettings,   &QAction::triggered,                                                                              this,                     [=]() { addToJobQueue(true,  CMSAction::NewSettings); });
  connect(m_startMuxingCloseSettings,       &QAction::triggered,                                                                              this,                     [=]() { addToJobQueue(true,  CMSAction::CloseSettings); });
  connect(m_startMuxingRemoveInputFiles,    &QAction::triggered,                                                                              this,                     [=]() { addToJobQueue(true,  CMSAction::RemoveInputFiles); });

  connect(m_addToJobQueueLeaveAsIs,         &QAction::triggered,                                                                              this,                     [=]() { addToJobQueue(false, CMSAction::None); });
  connect(m_addToJobQueueCreateNewSettings, &QAction::triggered,                                                                              this,                     [=]() { addToJobQueue(false, CMSAction::NewSettings); });
  connect(m_addToJobQueueCloseSettings,     &QAction::triggered,                                                                              this,                     [=]() { addToJobQueue(false, CMSAction::CloseSettings); });
  connect(m_addToJobQueueRemoveInputFiles,  &QAction::triggered,                                                                              this,                     [=]() { addToJobQueue(false, CMSAction::RemoveInputFiles); });

  connect(m_addFilesMenu,                   &QMenu::aboutToShow,                                                                              this,                     &Tab::enableFilesActions);

  connect(m_filesModel,                     &SourceFileModel::rowsInserted,                                                                   this,                     &Tab::onFileRowsInserted);
  connect(m_tracksModel,                    &TrackModel::rowsInserted,                                                                        this,                     &Tab::onTrackRowsInserted);
  connect(m_tracksModel,                    &TrackModel::itemChanged,                                                                         this,                     &Tab::onTrackItemChanged);

  connect(mw,                               &MainWindow::preferencesChanged,                                                                  this,                     &Tab::setupMoveUpDownButtons);
  connect(mw,                               &MainWindow::preferencesChanged,                                                                  this,                     &Tab::setupInputLayout);
  connect(mw,                               &MainWindow::preferencesChanged,                                                                  ui->trackLanguage,        &Util::ComboBoxBase::reInitialize);
  connect(mw,                               &MainWindow::preferencesChanged,                                                                  ui->chapterLanguage,      &Util::ComboBoxBase::reInitialize);
  connect(mw,                               &MainWindow::preferencesChanged,                                                                  ui->subtitleCharacterSet, &Util::ComboBoxBase::reInitialize);
  connect(mw,                               &MainWindow::preferencesChanged,                                                                  ui->chapterCharacterSet,  &Util::ComboBoxBase::reInitialize);

  enableMoveFilesButtons();
  onTrackSelectionChanged();

  Util::HeaderViewManager::create(*ui->files,  "Merge::Files");
  Util::HeaderViewManager::create(*ui->tracks, "Merge::Tracks");
}

void
Tab::setupInputToolTips() {
  Util::setToolTip(ui->files,     QY("Right-click to add, append and remove files"));
  Util::setToolTip(ui->tracks,    QY("Right-click for actions for all items"));

  Util::setToolTip(ui->muxThis,   QY("If set to 'no' then the selected tracks will not be copied to the destination file."));
  Util::setToolTip(ui->trackName, QY("A name for this track that players can display helping the user chose the right track to play, e.g. \"director's comments\"."));
  Util::setToolTip(ui->trackLanguage,
                   Q("%1 %2")
                   .arg(QY("The language for this track that players can use for automatic track selection and display for the user."))
                   .arg(QY("Select one of the ISO639-2 language codes.")));
  Util::setToolTip(ui->defaultTrackFlag,
                   Q("%1 %2 %3")
                   .arg(QY("Make this track the default track for its type (audio, video, subtitles)."))
                   .arg(QY("Players should prefer tracks with the default track flag set."))
                   .arg(QY("If set to 'determine automatically' then mkvmerge will choose one track of each type to have this flag set based on the information in the source files and the order of the tracks.")));
  Util::setToolTip(ui->forcedTrackFlag,
                   Q("%1 %2")
                   .arg(QY("Mark this track as 'forced'."))
                   .arg(QY("Players must play this track.")));
  Util::setToolTip(ui->compression,
                   Q("%1 %2 %3")
                   .arg(QY("Sets the lossless compression algorithm to be used for this track."))
                   .arg(QY("If set to 'determine automatically' then mkvmerge will decide whether or not to compress and which algorithm to use based on the track type."))
                   .arg(QY("Currently only certain subtitle formats are compressed with the zlib algorithm.")));
  Util::setToolTip(ui->delay,
                   Q("%1 %2 %3")
                   .arg(QY("Delay this track's timestamps by a couple of ms."))
                   .arg(QY("The value can be negative, but keep in mind that any frame whose timestamp is negative after this calculation is dropped."))
                   .arg(QY("This works with all track types.")));
  Util::setToolTip(ui->stretchBy,
                   Q("<p>%1 %2</p><p>%3</p>")
                   .arg(QYH("Multiply this track's timestamps with a factor."))
                   .arg(QYH("The value can be given either as a floating point number (e.g. 12.345) or a fraction of integer values (e.g. 123/456)."))
                   .arg(QYH("This works well for video and subtitle tracks but should not be used with audio tracks.")));
  Util::setToolTip(ui->defaultDuration,
                   Q("%1 %2 %3 %4")
                   .arg(QY("Forces the default duration or number of frames per second for a track."))
                   .arg(QY("The value can be given either as a floating point number (e.g. 12.345) or a fraction of integer values (e.g. 123/456)."))
                   .arg(QY("You can specify one of the units 's', 'ms', 'us', 'ns', 'fps', 'i' or 'p'."))
                   .arg(QY("If no unit is given, 'fps' will be used.")));
  Util::setToolTip(ui->fixBitstreamTimingInfo,
                   Q("%1 %2 %3")
                   .arg(QY("Normally mkvmerge does not change the timing information (frame/field rate) stored in the video bitstream."))
                   .arg(QY("With this option that information is adjusted to match the container's timing information."))
                   .arg(QY("The source for the container's timing information be various things: a value given on the command line with the '--default-duration' option, "
                           "the source container or the video bitstream.")));
  Util::setToolTip(ui->aspectRatio,
                   Q("<p>%1 %2 %3</p><p>%4</p>")
                   .arg(QYH("The Matroska container format can store the display width/height for a video track."))
                   .arg(QYH("This option tells mkvmerge the display aspect ratio to use when it calculates the display width/height."))
                   .arg(QYH("Note that many players don't use the display width/height values directly but only use the ratio given by these values when setting the initial window size."))
                   .arg(QYH("The value can be given either as a floating point number (e.g. 12.345) or a fraction of integer values (e.g. 123/456).")));
  Util::setToolTip(ui->displayWidth,
                   Q("<p>%1 %2</p><p>%3</p>")
                   .arg(QYH("The Matroska container format can store the display width/height for a video track."))
                   .arg(QYH("This parameter is the display width in pixels."))
                   .arg(QYH("Note that many players don't use the display width/height values directly but only use the ratio given by these values when setting the initial window size.")));
  Util::setToolTip(ui->displayHeight,
                   Q("<p>%1 %2</p><p>%3</p>")
                   .arg(QYH("The Matroska container format can store the display width/height for a video track."))
                   .arg(QYH("This parameter is the display height in pixels."))
                   .arg(QYH("Note that many players don't use the display width/height values directly but only use the ratio given by these values when setting the initial window size.")));
  Util::setToolTip(ui->cropping,
                   Q("<p>%1 %2</p><p>%3 %4</p><p>%5</p>")
                   .arg(QYH("Sets the cropping parameters which tell a player to omit a certain number of pixels on the four sides during playback."))
                   .arg(QYH("This must be comma-separated list of four numbers for the cropping to be used at the left, top, right and bottom, e.g. '0,20,0,20'."))
                   .arg(QYH("Note that the video content is not modified by this option."))
                   .arg(QYH("The values are only stored in the track headers."))
                   .arg(QYH("Note also that there are not a lot of players that support the cropping parameters.")));
  Util::setToolTip(ui->stereoscopy,
                   Q("%1 %2")
                   .arg(QY("Sets the stereo mode of the video track to this value."))
                   .arg(QY("If left empty then the track's original stereo mode will be kept or, if it didn't have one, none will be set at all.")));
  Util::setToolTip(ui->naluSizeLength,
                   Q("<p>%1 %2 %3</p><p>%4</p>")
                   .arg(QYH("Forces the NALU size length to a certain number of bytes."))
                   .arg(QYH("It defaults to 4 bytes, but there are files which do not contain a frame or slice that is bigger than 65535 bytes."))
                   .arg(QYH("For such files you can use this parameter and decrease the size to 2."))
                   .arg(QYH("This parameter is only effective for AVC/h.264 and HEVC/h.265 elementary streams read from AVC/h.264 ES or HEVC/h.265 ES files, AVIs or Matroska files created with '--engage allow_avc_in_vwf_mode'.")));
  Util::setToolTip(ui->aacIsSBR,
                   Q("%1 %2 %3")
                   .arg(QY("This track contains SBR AAC/HE-AAC/AAC+ data."))
                   .arg(QY("Only needed for AAC source files as SBR AAC cannot be detected automatically for these files."))
                   .arg(QY("Not needed for AAC tracks read from other container formats like MP4 or Matroska files.")));
  Util::setToolTip(ui->reduceToAudioCore,
                   Q("%1 %2")
                   .arg(QY("Drops the lossless extensions from an audio track and keeps only its lossy core."))
                   .arg(QY("This only works with DTS audio tracks.")));
  Util::setToolTip(ui->subtitleCharacterSet,
                   Q("<p>%1 %2</p><p><ol><li>%3</li><li>%4</li></p>")
                   .arg(QYH("Selects the character set a subtitle file or chapter information was written with."))
                   .arg(QYH("Only needed in certain situations:"))
                   .arg(QYH("for subtitle files that do not use a byte order marker (BOM) and that are not encoded in the system's current character set (%1)").arg(Q(g_cc_local_utf8->get_charset())))
                   .arg(QYH("for files with chapter information (e.g. OGM, MP4) for which mkvmerge does not detect the encoding correctly")));
  Util::setToolTip(ui->cues,
                   Q("%1 %2")
                   .arg(QY("Selects for which blocks mkvmerge will produce index entries ( = cue entries)."))
                   .arg(QY("\"Determine automatically\" is a good choice for almost all situations.")));
  Util::setToolTip(ui->additionalTrackOptions,
                   Q("%1 %2 %3")
                   .arg(QY("Free-form edit field for user defined options for this track."))
                   .arg(QY("What you input here is added after all the other options the GUI adds so that you could overwrite any of the GUI's options for this track."))
                   .arg(QY("All occurences of the string \"<TID>\" will be replaced by the track's track ID.")));
}

void
Tab::setupFileIdentificationThread() {
  auto &worker = m_identifier->worker();

  connect(&worker, &FileIdentificationWorker::queueStarted,                    this, &Tab::fileIdentificationStarted);
  connect(&worker, &FileIdentificationWorker::queueFinished,                   this, &Tab::fileIdentificationFinished);
  connect(&worker, &FileIdentificationWorker::filesIdentified,                 this, &Tab::addOrAppendIdentifiedFiles);
  connect(&worker, &FileIdentificationWorker::identificationFailed,            this, &Tab::showFileIdentificationError);
  connect(&worker, &FileIdentificationWorker::identifiedAsXmlOrSimpleChapters, this, &Tab::handleIdentifiedXmlOrSimpleChapters);
  connect(&worker, &FileIdentificationWorker::identifiedAsXmlSegmentInfo,      this, &Tab::handleIdentifiedXmlSegmentInfo);
  connect(&worker, &FileIdentificationWorker::identifiedAsXmlTags,             this, &Tab::handleIdentifiedXmlTags);
  connect(&worker, &FileIdentificationWorker::playlistScanStarted,             this, &Tab::showScanningPlaylistDialog);
  connect(&worker, &FileIdentificationWorker::playlistScanDecisionNeeded,      this, &Tab::selectScanPlaylistPolicy);
  connect(&worker, &FileIdentificationWorker::playlistSelectionNeeded,         this, &Tab::selectPlaylistToAdd);

  m_identifier->start();
}

void
Tab::fileIdentificationStarted() {
  ui->overlordWidget->setEnabled(false);
}

void
Tab::fileIdentificationFinished() {
  ui->overlordWidget->setEnabled(true);
}

void
Tab::onFileRowsInserted(QModelIndex const &parentIdx,
                        int,
                        int) {
  if (parentIdx.isValid())
    ui->files->setExpanded(parentIdx, true);
}

void
Tab::onTrackRowsInserted(QModelIndex const &parentIdx,
                         int,
                         int) {
  if (parentIdx.isValid())
    ui->tracks->setExpanded(parentIdx, true);
}

void
Tab::onTrackSelectionChanged() {
  Util::enableWidgets(m_allInputControls, false);
  ui->moveTracksUp->setEnabled(false);
  ui->moveTracksDown->setEnabled(false);
  ui->subtitleCharacterSetPreview->setEnabled(false);

  auto selection = ui->tracks->selectionModel()->selection();
  auto numRows   = Util::numSelectedRows(selection);
  if (!numRows)
    return;

  ui->moveTracksUp->setEnabled(true);
  ui->moveTracksDown->setEnabled(true);

  if (1 < numRows) {
    setInputControlValues(nullptr);
    Util::enableWidgets(m_allInputControls, true);
    return;
  }

  Util::enableWidgets(m_typeIndependentControls, true);

  auto idxs = selection[0].indexes();
  if (idxs.isEmpty() || !idxs[0].isValid())
    return;

  auto track = m_tracksModel->fromIndex(idxs[0]);
  if (!track)
    return;

  setInputControlValues(track);

  if (track->isAudio()) {
    Util::enableWidgets(m_audioControls, true);
    Util::enableWidgets({ ui->aacIsSBRLabel, ui->aacIsSBR }, track->canSetAacToSbr());
    ui->reduceToAudioCore->setEnabled(track->canReduceToAudioCore());

  } else if (track->isVideo())
    Util::enableWidgets(m_videoControls, true);

  else if (track->isSubtitles()) {
    Util::enableWidgets(m_subtitleControls, true);
    if (!track->canChangeSubCharset())
      Util::enableWidgets(QList<QWidget *>{} << ui->characterSetLabel << ui->subtitleCharacterSet, false);

    else if (track->m_file->isTextSubtitleContainer())
      ui->subtitleCharacterSetPreview->setEnabled(true);

  } else if (track->isChapters()) {
    Util::enableWidgets(m_chapterControls, true);

    if (!track->canChangeSubCharset())
      Util::enableWidgets(QList<QWidget *>{} << ui->characterSetLabel << ui->subtitleCharacterSet, false);
  }

  if (track->isAppended())
    Util::enableWidgets(m_notIfAppendingControls, false);
}

void
Tab::addOrRemoveEmptyComboBoxItem(bool add) {
  for (auto &comboBox : m_comboBoxControls)
    if (add && comboBox->itemData(0).isValid())
      comboBox->insertItem(0, QY("<Do not change>"));
    else if (!add && !comboBox->itemData(0).isValid())
      comboBox->removeItem(0);
}

void
Tab::clearInputControlValues() {
  for (auto comboBox : m_comboBoxControls)
    comboBox->setCurrentIndex(0);

  for (auto control : std::vector<QLineEdit *>{ui->trackName, ui->trackTags, ui->delay, ui->stretchBy, ui->timestamps, ui->displayWidth, ui->displayHeight, ui->cropping, ui->additionalTrackOptions})
    control->setText(Q(""));

  ui->defaultDuration->setEditText(Q(""));
  ui->aspectRatio->setEditText(Q(""));

  ui->setAspectRatio->setChecked(false);
  ui->setDisplayWidthHeight->setChecked(false);
}

void
Tab::setInputControlValues(Track *track) {
  m_currentlySettingInputControlValues = true;

  addOrRemoveEmptyComboBoxItem(!track);

  auto additionalLanguages     = QSet<QString>{};
  auto additionalCharacterSets = QSet<QString>{};

  for (auto const &sourceFile : m_config.m_files)
    for (auto const &track : sourceFile->m_tracks) {
      additionalLanguages     << track->m_language;
      additionalCharacterSets << track->m_characterSet;
    }

  ui->trackLanguage->setAdditionalItems(additionalLanguages.toList()).reInitializeIfNecessary();
  ui->subtitleCharacterSet->setAdditionalItems(additionalCharacterSets.toList()).reInitializeIfNecessary();

  if (!track) {
    clearInputControlValues();
    m_currentlySettingInputControlValues = false;
    return;
  }

  Util::setComboBoxIndexIf(ui->muxThis,          [&track](auto const &, auto const &data) { return data.isValid() && (data.toBool() == track->m_muxThis);          });
  Util::setComboBoxIndexIf(ui->defaultTrackFlag, [&track](auto const &, auto const &data) { return data.isValid() && (data.toUInt() == track->m_defaultTrackFlag); });
  Util::setComboBoxIndexIf(ui->forcedTrackFlag,  [&track](auto const &, auto const &data) { return data.isValid() && (data.toUInt() == track->m_forcedTrackFlag);  });
  Util::setComboBoxIndexIf(ui->compression,      [&track](auto const &, auto const &data) { return data.isValid() && (data.toUInt() == track->m_compression);      });
  Util::setComboBoxIndexIf(ui->cues,             [&track](auto const &, auto const &data) { return data.isValid() && (data.toUInt() == track->m_cues);             });
  Util::setComboBoxIndexIf(ui->stereoscopy,      [&track](auto const &, auto const &data) { return data.isValid() && (data.toUInt() == track->m_stereoscopy);      });
  Util::setComboBoxIndexIf(ui->naluSizeLength,   [&track](auto const &, auto const &data) { return data.isValid() && (data.toUInt() == track->m_naluSizeLength);   });
  Util::setComboBoxIndexIf(ui->aacIsSBR,         [&track](auto const &, auto const &data) { return data.isValid() && (data.toUInt() == track->m_aacIsSBR);         });

  ui->trackLanguage->setCurrentByData(track->m_language);
  ui->subtitleCharacterSet->setCurrentByData(track->m_characterSet);

  ui->trackName->setText(                track->m_name);
  ui->trackTags->setText(                track->m_tags);
  ui->delay->setText(                    track->m_delay);
  ui->stretchBy->setText(                track->m_stretchBy);
  ui->timestamps->setText(               track->m_timestamps);
  ui->displayWidth->setText(             track->m_displayWidth);
  ui->displayHeight->setText(            track->m_displayHeight);
  ui->cropping->setText(                 track->m_cropping);
  ui->additionalTrackOptions->setText(   track->m_additionalOptions);
  ui->defaultDuration->setEditText(      track->m_defaultDuration);
  ui->aspectRatio->setEditText(          track->m_aspectRatio);

  ui->setAspectRatio->setChecked(        track->m_setAspectRatio);
  ui->setDisplayWidthHeight->setChecked(!track->m_setAspectRatio);
  ui->fixBitstreamTimingInfo->setChecked(track->m_fixBitstreamTimingInfo);
  ui->reduceToAudioCore->setChecked(     track->m_reduceAudioToCore);

  m_currentlySettingInputControlValues = false;
}

QList<SourceFile *>
Tab::selectedSourceFiles()
  const {
  auto sourceFiles = QList<SourceFile *>{};
  Util::withSelectedIndexes(ui->files, [&sourceFiles, this](QModelIndex const &idx) {
      auto sourceFile = m_filesModel->fromIndex(idx);
      if (sourceFile)
        sourceFiles << sourceFile.get();
  });

  return sourceFiles;
}

QList<Track *>
Tab::selectedTracks()
  const {
  auto tracks = QList<Track *>{};
  Util::withSelectedIndexes(ui->tracks, [&tracks, this](QModelIndex const &idx) {
      auto track = m_tracksModel->fromIndex(idx);
      if (track)
        tracks << track;
    });

  return tracks;
}

void
Tab::withSelectedTracks(std::function<void(Track &)> code,
                        bool notIfAppending,
                        QWidget *widget) {
  if (m_currentlySettingInputControlValues)
    return;

  auto tracks = selectedTracks();
  if (tracks.isEmpty())
    return;

  if (!widget)
    widget = static_cast<QWidget *>(QObject::sender());

  bool withAudio     = m_audioControls.contains(widget);
  bool withVideo     = m_videoControls.contains(widget);
  bool withSubtitles = m_subtitleControls.contains(widget);
  bool withChapters  = m_chapterControls.contains(widget);
  bool withAll       = m_typeIndependentControls.contains(widget);

  for (auto &track : tracks) {
    if (track->m_appendedTo && notIfAppending)
      continue;

    if (   withAll
        || (track->isAudio()     && withAudio)
        || (track->isVideo()     && withVideo)
        || (track->isSubtitles() && withSubtitles)
        || (track->isChapters()  && withChapters)) {
      code(*track);
      m_tracksModel->trackUpdated(track);
    }
  }
}

void
Tab::onTrackNameChanged(QString newValue) {
  withSelectedTracks([&newValue](auto &track) { track.m_name = newValue; }, true);
}

void
Tab::onTrackItemChanged(QStandardItem *item) {
  if (!item)
    return;

  auto idx = m_tracksModel->indexFromItem(item);
  if (idx.column())
    return;

  auto track = m_tracksModel->fromIndex(idx);
  if (!track)
    return;

  auto newWantedMuxThis = (item->checkState() == Qt::Checked);
  auto newMuxThis       = newWantedMuxThis && (!track->m_appendedTo || track->m_appendedTo->m_muxThis);

  if (newWantedMuxThis != newMuxThis)
    item->setCheckState(newMuxThis ? Qt::Checked : Qt::Unchecked);

  if (newMuxThis == track->m_muxThis)
    return;

  track->m_muxThis = newMuxThis;
  m_tracksModel->trackUpdated(track);

  for (auto appendedTrack : track->m_appendedTracks) {
    appendedTrack->m_muxThis = newMuxThis;
    m_tracksModel->trackUpdated(appendedTrack);
  }

  auto selected = selectedTracks();
  if ((1 == selected.count()) && selected.contains(track))
    Util::setComboBoxIndexIf(ui->muxThis, [newMuxThis](QString const &, QVariant const &data) { return data.isValid() && (data.toBool() == newMuxThis); });

  setOutputFileNameMaybe();

  m_tracksModel->updateEffectiveDefaultTrackFlags();
}

void
Tab::onMuxThisChanged(int selected) {
  auto data = ui->muxThis->itemData(selected);
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
    m_tracksModel->trackUpdated(track);
  }

  setOutputFileNameMaybe();

  m_tracksModel->updateEffectiveDefaultTrackFlags();

  auto tracks = selectedTracks();
  if (1 == tracks.count())
    Util::setComboBoxIndexIf(ui->muxThis, [&tracks](QString const &, QVariant const &data) { return data.isValid() && (data.toBool() == tracks[0]->m_muxThis); });
}

void
Tab::toggleMuxThisForSelectedTracks() {
  auto allEnabled     = true;
  auto tracksSelected = false;

  withSelectedTracks([&allEnabled, &tracksSelected](auto &track) {
    tracksSelected = true;

    if (!track.m_muxThis)
      allEnabled = false;
  }, false, ui->muxThis);

  if (!tracksSelected) {
    m_tracksModel->updateEffectiveDefaultTrackFlags();
    return;
  }

  auto newEnabled = !allEnabled;
  QList<Track *> appendedTracks;

  withSelectedTracks([newEnabled, &appendedTracks](auto &track) {
    if (!track.m_appendedTo || track.m_appendedTo->m_muxThis)
      track.m_muxThis = newEnabled;
    appendedTracks += track.m_appendedTracks;
  }, false, ui->muxThis);

  for (auto track : appendedTracks) {
    track->m_muxThis = newEnabled;
    m_tracksModel->trackUpdated(track);
  }

  Util::setComboBoxIndexIf(ui->muxThis, [newEnabled](auto const &, auto const &data) { return data.isValid() && (data.toBool() == newEnabled); });

  m_tracksModel->updateEffectiveDefaultTrackFlags();
}

void
Tab::onTrackLanguageChanged(int newValue) {
  auto code = ui->trackLanguage->itemData(newValue).toString();
  if (code.isEmpty())
    return;

  withSelectedTracks([&code](auto &track) { track.m_language = code; }, true);
}

void
Tab::onDefaultTrackFlagChanged(int newValue) {
  auto data = ui->defaultTrackFlag->itemData(newValue);
  if (!data.isValid())
    return;
  newValue = data.toInt();

  withSelectedTracks([this, newValue](auto &track) {
    track.m_defaultTrackFlag = newValue;
    if (1 == newValue)
      this->ensureOneDefaultFlagOnly(&track);
  }, true);

  m_tracksModel->updateEffectiveDefaultTrackFlags();
}

void
Tab::onForcedTrackFlagChanged(int newValue) {
  auto data = ui->forcedTrackFlag->itemData(newValue);
  if (!data.isValid())
    return;
  newValue = data.toInt();

  withSelectedTracks([&newValue](auto &track) { track.m_forcedTrackFlag = newValue; }, true);
}

void
Tab::onCompressionChanged(int newValue) {
  auto data = ui->compression->itemData(newValue);
  if (!data.isValid())
    return;
  newValue = data.toInt();

  auto compression = 1 == newValue ? Track::CompNone
                   : 2 == newValue ? Track::CompZlib
                   :                 Track::CompDefault;

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
Tab::onTimestampsChanged(QString newValue) {
  withSelectedTracks([&newValue](auto &track) { track.m_timestamps = newValue; });
}

void
Tab::onBrowseTimestamps() {
  auto fileName = getOpenFileName(QY("Select timestamp file"), QY("Text files") + Q(" (*.txt)"), ui->timestamps);
  if (!fileName.isEmpty())
    withSelectedTracks([&fileName](auto &track) { track.m_timestamps = fileName; });
}

void
Tab::onFixBitstreamTimingInfoChanged(bool newValue) {
  withSelectedTracks([&newValue](auto &track) { track.m_fixBitstreamTimingInfo = newValue; }, true);
}

void
Tab::onBrowseTrackTags() {
  auto fileName = getOpenFileName(QY("Select tags file"), QY("XML tag files") + Q(" (*.xml)"), ui->trackTags);
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
  withSelectedTracks([&newValue](auto &track) { track.m_aspectRatio = newValue; }, true);
}

void
Tab::onDisplayWidthChanged(QString newValue) {
  withSelectedTracks([&newValue](auto &track) { track.m_displayWidth = newValue; }, true);
}

void
Tab::onDisplayHeightChanged(QString newValue) {
  withSelectedTracks([&newValue](auto &track) { track.m_displayHeight = newValue; }, true);
}

void
Tab::onStereoscopyChanged(int newValue) {
  auto data = ui->stereoscopy->itemData(newValue);
  if (!data.isValid())
    return;
  newValue = data.toInt();

  withSelectedTracks([&newValue](auto &track) { track.m_stereoscopy = newValue; }, true);
}

void
Tab::onNaluSizeLengthChanged(int newValue) {
  auto data = ui->naluSizeLength->itemData(newValue);
  if (!data.isValid())
    return;
  newValue = data.toInt();

  withSelectedTracks([&newValue](auto &track) { track.m_naluSizeLength = newValue; }, true);
}

void
Tab::onCroppingChanged(QString newValue) {
  withSelectedTracks([&newValue](auto &track) { track.m_cropping = newValue; }, true);
}

void
Tab::onAacIsSBRChanged(int newValue) {
  auto data = ui->aacIsSBR->itemData(newValue);
  if (!data.isValid())
    return;
  newValue = data.toInt();

  withSelectedTracks([&newValue](auto &track) { track.m_aacIsSBR = newValue; }, true);
}

void
Tab::onReduceAudioToCoreChanged(bool newValue) {
  withSelectedTracks([&newValue](auto &track) { track.m_reduceAudioToCore = newValue; });
}

void
Tab::onSubtitleCharacterSetChanged(int newValue) {
  auto data = ui->subtitleCharacterSet->itemData(newValue);
  if (!data.isValid())
    return;

  withSelectedTracks([&data](auto &track) {
    if (track.canChangeSubCharset())
      track.m_characterSet = data.toString();
  }, true);
}

void
Tab::onCuesChanged(int newValue) {
  auto data = ui->cues->itemData(newValue);
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
  addOrAppendFiles(false);
}

void
Tab::onAppendFiles() {
  addOrAppendFiles(true);
}

void
Tab::addOrAppendFiles(bool append) {
  auto fileNames = selectFilesToAdd(append ? QY("Append media files") : QY("Add media files"));
  if (!fileNames.isEmpty())
    addOrAppendFiles(append, fileNames, selectedSourceFile());
}

void
Tab::addFiles(QStringList const &fileNames) {
  addOrAppendFiles(false, fileNames, QModelIndex{});
}

void
Tab::handleIdentifiedXmlOrSimpleChapters(QString const &fileName) {
  Util::MessageBox::warning(this)
    ->title(QY("Adding chapter files"))
    .text(Q("%1 %2 %3 %4")
          .arg(QY("The file '%1' contains chapters.").arg(fileName))
          .arg(QY("These aren't treated like other source files in MKVToolNix."))
          .arg(QY("Instead such a file must be set via the 'chapter file' option on the 'output' tab."))
          .arg(QY("The GUI will enter the dropped file's file name into that control replacing any file name which might have been set earlier.")))
    .onlyOnce(Q("mergeChaptersDropped"))
    .exec();

  m_config.m_chapters = fileName;
  ui->chapters->setText(fileName);

  m_identifier->continueIdentification();
}

void
Tab::handleIdentifiedXmlSegmentInfo(QString const &fileName) {
  Util::MessageBox::warning(this)
    ->title(QY("Adding segment info files"))
    .text(Q("%1 %2 %3 %4")
          .arg(QY("The file '%1' contains segment information.").arg(fileName))
          .arg(QY("These aren't treated like other source files in MKVToolNix."))
          .arg(QY("Instead such a file must be set via the 'segment info' option on the 'output' tab."))
          .arg(QY("The GUI will enter the dropped file's file name into that control replacing any file name which might have been set earlier.")))
    .onlyOnce(Q("mergeSegmentInfoDropped"))
    .exec();

  m_config.m_segmentInfo = fileName;
  ui->segmentInfo->setText(fileName);

  m_identifier->continueIdentification();
}

void
Tab::handleIdentifiedXmlTags(QString const &fileName) {
  Util::MessageBox::warning(this)
    ->title(QY("Adding tag files"))
    .text(Q("%1 %2 %3 %4")
          .arg(QY("The file '%1' contains tags.").arg(fileName))
          .arg(QY("These aren't treated like other source files in MKVToolNix."))
          .arg(QY("Instead such a file must be set via the 'global tags' option on the 'output' tab."))
          .arg(QY("The GUI will enter the dropped file's file name into that control replacing any file name which might have been set earlier.")))
    .onlyOnce(Q("mergeTagsDropped"))
    .exec();

  m_config.m_globalTags = fileName;
  ui->globalTags->setText(fileName);

  m_identifier->continueIdentification();
}

void
Tab::showFileIdentificationError(QString const &errorTitle,
                                 QString const &errorText) {
  Util::MessageBox::critical(this)->title(errorTitle).text(errorText).exec();
  m_identifier->continueIdentification();
}

void
Tab::addOrAppendFiles(bool append,
                      QStringList const &fileNames,
                      QModelIndex const &sourceFileIdx) {
  if (!fileNames.isEmpty())
    Util::Settings::get().m_lastOpenDir = QFileInfo{fileNames.last()}.path();

  qDebug() << "Tab::addOrAppendFiles: TID" << QThread::currentThreadId();

  m_identifier->worker().addFilesToIdentify(fileNames, append, sourceFileIdx);
}

void
Tab::addOrAppendIdentifiedFiles(QList<SourceFilePtr> const &identifiedFiles,
                                bool append,
                                QModelIndex const &sourceFileIdx) {
  if (identifiedFiles.isEmpty())
    return;

  if (m_debugTrackModel) {
    log_it(boost::format("### BEFORE adding/appending ###\n"));
    m_config.debugDumpFileList();
    m_config.debugDumpTrackList();
  }

  setDefaultsFromSettingsForAddedFiles(identifiedFiles);

  m_filesModel->addOrAppendFilesAndTracks(sourceFileIdx, identifiedFiles, append);

  if (m_debugTrackModel) {
    log_it(boost::format("### AFTER adding/appending ###\n"));
    m_config.debugDumpFileList();
    m_config.debugDumpTrackList();
  }

  reinitFilesTracksControls();

  if (m_config.m_firstInputFileName.isEmpty())
    m_config.m_firstInputFileName = m_config.determineFirstInputFileName(identifiedFiles);

  setTitleMaybe(identifiedFiles);
  setOutputFileNameMaybe();
}

void
Tab::selectScanPlaylistPolicy(SourceFilePtr const &sourceFile,
                              QFileInfoList const &files) {
  AskScanForPlaylistsDialog dialog{this};

  if (dialog.ask(*sourceFile, files.count() - 1))
    m_identifier->continueByScanningPlaylists(files);

  else {
    m_identifier->worker().addIdentifiedFile(sourceFile);
    m_identifier->continueIdentification();
  }
}

void
Tab::showScanningPlaylistDialog(int numFilesToScan) {
  auto progressDialog = new QProgressDialog{ QY("Scanning directory"), QY("Cancel"), 0, numFilesToScan, this };
  auto &worker        = m_identifier->worker();

  connect(&worker,        &FileIdentificationWorker::playlistScanProgressChanged, progressDialog, &QProgressDialog::setValue);
  connect(&worker,        &FileIdentificationWorker::playlistScanFinished,        progressDialog, &QProgressDialog::deleteLater);
  connect(progressDialog, &QProgressDialog::canceled,                             m_identifier,   &FileIdentificationThread::abortPlaylistScan);

  progressDialog->show();
}

void
Tab::selectPlaylistToAdd(QList<SourceFilePtr> const &identifiedPlaylists) {
  auto playlist = SelectPlaylistDialog{this, identifiedPlaylists}.select();

  if (playlist)
    m_identifier->worker().addIdentifiedFile(playlist);

  m_identifier->continueIdentification();
}

void
Tab::setDefaultsFromSettingsForAddedFiles(QList<SourceFilePtr> const &files) {
  auto &cfg = Util::Settings::get();

  auto defaultFlagSet = QHash<Track::Type, bool>{};
  for (auto const &track : m_config.m_tracks)
    if (track->m_defaultTrackFlag == 1)
      defaultFlagSet[track->m_type] = true;

  for (auto const &file : files)
    for (auto const &track : file->m_tracks) {
      if (cfg.m_disableCompressionForAllTrackTypes)
        track->m_compression = Track::CompNone;

      if (cfg.m_disableDefaultTrackForSubtitles && track->isSubtitles())
        track->m_defaultTrackFlag = 2;

      else if (track->m_defaultTrackFlagWasSet && !defaultFlagSet[track->m_type]) {
        track->m_defaultTrackFlag     = 1;
        defaultFlagSet[track->m_type] = true;
      }
    }
}

QStringList
Tab::selectFilesToAdd(QString const &title) {
  return Util::getOpenFileNames(this, title, Util::Settings::get().lastOpenDirPath(), Util::FileTypeFilter::get().join(Q(";;")), nullptr, QFileDialog::HideNameFilterDetails);
}

void
Tab::onAddAdditionalParts() {
  auto currentIdx = selectedSourceFile();
  auto sourceFile = m_filesModel->fromIndex(currentIdx);
  if (sourceFile && !sourceFile->m_tracks.size()) {
    Util::MessageBox::critical(this)->title(QY("Unable to append files")).text(QY("You cannot add additional parts to files that don't contain tracks.")).exec();
    return;
  }

  m_filesModel->addAdditionalParts(currentIdx, selectFilesToAdd(QY("Add media files as additional parts")));
}

void
Tab::onRemoveFiles() {
  auto selectedFiles = selectedSourceFiles();
  if (selectedFiles.isEmpty())
    return;

  m_filesModel->removeFiles(selectedFiles);

  reinitFilesTracksControls();

  if (!m_filesModel->rowCount()) {
    m_config.m_firstInputFileName.clear();
    clearDestinationMaybe();
    clearTitleMaybe();
  }
}

void
Tab::onRemoveAllFiles() {
  if (m_config.m_files.isEmpty())
    return;

  m_attachedFilesModel->reset();
  m_filesModel->removeRows(0, m_filesModel->rowCount());
  m_tracksModel->removeRows(0, m_tracksModel->rowCount());
  m_config.m_files.clear();
  m_config.m_tracks.clear();
  m_config.m_firstInputFileName.clear();

  reinitFilesTracksControls();
  clearDestinationMaybe();
  clearTitleMaybe();
}

void
Tab::reinitFilesTracksControls() {
  resizeFilesColumnsToContents();
  resizeTracksColumnsToContents();
  resizeAttachedFilesColumnsToContents();
  onTrackSelectionChanged();
}

void
Tab::resizeFilesColumnsToContents()
  const {
  Util::resizeViewColumnsToContents(ui->files);
}

void
Tab::resizeTracksColumnsToContents()
  const {
  Util::resizeViewColumnsToContents(ui->tracks);
}

void
Tab::enableMoveFilesButtons() {
  auto hasSelected = !ui->files->selectionModel()->selection().isEmpty();

  ui->moveFilesUp->setEnabled(hasSelected);
  ui->moveFilesDown->setEnabled(hasSelected);
}

void
Tab::enableFilesActions() {
  int numSelected      = selectedSourceFiles().size();
  bool hasRegularTrack = false;
  if (1 == numSelected)
    hasRegularTrack = m_config.m_files.end() != brng::find_if(m_config.m_files, [](SourceFilePtr const &file) { return file->hasRegularTrack(); });

  m_addFilesAction->setEnabled(true);
  m_addFilesAction2->setEnabled(true);
  m_appendFilesAction->setEnabled((1 == numSelected) && hasRegularTrack);
  m_appendFilesAction2->setEnabled((1 == numSelected) && hasRegularTrack);
  m_addAdditionalPartsAction->setEnabled(1 == numSelected);
  m_addAdditionalPartsAction2->setEnabled(1 == numSelected);
  m_removeFilesAction->setEnabled(0 < numSelected);
  m_removeAllFilesAction->setEnabled(!m_config.m_files.isEmpty());
  m_setDestinationFileNameAction->setEnabled(1 == numSelected);
  m_openFilesInMediaInfoAction->setEnabled(0 < numSelected);
  m_selectTracksFromFilesAction->setEnabled(0 < numSelected);

  m_removeFilesAction->setText(QNY("&Remove file", "&Remove files", numSelected));
  m_openFilesInMediaInfoAction->setText(QNY("Open file in &MediaInfo", "Open files in &MediaInfo", numSelected));
  m_selectTracksFromFilesAction->setText(QNY("Select all &items from selected file", "Select all &items from selected files", numSelected));
}

void
Tab::enableTracksActions() {
  int numSelected = selectedTracks().size();
  bool hasTracks  = !!m_tracksModel->rowCount();

  m_selectAllTracksAction->setEnabled(hasTracks);
  m_selectTracksOfTypeMenu->setEnabled(hasTracks);
  m_enableAllTracksAction->setEnabled(hasTracks);
  m_disableAllTracksAction->setEnabled(hasTracks);

  m_selectAllVideoTracksAction->setEnabled(hasTracks);
  m_selectAllAudioTracksAction->setEnabled(hasTracks);
  m_selectAllSubtitlesTracksAction->setEnabled(hasTracks);

  m_openTracksInMediaInfoAction->setEnabled(0 < numSelected);

  m_openTracksInMediaInfoAction->setText(QNY("Open corresponding file in &MediaInfo", "Open corresponding files in &MediaInfo", numSelected));
}

void
Tab::retranslateInputUI() {
  m_filesModel->retranslateUi();
  m_tracksModel->retranslateUi();

  resizeFilesColumnsToContents();
  resizeTracksColumnsToContents();

  m_addFilesAction ->setText(QY("&Add files"));
  m_addFilesAction2->setText(QY("&Add files"));
  m_appendFilesAction ->setText(QY("A&ppend files"));
  m_appendFilesAction2->setText(QY("A&ppend files"));
  m_addAdditionalPartsAction ->setText(QY("Add files as a&dditional parts"));
  m_addAdditionalPartsAction2->setText(QY("Add files as a&dditional parts"));

  m_removeAllFilesAction->setText(QY("Remove a&ll files"));

  m_setDestinationFileNameAction->setText(QY("Set destination &file name from selected file's name"));

  m_selectAllTracksAction->setText(QY("&Select all items"));
  m_selectTracksOfTypeMenu->setTitle(QY("Select all tracks of specific &type"));
  m_enableAllTracksAction->setText(QY("&Enable all items"));
  m_disableAllTracksAction->setText(QY("&Disable all items"));

  m_selectAllVideoTracksAction->setText(QY("&Video"));
  m_selectAllAudioTracksAction->setText(QY("&Audio"));
  m_selectAllSubtitlesTracksAction->setText(QY("&Subtitles"));

  m_startMuxingLeaveAsIs->setText(QY("Afterwards &leave the settings as they are."));
  m_startMuxingCreateNewSettings->setText(QY("Afterwards create &new multiplex settings and close the current ones."));
  m_startMuxingCloseSettings->setText(QY("Afterwards &close the current multiplex settings."));
  m_startMuxingRemoveInputFiles->setText(QY("Afterwards &remove all source files."));

  m_addToJobQueueLeaveAsIs->setText(QY("Afterwards &leave the settings as they are."));
  m_addToJobQueueCreateNewSettings->setText(QY("Afterwards create &new multiplex settings and close the current ones."));
  m_addToJobQueueCloseSettings->setText(QY("Afterwards &close the current multiplex settings."));
  m_addToJobQueueRemoveInputFiles->setText(QY("Afterwards &remove all source files."));

  for (auto idx = 0u, end = stereo_mode_c::max_index(); idx <= end; ++idx)
    ui->stereoscopy->setItemText(idx + 1, QString{"%1 (%2; %3)"}.arg(to_qs(stereo_mode_c::translate(idx))).arg(idx).arg(to_qs(stereo_mode_c::s_modes[idx])));

  Util::fixComboBoxViewWidth(*ui->stereoscopy);

  for (auto &comboBox : m_comboBoxControls)
    if (comboBox->count() && !comboBox->itemData(0).isValid())
      comboBox->setItemText(0, QY("<Do not change>"));

  Util::setComboBoxTexts(ui->muxThis,          QStringList{}                                  << QY("Yes")                  << QY("No"));
  Util::setComboBoxTexts(ui->naluSizeLength,   QStringList{} << QY("Don't change")            << QY("Force 2 bytes")        << QY("Force 4 bytes"));
  Util::setComboBoxTexts(ui->defaultTrackFlag, QStringList{} << QY("Determine automatically") << QY("Yes")                  << QY("No"));
  Util::setComboBoxTexts(ui->forcedTrackFlag,  QStringList{}                                  << QY("Yes")                  << QY("No"));
  Util::setComboBoxTexts(ui->compression,      QStringList{} << QY("Determine automatically") << QY("No extra compression") << Q("zlib"));
  Util::setComboBoxTexts(ui->cues,             QStringList{} << QY("Determine automatically") << QY("Only for I frames")    << QY("For all frames") << QY("No cues"));
  Util::setComboBoxTexts(ui->aacIsSBR,         QStringList{} << QY("Determine automatically") << QY("Yes")                  << QY("No"));

  setupInputToolTips();
}

QModelIndex
Tab::selectedSourceFile()
  const {
  auto idx = ui->files->selectionModel()->currentIndex();
  return m_filesModel->index(idx.row(), 0, idx.parent());
}

void
Tab::setTitleMaybe(QList<SourceFilePtr> const &files) {
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
  if (!Util::Settings::get().m_autoSetFileTitle || title.isEmpty() || !m_config.m_title.isEmpty())
    return;

  ui->title->setText(title);
  m_config.m_title = title;
}

QString
Tab::suggestOutputFileNameExtension()
  const {
  auto hasTracks = false, hasVideo = false, hasAudio = false, hasStereoscopy = false;

  for (auto const &t : m_config.m_tracks) {
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

  return m_config.m_webmMode ? "webm"
       : hasStereoscopy      ? "mk3d"
       : hasVideo            ? "mkv"
       : hasAudio            ? "mka"
       : hasTracks           ? "mks"
       :                       "mkv";
}

void
Tab::setOutputFileNameMaybe(bool force) {
  auto &settings = Util::Settings::get();
  auto policy    = settings.m_outputFileNamePolicy;

  if (   !force
      && (   (Util::Settings::DontSetOutputFileName == policy)
          || m_config.m_firstInputFileName.isEmpty()))
    return;

  auto currentOutput = ui->output->text();
  QDir outputDir;

  // Don't override custom changes to the destination file name.
  if (   !force
      && !currentOutput.isEmpty()
      && !m_config.m_destinationAuto.isEmpty()
      && (QDir::toNativeSeparators(currentOutput) != QDir::toNativeSeparators(m_config.m_destinationAuto)))
    return;

  if (Util::Settings::ToPreviousDirectory == policy)
    outputDir = settings.m_lastOutputDir;

  else if (Util::Settings::ToFixedDirectory == policy)
    outputDir = settings.m_fixedOutputDir;

  else if (Util::Settings::ToRelativeOfFirstInputFile == policy)
    outputDir = QDir{ QFileInfo{m_config.m_firstInputFileName}.absoluteDir().path() + Q("/") + settings.m_relativeOutputDir.path() };

  else if (   (Util::Settings::ToSameAsFirstInputFile == policy)
           || force)
    outputDir = QFileInfo{m_config.m_firstInputFileName}.absoluteDir();

  else
    Q_ASSERT_X(false, "setOutputFileNameMaybe", "Untested destination file name policy");

  auto baseName = QFileInfo{ m_config.m_firstInputFileName }.completeBaseName();
  auto idx      = 0;

  while (true) {
    auto suffix          = suggestOutputFileNameExtension();
    auto currentBaseName = QString{"%1%2.%3"}.arg(baseName).arg(idx ? QString{" (%1)"}.arg(idx) : "").arg(suffix);
    currentBaseName      = Util::removeInvalidPathCharacters(currentBaseName);
    auto outputFileName  = QFileInfo{outputDir, currentBaseName};

    if (!settings.m_uniqueOutputFileNames || !outputFileName.exists()) {
      m_config.m_destinationAuto = outputFileName.absoluteFilePath();

      ui->output->setText(m_config.m_destinationAuto);
      setDestination(m_config.m_destinationAuto);

      break;
    }

    ++idx;
  }
}

void
Tab::setDestinationFileNameFromSelectedFile() {
  auto selectedFiles = selectedSourceFiles();
  if (selectedFiles.isEmpty())
    return;

  m_config.m_destinationAuto.clear();
  m_config.m_firstInputFileName = QDir::toNativeSeparators(selectedFiles[0]->m_fileName);

  setOutputFileNameMaybe(true);
}

void
Tab::addOrAppendDroppedFiles(QStringList const &fileNamesToAddOrAppend,
                             Qt::MouseButtons mouseButtons) {
  auto fileNames = Util::replaceDirectoriesByContainedFiles(fileNamesToAddOrAppend);

  if (fileNames.isEmpty())
    return;

  auto noFilesAdded = m_config.m_files.isEmpty();

  if ((fileNames.count() == 1) && noFilesAdded) {
    addOrAppendFiles(false, fileNames, QModelIndex{});
    return;
  }

  auto &settings = Util::Settings::get();

  auto decision  = settings.m_mergeAddingAppendingFilesPolicy;
  auto fileIdx   = QModelIndex{};

  if (   (Util::Settings::MergeAddingAppendingFilesPolicy::Ask == decision)
      || ((mouseButtons & Qt::RightButton)                     == Qt::RightButton)) {
    AddingAppendingFilesDialog dlg{this, m_config.m_files};
    dlg.setDefaults(settings.m_mergeLastAddingAppendingDecision, m_lastAddAppendFileIdx);
    if (!dlg.exec())
      return;

    decision = dlg.decision();
    fileIdx  = m_filesModel->index(dlg.fileIndex(), 0);

    settings.m_mergeLastAddingAppendingDecision = decision;
    m_lastAddAppendFileIdx                      = dlg.fileIndex();

    if (dlg.alwaysUseThisDecision())
      settings.m_mergeAddingAppendingFilesPolicy = decision;

    settings.save();
  }

  if (Util::Settings::MergeAddingAppendingFilesPolicy::AddAdditionalParts == decision)
    m_filesModel->addAdditionalParts(fileIdx, fileNames);

  else if (Util::Settings::MergeAddingAppendingFilesPolicy::AddToNew == decision)
    MainWindow::mergeTool()->addMultipleFilesToNewSettings(fileNames, false);

  else if (Util::Settings::MergeAddingAppendingFilesPolicy::AddEachToNew == decision) {
    auto toAdd = fileNames;

    if (noFilesAdded)
      addOrAppendFiles(false, QStringList{} << toAdd.takeFirst(), QModelIndex{});

    if (!toAdd.isEmpty())
      MainWindow::mergeTool()->addMultipleFilesToNewSettings(toAdd, true);

  } else
    addOrAppendFiles(Util::Settings::MergeAddingAppendingFilesPolicy::Append == decision, fileNames, fileIdx);
}

void
Tab::addOrAppendDroppedFilesDelayed() {
  addOrAppendDroppedFiles(m_filesToAddDelayed, m_mouseButtonsForFilesToAddDelayed);
  m_filesToAddDelayed.clear();
  m_mouseButtonsForFilesToAddDelayed = Qt::NoButton;
}

void
Tab::addFilesToBeAddedOrAppendedDelayed(QStringList const &fileNames,
                                        Qt::MouseButtons mouseButtons) {
  m_filesToAddDelayed                += fileNames;
  m_mouseButtonsForFilesToAddDelayed  = mouseButtons;

  QTimer::singleShot(0, this, SLOT(addOrAppendDroppedFilesDelayed()));
}

void
Tab::selectAllTracksOfType(boost::optional<Track::Type> type) {
  auto numRows = m_tracksModel->rowCount();
  if (!numRows)
    return;

  auto lastColumn = m_tracksModel->columnCount() - 1;
  auto selection  = QItemSelection{};

  for (auto row = 0; row < numRows; ++row) {
    auto &track      = *m_config.m_tracks[row];
    auto numAppended = track.m_appendedTracks.count();

    if (!type || (track.m_type == *type))
      selection.select(m_tracksModel->index(row, 0), m_tracksModel->index(row, lastColumn));

    if (!numAppended)
      continue;

    auto rowIdx = m_tracksModel->index(row, 0);

    for (auto appendedRow = 0; appendedRow < numAppended; ++appendedRow) {
      auto &appendedTrack = *track.m_appendedTracks[appendedRow];
      if (!type || (appendedTrack.m_type == *type))
        selection.select(m_tracksModel->index(appendedRow, 0, rowIdx), m_tracksModel->index(appendedRow, lastColumn, rowIdx));
    }
  }

  ui->tracks->selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect);
}

void
Tab::selectAllTracks() {
  selectAllTracksOfType({});
}

void
Tab::selectAllVideoTracks() {
  selectAllTracksOfType(Track::Video);
}

void
Tab::selectAllAudioTracks() {
  selectAllTracksOfType(Track::Audio);
}

void
Tab::selectAllSubtitlesTracks() {
  selectAllTracksOfType(Track::Subtitles);
}

void
Tab::selectAllTracksFromSelectedFiles() {
  if (!m_tracksModel->rowCount())
    return;

  auto tracksToSelect = QList<Track *>{};
  for (auto const &sourceFile : selectedSourceFiles())
    for (auto const &track : sourceFile->m_tracks)
      tracksToSelect << track.get();

  selectTracks(tracksToSelect);
}

void
Tab::selectTracks(QList<Track *> const &tracks) {
  auto numColumns = m_tracksModel->columnCount() - 1;
  auto selection  = QItemSelection{};

  for (auto const &track : tracks) {
    auto idx = m_tracksModel->indexFromTrack(track);
    selection.select(idx.sibling(idx.row(), 0), idx.sibling(idx.row(), numColumns));
  }

  ui->tracks->selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect);
}

void
Tab::selectSourceFiles(QList<SourceFile *> const &files) {
  auto numColumns = m_filesModel->columnCount() - 1;
  auto selection  = QItemSelection{};

  for (auto const &file : files) {
    auto idx = m_filesModel->indexFromSourceFile(file);
    selection.select(idx.sibling(idx.row(), 0), idx.sibling(idx.row(), numColumns));
  }

  ui->files->selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect);
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
  for (auto row = 0, numRows = m_tracksModel->rowCount(); row < numRows; ++row) {
    auto track       = m_tracksModel->fromIndex(m_tracksModel->index(row, 0));
    track->m_muxThis = enable;
    m_tracksModel->trackUpdated(track);

    for (auto const &appendedTrack : track->m_appendedTracks) {
      appendedTrack->m_muxThis = enable;
      m_tracksModel->trackUpdated(appendedTrack);
    }
  }

  auto base = ui->muxThis->itemData(0).isValid() ? 0 : 1;
  ui->muxThis->setCurrentIndex(base + (enable ? 0 : 1));
}

void
Tab::ensureOneDefaultFlagOnly(Track *thisOneHasIt) {
  auto selection = selectedTracks();
  for (auto const &track : m_config.m_tracks)
    if (   (track->m_defaultTrackFlag == 1)
        && (track->m_type             == thisOneHasIt->m_type)
        && (track                     != thisOneHasIt)) {
      track->m_defaultTrackFlag = 0;
      if ((selection.count() == 1) && (selection[0] == track))
        ui->defaultTrackFlag->setCurrentIndex(0);
    }

  m_tracksModel->updateEffectiveDefaultTrackFlags();
}

void
Tab::moveSourceFilesUpOrDown(bool up) {
  auto focus = App::instance()->focusWidget();
  auto files = selectedSourceFiles();

  m_filesModel->moveSourceFilesUpOrDown(files, up);

  for (auto const &file : files)
    if (file->isRegular())
      ui->files->setExpanded(m_filesModel->indexFromSourceFile(file), true);

  selectSourceFiles(files);

  if (focus)
    focus->setFocus();
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
  auto focus  = App::instance()->focusWidget();
  auto tracks = selectedTracks();

  m_tracksModel->moveTracksUpOrDown(tracks, up);

  for (auto const &track : tracks)
    if (track->isRegular() && !track->m_appendedTo)
      ui->tracks->setExpanded(m_tracksModel->indexFromTrack(track), true);

  selectTracks(tracks);

  if (focus)
    focus->setFocus();
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
  enableFilesActions();
  m_filesMenu->exec(ui->files->viewport()->mapToGlobal(pos));
}

void
Tab::showTracksContextMenu(QPoint const &pos) {
  enableTracksActions();
  m_tracksMenu->exec(ui->tracks->viewport()->mapToGlobal(pos));
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
  auto exe  = cfg.m_mediaInfoExe.isEmpty() ? Q("mediainfo-gui") : cfg.m_mediaInfoExe;
  exe       = Util::Settings::exeWithPath(exe);

  if (!exe.isEmpty() && QFileInfo{exe}.exists())
    return exe;

  exe = Util::Settings::exeWithPath(Q("mediainfo"));

#if defined(SYS_WINDOWS)
  exe = QSettings{Q("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\MediaInfo.exe"), QSettings::NativeFormat}.value("Default").toString();
  if (!exe.isEmpty() && QFileInfo{exe}.exists())
    return exe;
#endif

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

  for (auto const &sourceFile : m_config.m_files)
    for (auto const &track : sourceFile->m_tracks)
      additionalCharacterSets << track->m_characterSet;

  auto dlg = new SelectCharacterSetDialog{this, track->m_file->m_fileName, track->m_characterSet, additionalCharacterSets.toList()};
  dlg->setUserData(reinterpret_cast<qulonglong>(track));

  connect(dlg, &SelectCharacterSetDialog::characterSetSelected, this, &Tab::setSubtitleCharacterSet);

  dlg->show();
}

void
Tab::setSubtitleCharacterSet(QString const &characterSet) {
  auto dlg = qobject_cast<SelectCharacterSetDialog *>(QObject::sender());
  if (!dlg)
    return;

  auto track = reinterpret_cast<Track *>(dlg->userData().toULongLong());

  if (!m_config.m_tracks.contains(track))
    return;

  track->m_characterSet = characterSet;
  auto selection        = selectedTracks();

  if ((selection.count() == 1) && (selection[0] == track))
    Util::setComboBoxTextByData(ui->subtitleCharacterSet, characterSet);
}

bool
Tab::hasSourceFiles()
  const {
  return !m_config.m_files.isEmpty();
}

}}}
