#include "common/common_pch.h"

#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QList>
#include <QMessageBox>
#include <QRegularExpression>
#include <QString>
#include <QTimer>

#include "common/extern_data.h"
#include "common/iso639.h"
#include "common/qt.h"
#include "common/stereo_mode.h"
#include "mkvtoolnix-gui/app.h"
#include "mkvtoolnix-gui/forms/merge/tab.h"
#include "mkvtoolnix-gui/merge/adding_appending_files_dialog.h"
#include "mkvtoolnix-gui/merge/tab.h"
#include "mkvtoolnix-gui/merge/playlist_scanner.h"
#include "mkvtoolnix-gui/util/file_identifier.h"
#include "mkvtoolnix-gui/util/file_type_filter.h"
#include "mkvtoolnix-gui/util/message_box.h"
#include "mkvtoolnix-gui/util/settings.h"
#include "mkvtoolnix-gui/util/util.h"

namespace mtx { namespace gui { namespace Merge {

using namespace mtx::gui;

void
Tab::setupControlLists() {
  m_typeIndependantControls << ui->muxThisLabel << ui->muxThis << ui->miscellaneousBox << ui->additionalTrackOptionsLabel << ui->additionalTrackOptions;

  m_audioControls << ui->trackNameLabel << ui->trackName << ui->trackLanguageLabel << ui->trackLanguage << ui->defaultTrackFlagLabel << ui->defaultTrackFlag << ui->forcedTrackFlagLabel << ui->forcedTrackFlag
                  << ui->compressionLabel << ui->compression << ui->trackTagsLabel << ui->trackTags << ui->browseTrackTags << ui->timecodesAndDefaultDurationBox
                  << ui->delayLabel << ui->delay << ui->stretchByLabel << ui->stretchBy << ui->timecodesLabel << ui->timecodes << ui->browseTimecodes << ui->audioPropertiesBox << ui->aacIsSBR << ui->cuesLabel << ui->cues
                  << ui->propertiesLabel << ui->generalOptionsBox << ui->reduceToAudioCore;

  m_videoControls << ui->trackNameLabel << ui->trackName << ui->trackLanguageLabel << ui->trackLanguage << ui->defaultTrackFlagLabel << ui->defaultTrackFlag
                  << ui->forcedTrackFlagLabel << ui->forcedTrackFlag << ui->compressionLabel << ui->compression << ui->trackTagsLabel << ui->trackTags << ui->browseTrackTags << ui->timecodesAndDefaultDurationBox
                  << ui->delayLabel << ui->delay << ui->stretchByLabel << ui->stretchBy << ui->defaultDurationLabel << ui->defaultDuration << ui->timecodesLabel << ui->timecodes << ui->browseTimecodes
                  << ui->picturePropertiesBox << ui->setAspectRatio << ui->aspectRatio << ui->setDisplayWidthHeight << ui->displayWidth << ui->displayDimensionsXLabel << ui->displayHeight << ui->stereoscopyLabel
                  << ui->stereoscopy << ui->naluSizeLengthLabel << ui->naluSizeLength << ui->croppingLabel << ui->cropping << ui->cuesLabel << ui->cues
                  << ui->propertiesLabel << ui->generalOptionsBox << ui->fixBitstreamTimingInfo;

  m_subtitleControls << ui->trackNameLabel << ui->trackName << ui->trackLanguageLabel << ui->trackLanguage << ui->defaultTrackFlagLabel << ui->defaultTrackFlag
                     << ui->forcedTrackFlagLabel << ui->forcedTrackFlag << ui->compressionLabel << ui->compression << ui->trackTagsLabel << ui->trackTags << ui->browseTrackTags << ui->timecodesAndDefaultDurationBox
                     << ui->delayLabel << ui->delay << ui->stretchByLabel << ui->stretchBy << ui->timecodesLabel << ui->timecodes << ui->browseTimecodes
                     << ui->subtitleAndChapterPropertiesBox << ui->characterSetLabel << ui->subtitleCharacterSet << ui->cuesLabel << ui->cues
                     << ui->propertiesLabel << ui->generalOptionsBox;

  m_chapterControls << ui->subtitleAndChapterPropertiesBox << ui->characterSetLabel << ui->subtitleCharacterSet << ui->propertiesLabel << ui->generalOptionsBox;

  m_allInputControls << ui->muxThisLabel << ui->muxThis << ui->trackNameLabel << ui->trackName << ui->trackLanguageLabel << ui->trackLanguage << ui->defaultTrackFlagLabel << ui->defaultTrackFlag
                     << ui->forcedTrackFlagLabel << ui->forcedTrackFlag << ui->compressionLabel << ui->compression << ui->trackTagsLabel << ui->trackTags << ui->browseTrackTags << ui->timecodesAndDefaultDurationBox
                     << ui->delayLabel << ui->delay << ui->stretchByLabel << ui->stretchBy << ui->defaultDurationLabel << ui->defaultDuration << ui->timecodesLabel << ui->timecodes << ui->browseTimecodes
                     << ui->picturePropertiesBox << ui->setAspectRatio << ui->aspectRatio << ui->setDisplayWidthHeight << ui->displayWidth << ui->displayDimensionsXLabel << ui->displayHeight << ui->stereoscopyLabel
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
Tab::setupInputControls() {
  setupControlLists();

  ui->files->setModel(m_filesModel);
  ui->tracks->setModel(m_tracksModel);
  ui->tracks->enterActivatesAllSelected(true);

  // Track & chapter language
  Util::setupLanguageComboBox(*ui->trackLanguage);
  Util::setupLanguageComboBox(*ui->chapterLanguage, QString{}, true);

  // Track & chapter character set
  Util::setupCharacterSetComboBox(*ui->subtitleCharacterSet, QString{}, true);
  Util::setupCharacterSetComboBox(*ui->chapterCharacterSet,  QString{}, true);

  ui->muxThis->addItem(QString{}, true);
  ui->muxThis->addItem(QString{}, false);

  // Stereoscopy
  ui->stereoscopy->addItem(Q(""), 0);
  for (auto idx = 0u, end = stereo_mode_c::max_index(); idx <= end; ++idx)
    ui->stereoscopy->addItem(QString{"%1 (%2; %3)"}.arg(to_qs(stereo_mode_c::translate(idx))).arg(idx).arg(to_qs(stereo_mode_c::s_modes[idx])), idx + 1);

  // NALU size length
  for (auto idx = 0; idx < 3; ++idx)
    ui->naluSizeLength->addItem(QString{}, idx * 2);

  for (auto idx = 0; idx < 3; ++idx)
    ui->defaultTrackFlag->addItem(QString{}, idx);

  for (auto idx = 0; idx < 2; ++idx)
    ui->forcedTrackFlag->addItem(QString{}, idx);

  for (auto idx = 0; idx < 3; ++idx)
    ui->compression->addItem(QString{}, idx);

  for (auto idx = 0; idx < 4; ++idx)
    ui->cues->addItem(QString{}, idx);

  for (auto idx = 0; idx < 3; ++idx)
    ui->aacIsSBR->addItem(QString{}, idx);

  for (auto const &control : m_comboBoxControls)
    control->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

  // "files" context menu
  ui->files->addAction(m_addFilesAction);
  ui->files->addAction(m_appendFilesAction);
  ui->files->addAction(m_addAdditionalPartsAction);
  ui->files->addAction(m_removeFilesAction);
  ui->files->addAction(m_removeAllFilesAction);

  // "tracks" context menu
  ui->tracks->addAction(m_selectAllTracksAction);
  ui->tracks->addAction(m_enableAllTracksAction);
  ui->tracks->addAction(m_disableAllTracksAction);

  // Connect signals & slots.
  connect(ui->files,                    &Util::BasicTreeView::filesDropped,         this,          &Tab::addOrAppendDroppedFiles);
  connect(ui->files,                    &Util::BasicTreeView::deletePressed,        this,          &Tab::onRemoveFiles);
  connect(ui->files->selectionModel(),  &QItemSelectionModel::selectionChanged,     this,          &Tab::onFileSelectionChanged);
  connect(ui->files->selectionModel(),  &QItemSelectionModel::selectionChanged,     m_filesModel,  &SourceFileModel::updateSelectionStatus);
  connect(ui->tracks->selectionModel(), &QItemSelectionModel::selectionChanged,     this,          &Tab::onTrackSelectionChanged);
  connect(ui->tracks->selectionModel(), &QItemSelectionModel::selectionChanged,     m_tracksModel, &TrackModel::updateSelectionStatus);
  connect(ui->tracks,                   &Util::BasicTreeView::allSelectedActivated, this,          &Tab::toggleMuxThisForSelectedTracks);
  connect(ui->tracks,                   &QTreeView::doubleClicked,                  this,          &Tab::toggleMuxThisForSelectedTracks);

  connect(m_addFilesAction,             &QAction::triggered,                        this,          &Tab::onAddFiles);
  connect(m_appendFilesAction,          &QAction::triggered,                        this,          &Tab::onAppendFiles);
  connect(m_addAdditionalPartsAction,   &QAction::triggered,                        this,          &Tab::onAddAdditionalParts);
  connect(m_removeFilesAction,          &QAction::triggered,                        this,          &Tab::onRemoveFiles);
  connect(m_removeAllFilesAction,       &QAction::triggered,                        this,          &Tab::onRemoveAllFiles);

  connect(m_selectAllTracksAction,      &QAction::triggered,                        this,          &Tab::selectAllTracks);
  connect(m_enableAllTracksAction,      &QAction::triggered,                        this,          &Tab::enableAllTracks);
  connect(m_disableAllTracksAction,     &QAction::triggered,                        this,          &Tab::disableAllTracks);

  connect(m_filesModel,                 &SourceFileModel::rowsInserted,             this,          &Tab::onFileRowsInserted);
  connect(m_tracksModel,                &TrackModel::rowsInserted,                  this,          &Tab::onTrackRowsInserted);
  connect(m_tracksModel,                &TrackModel::itemChanged,                   this,          &Tab::onTrackItemChanged);

  onTrackSelectionChanged();
  enableTracksActions();
}

void
Tab::setupInputToolTips() {
  Util::setToolTip(ui->files,     QY("Right-click to add, append and remove files"));
  Util::setToolTip(ui->tracks,    QY("Right-click for actions for all tracks"));

  Util::setToolTip(ui->muxThis,   QY("If set to 'no' then the selected tracks will not be copied to the output file."));
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
                   .arg(QY("Multiply this track's timestamps with a factor."))
                   .arg(QY("The value can be given either as a floating point number (e.g. 12.345) or a fraction of integer values (e.g. 123/456)."))
                   .arg(QY("This works well for video and subtitle tracks but should not be used with audio tracks.")));
  Util::setToolTip(ui->defaultDuration,
                   Q("%1 %2")
                   .arg(QY("Forces the default duration or number of frames per second for a track."))
                   .arg(QY("The value can be given either as a floating point number (e.g. 12.345) or a fraction of integer values (e.g. 123/456).")));
  Util::setToolTip(ui->fixBitstreamTimingInfo,
                   Q("%1 %2 %3")
                   .arg(QY("Normally mkvmerge does not change the timing information (frame/field rate) stored in the video bitstream."))
                   .arg(QY("With this option that information is adjusted to match the container's timing information."))
                   .arg(QY("The source for the container's timing information be various things: a value given on the command line with the '--default-duration' option, "
                           "the source container or the video bitstream.")));
  Util::setToolTip(ui->aspectRatio,
                   Q("<p>%1 %2 %3</p><p>%4</p>")
                   .arg(QY("The Matroska container format can store the display width/height for a video track."))
                   .arg(QY("This option tells mkvmerge the display aspect ratio to use when it calculates the display width/height."))
                   .arg(QY("Note that many players don't use the display width/height values directly but only use the ratio given by these values when setting the initial window size."))
                   .arg(QY("The value can be given either as a floating point number (e.g. 12.345) or a fraction of integer values (e.g. 123/456).")));
  Util::setToolTip(ui->displayWidth,
                   Q("<p>%1 %2</p><p>%3</p>")
                   .arg(QY("The Matroska container format can store the display width/height for a video track."))
                   .arg(QY("This parameter is the display width in pixels."))
                   .arg(QY("Note that many players don't use the display width/height values directly but only use the ratio given by these values when setting the initial window size.")));
  Util::setToolTip(ui->displayHeight,
                   Q("<p>%1 %2</p><p>%3</p>")
                   .arg(QY("The Matroska container format can store the display width/height for a video track."))
                   .arg(QY("This parameter is the display height in pixels."))
                   .arg(QY("Note that many players don't use the display width/height values directly but only use the ratio given by these values when setting the initial window size.")));
  Util::setToolTip(ui->cropping,
                   Q("<p>%1 %2</p><p>%3 %4</p><p>%5</p>")
                   .arg(QY("Sets the cropping parameters which tell a player to omit a certain number of pixels on the four sides during playback."))
                   .arg(QY("This must be comma-separated list of four numbers for the cropping to be used at the left, top, right and bottom, e.g. '0,20,0,20'."))
                   .arg(QY("Note that the video content is not modified by this option."))
                   .arg(QY("The values are only stored in the track headers."))
                   .arg(QY("Note also that there are not a lot of players that support the cropping parameters.")));
  Util::setToolTip(ui->stereoscopy,
                   Q("%1 %2")
                   .arg(QY("Sets the stereo mode of the video track to this value."))
                   .arg("If left empty then the track's original stereo mode will be kept or, if it didn't have one, none will be set at all."));
  Util::setToolTip(ui->naluSizeLength,
                   Q("<p>%1 %2 %3</p><p>%4</p>")
                   .arg(QY("Forces the NALU size length to a certain number of bytes."))
                   .arg(QY("It defaults to 4 bytes, but there are files which do not contain a frame or slice that is bigger than 65535 bytes."))
                   .arg(QY("For such files you can use this parameter and decrease the size to 2."))
                   .arg(QY("This parameter is only effective for AVC/h.264 and HEVC/h.265 elementary streams read from AVC/h.264/ ES or HEVC/h.265 ES files, AVIs or Matroska files created with '--engage allow_avc_in_vwf_mode'.")));
  Util::setToolTip(ui->aacIsSBR,
                   Q("%1 %2 %3")
                   .arg(QY("This track contains SBR AAC/HE-AAC/AAC+ data."))
                   .arg(QY("Only needed for AAC input files as SBR AAC cannot be detected automatically for these files."))
                   .arg(QY("Not needed for AAC tracks read from other container formats like MP4 or Matroska files.")));
  Util::setToolTip(ui->reduceToAudioCore,
                   Q("%1 %2")
                   .arg(QY("Drops the lossless extensions from an audio track and keeps only its lossy core."))
                   .arg(QY("This only works with DTS audio tracks.")));
  Util::setToolTip(ui->subtitleCharacterSet,
                   Q("<p>%1 %2</p><p><ol><li>%3</li><li>%4</li></p>")
                   .arg(QY("Selects the character set a subtitle file or chapter information was written with."))
                   .arg(QY("Only needed in certain situations:"))
                   .arg(QY("for subtitle files that do not use a byte order marker (BOM) and that are not encoded in the system's current character set (%1)").arg(Q(g_cc_local_utf8->get_charset())))
                   .arg(QY("for files with chapter information (e.g. OGM, MP4) for which mkvmerge does not detect the encoding correctly")));
  Util::setToolTip(ui->cues,
                   Q("%1 %2")
                   .arg(QY("Selects for which blocks mkvmerge will produce index entries ( = cue entries)."))
                   .arg(QY("\"default\" is a good choice for almost all situations.")));
  Util::setToolTip(ui->additionalTrackOptions,
                   Q("%1 %2 %3")
                   .arg(QY("Free-form edit field for user defined options for this track."))
                   .arg(QY("What you input here is added after all the other options the GUI adds so that you could overwrite any of the GUI's options for this track."))
                   .arg(QY("All occurences of the string \"<TID>\" will be replaced by the track's track ID.")));
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
  enableTracksActions();
}

void
Tab::onFileSelectionChanged() {
  enableFilesActions();
}

void
Tab::onTrackSelectionChanged() {
  Util::enableWidgets(m_allInputControls, false);

  auto selection = ui->tracks->selectionModel()->selection();
  auto numRows   = Util::numSelectedRows(selection);
  if (!numRows)
    return;

  if (1 < numRows) {
    setInputControlValues(nullptr);
    Util::enableWidgets(m_allInputControls, true);
    return;
  }

  Util::enableWidgets(m_typeIndependantControls, true);

  auto idxs = selection[0].indexes();
  if (idxs.isEmpty() || !idxs[0].isValid())
    return;

  auto track = m_tracksModel->fromIndex(idxs[0]);
  if (!track)
    return;

  setInputControlValues(track);

  if (track->isAudio())
    Util::enableWidgets(m_audioControls, true);

  else if (track->isVideo())
    Util::enableWidgets(m_videoControls, true);

  else if (track->isSubtitles())
    Util::enableWidgets(m_subtitleControls, true);

  else if (track->isChapters())
    Util::enableWidgets(m_chapterControls, true);

  if (track->isAppended())
    Util::enableWidgets(m_notIfAppendingControls, false);
}

void
Tab::addOrRemoveEmptyComboBoxItem(bool add) {
  for (auto &comboBox : m_comboBoxControls)
    if (add && comboBox->itemData(0).isValid())
      comboBox->insertItem(0, QY("<do not change>"));
    else if (!add && !comboBox->itemData(0).isValid())
      comboBox->removeItem(0);
}

void
Tab::clearInputControlValues() {
  for (auto comboBox : m_comboBoxControls)
    comboBox->setCurrentIndex(0);

  for (auto control : std::vector<QLineEdit *>{ui->trackName, ui->trackTags, ui->delay, ui->stretchBy, ui->timecodes, ui->displayWidth, ui->displayHeight, ui->cropping, ui->additionalTrackOptions})
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

  if (!track) {
    clearInputControlValues();
    m_currentlySettingInputControlValues = false;
    return;
  }

  Util::setComboBoxIndexIf(ui->muxThis,              [&](QString const &, QVariant const &data) { return data.isValid() && (data.toBool()   == track->m_muxThis);           });
  Util::setComboBoxIndexIf(ui->defaultTrackFlag,     [&](QString const &, QVariant const &data) { return data.isValid() && (data.toUInt()   == track->m_defaultTrackFlag);  });
  Util::setComboBoxIndexIf(ui->forcedTrackFlag,      [&](QString const &, QVariant const &data) { return data.isValid() && (data.toUInt()   == track->m_forcedTrackFlag);   });
  Util::setComboBoxIndexIf(ui->compression,          [&](QString const &, QVariant const &data) { return data.isValid() && (data.toUInt()   == track->m_compression);       });
  Util::setComboBoxIndexIf(ui->cues,                 [&](QString const &, QVariant const &data) { return data.isValid() && (data.toUInt()   == track->m_cues);              });
  Util::setComboBoxIndexIf(ui->stereoscopy,          [&](QString const &, QVariant const &data) { return data.isValid() && (data.toUInt()   == track->m_stereoscopy);       });
  Util::setComboBoxIndexIf(ui->naluSizeLength,       [&](QString const &, QVariant const &data) { return data.isValid() && (data.toUInt()   == track->m_naluSizeLength);    });
  Util::setComboBoxIndexIf(ui->aacIsSBR,             [&](QString const &, QVariant const &data) { return data.isValid() && (data.toUInt()   == track->m_aacIsSBR);          });

  Util::setComboBoxTextByData(ui->trackLanguage,        track->m_language);
  Util::setComboBoxTextByData(ui->subtitleCharacterSet, track->m_characterSet);

  ui->trackName->setText(                track->m_name);
  ui->trackTags->setText(                track->m_tags);
  ui->delay->setText(                    track->m_delay);
  ui->stretchBy->setText(                track->m_stretchBy);
  ui->timecodes->setText(                track->m_timecodes);
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
Tab::withSelectedTracks(std::function<void(Track *)> code,
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
  bool withAll       = m_typeIndependantControls.contains(widget);

  for (auto &track : tracks) {
    if (track->m_appendedTo && notIfAppending)
      continue;

    if (   withAll
        || (track->isAudio()     && withAudio)
        || (track->isVideo()     && withVideo)
        || (track->isSubtitles() && withSubtitles)
        || (track->isChapters()  && withChapters)) {
      code(track);
      m_tracksModel->trackUpdated(track);
    }
  }
}

void
Tab::onTrackNameEdited(QString newValue) {
  withSelectedTracks([&](Track *track) { track->m_name = newValue; }, true);
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

  auto newMuxThis = item->checkState() == Qt::Checked;
  if (newMuxThis == track->m_muxThis)
    return;

  track->m_muxThis = newMuxThis;
  m_tracksModel->trackUpdated(track);

  auto selected = selectedTracks();
  if ((1 == selected.count()) && selected.contains(track))
    Util::setComboBoxIndexIf(ui->muxThis, [newMuxThis](QString const &, QVariant const &data) { return data.isValid() && (data.toBool() == newMuxThis); });

  setOutputFileNameMaybe(ui->output->text());
}

void
Tab::onMuxThisChanged(int selected) {
  auto data = ui->muxThis->itemData(selected);
  if (!data.isValid())
    return;
  auto muxThis = data.toBool();

  withSelectedTracks([&](Track *track) { track->m_muxThis = muxThis; });

  setOutputFileNameMaybe(ui->output->text());
}

void
Tab::toggleMuxThisForSelectedTracks() {
  auto allEnabled     = true;
  auto tracksSelected = false;

  withSelectedTracks([&allEnabled, &tracksSelected](Track *track) {
    tracksSelected = true;

    if (!track->m_muxThis)
      allEnabled = false;
  }, false, ui->muxThis);

  if (!tracksSelected)
    return;

  auto newEnabled = !allEnabled;

  withSelectedTracks([newEnabled](Track *track) { track->m_muxThis = newEnabled; }, false, ui->muxThis);

  Util::setComboBoxIndexIf(ui->muxThis, [&](QString const &, QVariant const &data) { return data.isValid() && (data.toBool() == newEnabled); });
}

void
Tab::onTrackLanguageChanged(int newValue) {
  auto code = ui->trackLanguage->itemData(newValue).toString();
  if (code.isEmpty())
    return;

  withSelectedTracks([&](Track *track) { track->m_language = code; }, true);
}

void
Tab::onDefaultTrackFlagChanged(int newValue) {
  auto data = ui->defaultTrackFlag->itemData(newValue);
  if (!data.isValid())
    return;
  newValue = data.toInt();

  withSelectedTracks([&](Track *track) {
    track->m_defaultTrackFlag = newValue;
    if (1 == newValue)
      ensureOneDefaultFlagOnly(track);
  }, true);
}

void
Tab::onForcedTrackFlagChanged(int newValue) {
  auto data = ui->forcedTrackFlag->itemData(newValue);
  if (!data.isValid())
    return;
  newValue = data.toInt();

  withSelectedTracks([&](Track *track) { track->m_forcedTrackFlag = newValue; }, true);
}

void
Tab::onCompressionChanged(int newValue) {
  auto data = ui->compression->itemData(newValue);
  if (!data.isValid())
    return;
  newValue = data.toInt();

  Track::Compression compression;
  if (3 > newValue)
    compression = 0 == newValue ? Track::CompDefault
                : 1 == newValue ? Track::CompNone
                :                 Track::CompZlib;

  withSelectedTracks([&](Track *track) { track->m_compression = compression; }, true);
}

void
Tab::onTrackTagsEdited(QString newValue) {
  withSelectedTracks([&](Track *track) { track->m_tags = newValue; }, true);
}

void
Tab::onDelayEdited(QString newValue) {
  withSelectedTracks([&](Track *track) { track->m_delay = newValue; });
}

void
Tab::onStretchByEdited(QString newValue) {
  withSelectedTracks([&](Track *track) { track->m_stretchBy = newValue; });
}

void
Tab::onDefaultDurationEdited(QString newValue) {
  withSelectedTracks([&](Track *track) { track->m_defaultDuration = newValue; }, true);
}

void
Tab::onTimecodesEdited(QString newValue) {
  withSelectedTracks([&](Track *track) { track->m_timecodes = newValue; });
}

void
Tab::onBrowseTimecodes() {
  auto fileName = getOpenFileName(QY("Select timecode file"), QY("Text files") + Q(" (*.txt)"), ui->timecodes);
  if (!fileName.isEmpty())
    withSelectedTracks([&](Track *track) { track->m_timecodes = fileName; });
}

void
Tab::onFixBitstreamTimingInfoChanged(bool newValue) {
  withSelectedTracks([&](Track *track) { track->m_fixBitstreamTimingInfo = newValue; }, true);
}

void
Tab::onBrowseTrackTags() {
  auto fileName = getOpenFileName(QY("Select tags file"), QY("XML tag files") + Q(" (*.xml)"), ui->trackTags);
  if (!fileName.isEmpty())
    withSelectedTracks([&](Track *track) { track->m_tags = fileName; }, true);
}

void
Tab::onSetAspectRatio() {
  withSelectedTracks([&](Track *track) { track->m_setAspectRatio = true; }, true);
}

void
Tab::onSetDisplayDimensions() {
  withSelectedTracks([&](Track *track) { track->m_setAspectRatio = false; }, true);
}

void
Tab::onAspectRatioEdited(QString newValue) {
  withSelectedTracks([&](Track *track) { track->m_aspectRatio = newValue; }, true);
}

void
Tab::onDisplayWidthEdited(QString newValue) {
  withSelectedTracks([&](Track *track) { track->m_displayWidth = newValue; }, true);
}

void
Tab::onDisplayHeightEdited(QString newValue) {
  withSelectedTracks([&](Track *track) { track->m_displayHeight = newValue; }, true);
}

void
Tab::onStereoscopyChanged(int newValue) {
  auto data = ui->stereoscopy->itemData(newValue);
  if (!data.isValid())
    return;
  newValue = data.toInt();

  withSelectedTracks([&](Track *track) { track->m_stereoscopy = newValue; }, true);
}

void
Tab::onNaluSizeLengthChanged(int newValue) {
  auto data = ui->naluSizeLength->itemData(newValue);
  if (!data.isValid())
    return;
  newValue = data.toInt();

  withSelectedTracks([&](Track *track) { track->m_naluSizeLength = newValue; }, true);
}

void
Tab::onCroppingEdited(QString newValue) {
  withSelectedTracks([&](Track *track) { track->m_cropping = newValue; }, true);
}

void
Tab::onAacIsSBRChanged(int newValue) {
  auto data = ui->aacIsSBR->itemData(newValue);
  if (!data.isValid())
    return;
  newValue = data.toInt();

  withSelectedTracks([&](Track *track) { track->m_aacIsSBR = newValue; }, true);
}

void
Tab::onReduceAudioToCoreChanged(bool newValue) {
  withSelectedTracks([&](Track *track) { track->m_reduceAudioToCore = newValue; });
}

void
Tab::onSubtitleCharacterSetChanged(int newValue) {
  auto characterSet = ui->subtitleCharacterSet->itemData(newValue).toString();
  if (characterSet.isEmpty())
    return;

  withSelectedTracks([&](Track *track) { track->m_characterSet = characterSet; }, true);
}

void
Tab::onCuesChanged(int newValue) {
  auto data = ui->cues->itemData(newValue);
  if (!data.isValid())
    return;
  newValue = data.toInt();

  withSelectedTracks([&](Track *track) { track->m_cues = newValue; }, true);
}

void
Tab::onAdditionalTrackOptionsEdited(QString newValue) {
  withSelectedTracks([&](Track *track) { track->m_additionalOptions = newValue; }, true);
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
Tab::addOrAppendFiles(bool append,
                      QStringList const &fileNames,
                      QModelIndex const &sourceFileIdx) {
  QList<SourceFilePtr> identifiedFiles;
  for (auto &fileName : fileNames) {
    Util::FileIdentifier identifier{ this, fileName };
    if (identifier.identify())
      identifiedFiles << identifier.file();
  }

  if (!append)
    PlaylistScanner{this}.checkAddingPlaylists(identifiedFiles);

  if (identifiedFiles.isEmpty())
    return;

  if (m_debugTrackModel) {
    mxinfo(boost::format("### BEFORE adding/appending ###\n"));
    m_config.debugDumpFileList();
    m_config.debugDumpTrackList();
  }

  setDefaultsFromSettingsForAddedFiles(identifiedFiles);

  m_filesModel->addOrAppendFilesAndTracks(sourceFileIdx, identifiedFiles, append);

  if (m_debugTrackModel) {
    mxinfo(boost::format("### AFTER adding/appending ###\n"));
    m_config.debugDumpFileList();
    m_config.debugDumpTrackList();
  }

  reinitFilesTracksControls();

  setTitleMaybe(identifiedFiles);
  setOutputFileNameMaybe(identifiedFiles[0]->m_fileName);
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

      if (track->m_defaultTrackFlagWasSet && !defaultFlagSet[track->m_type]) {
        track->m_defaultTrackFlag     = 1;
        defaultFlagSet[track->m_type] = true;
      }
    }
}

QStringList
Tab::selectFilesToAdd(QString const &title) {
  QFileDialog dlg{this};
  dlg.setNameFilters(Util::FileTypeFilter::get());
  dlg.setFileMode(QFileDialog::ExistingFiles);
  dlg.setDirectory(Util::Settings::get().m_lastOpenDir);
  dlg.setWindowTitle(title);
  dlg.setOptions(QFileDialog::HideNameFilterDetails);

  if (!dlg.exec())
    return QStringList{};

  Util::Settings::get().m_lastOpenDir = dlg.directory();

  return dlg.selectedFiles();
}

void
Tab::onAddAdditionalParts() {
  auto currentIdx = selectedSourceFile();
  auto sourceFile = m_filesModel->fromIndex(currentIdx);
  if (sourceFile && !sourceFile->m_tracks.size()) {
    Util::MessageBox::critical(this, QY("Unable to append files"), QY("You cannot add additional parts to files that don't contain tracks."));
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

  if (!m_filesModel->rowCount())
    clearDestinationMaybe();
}

void
Tab::onRemoveAllFiles() {
  if (m_config.m_files.isEmpty())
    return;

  m_filesModel->removeRows(0, m_filesModel->rowCount());
  m_tracksModel->removeRows(0, m_tracksModel->rowCount());
  m_config.m_files.clear();
  m_config.m_tracks.clear();

  reinitFilesTracksControls();
  clearDestinationMaybe();
}

void
Tab::reinitFilesTracksControls() {
  resizeFilesColumnsToContents();
  resizeTracksColumnsToContents();
  onFileSelectionChanged();
  onTrackSelectionChanged();
  enableFilesActions();
  enableTracksActions();
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
Tab::enableFilesActions() {
  int numSelected      = ui->files->selectionModel()->selection().size();
  bool hasRegularTrack = false;
  if (1 == numSelected)
    hasRegularTrack = m_config.m_files.end() != brng::find_if(m_config.m_files, [](SourceFilePtr const &file) { return file->hasRegularTrack(); });

  m_addFilesAction->setEnabled(true);
  m_appendFilesAction->setEnabled((1 == numSelected) && hasRegularTrack);
  m_addAdditionalPartsAction->setEnabled(1 == numSelected);
  m_removeFilesAction->setEnabled(0 < numSelected);
  m_removeAllFilesAction->setEnabled(!m_config.m_files.isEmpty());
}

void
Tab::enableTracksActions() {
  bool hasTracks = !!m_tracksModel->rowCount();

  m_selectAllTracksAction->setEnabled(hasTracks);
  m_enableAllTracksAction->setEnabled(hasTracks);
  m_disableAllTracksAction->setEnabled(hasTracks);
}

void
Tab::retranslateInputUI() {
  m_filesModel->retranslateUi();
  m_tracksModel->retranslateUi();

  resizeFilesColumnsToContents();
  resizeTracksColumnsToContents();

  m_addFilesAction->setText(QY("&Add files"));
  m_appendFilesAction->setText(QY("A&ppend files"));
  m_addAdditionalPartsAction->setText(QY("Add files as a&dditional parts"));
  m_removeFilesAction->setText(QY("&Remove files"));
  m_removeAllFilesAction->setText(QY("Remove a&ll files"));

  m_selectAllTracksAction->setText(QY("&Select all tracks"));
  m_enableAllTracksAction->setText(QY("&Enable all tracks"));
  m_disableAllTracksAction->setText(QY("&Disable all tracks"));

  for (auto &comboBox : m_comboBoxControls)
    if (comboBox->count() && !comboBox->itemData(0).isValid())
      comboBox->setItemText(0, QY("<do not change>"));

  Util::setComboBoxTexts(ui->muxThis,          QStringList{} << QY("yes")                     << QY("no"));
  Util::setComboBoxTexts(ui->naluSizeLength,   QStringList{} << QY("don't change")            << QY("force 2 bytes")        << QY("force 4 bytes"));
  Util::setComboBoxTexts(ui->defaultTrackFlag, QStringList{} << QY("determine automatically") << QY("set to \"yes\"")       << QY("set to \"no\""));
  Util::setComboBoxTexts(ui->forcedTrackFlag,  QStringList{} << QY("off")                     << QY("on"));
  Util::setComboBoxTexts(ui->compression,      QStringList{} << QY("determine automatically") << QY("no extra compression") << Q("zlib"));
  Util::setComboBoxTexts(ui->cues,             QStringList{} << QY("default")                 << QY("only for I frames")    << QY("for all frames") << QY("no cues"));
  Util::setComboBoxTexts(ui->aacIsSBR,         QStringList{} << QY("default")                 << QY("yes")                  << QY("no"));

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
    setTitleMaybe(file->m_properties["title"]);

    if (FILE_TYPE_OGM != file->m_type)
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
Tab::setOutputFileNameMaybe(QString const &fileName) {
  auto &settings = Util::Settings::get();
  auto policy    = settings.m_outputFileNamePolicy;

  if (fileName.isEmpty() || (Util::Settings::DontSetOutputFileName == policy))
    return;

  auto currentOutput = ui->output->text();
  auto srcFileName   = QFileInfo{ currentOutput.isEmpty() ? fileName : currentOutput };
  QDir outputDir;

  if (Util::Settings::ToPreviousDirectory == policy)
    outputDir = settings.m_lastOutputDir;

  else if (Util::Settings::ToFixedDirectory == policy)
    outputDir = settings.m_fixedOutputDir;

  else if (Util::Settings::ToSameAsFirstInputFile == policy)
    outputDir = srcFileName.absoluteDir();

  else if (Util::Settings::ToRelativeOfFirstInputFile == policy)
    outputDir = QDir{ srcFileName.absoluteDir().path() + Q("/") + settings.m_relativeOutputDir.path() };

  else
    Q_ASSERT_X(false, "setOutputFileNameMaybe", "Untested output file name policy");

  auto baseName = srcFileName.completeBaseName().replace(QRegularExpression{" \\(\\d+\\)$"}, Q(""));
  auto idx      = 0;

  while (true) {
    auto suffix          = suggestOutputFileNameExtension();
    auto currentBaseName = QString{"%1%2.%3"}.arg(baseName).arg(idx ? QString{" (%1)"}.arg(idx) : "").arg(suffix);
    auto outputFileName  = QFileInfo{outputDir, currentBaseName};

    if (!settings.m_uniqueOutputFileNames || !outputFileName.exists()) {
      ui->output->setText(outputFileName.absoluteFilePath());
      setDestination(outputFileName.absoluteFilePath());
      break;
    }

    ++idx;
  }
}

void
Tab::addOrAppendDroppedFiles(QStringList const &fileNames) {
  if (m_config.m_files.isEmpty() || Util::Settings::get().m_mergeAlwaysAddDroppedFiles) {
    addOrAppendFiles(false, fileNames, {});
    return;
  }

  AddingAppendingFilesDialog dlg{this, m_config.m_files};
  if (!dlg.exec())
    return;

  auto decision = dlg.decision();
  auto fileIdx  = m_filesModel->index(dlg.fileIndex(), 0);

  if (AddingAppendingFilesDialog::Decision::AddAdditionalParts == decision)
    m_filesModel->addAdditionalParts(fileIdx, fileNames);

  else
    addOrAppendFiles(AddingAppendingFilesDialog::Decision::Append == decision, fileNames, fileIdx);
}

void
Tab::addOrAppendDroppedFilesDelayed() {
  addOrAppendDroppedFiles(m_filesToAddDelayed);
  m_filesToAddDelayed.clear();
}

void
Tab::addFilesToBeAddedOrAppendedDelayed(QStringList const &fileNames) {
  m_filesToAddDelayed += fileNames;
  QTimer::singleShot(0, this, SLOT(addOrAppendDroppedFilesDelayed()));
}

void
Tab::selectAllTracks() {
  auto numRows = m_tracksModel->rowCount();
  if (!numRows)
    return;

  auto numColumns = m_tracksModel->columnCount() - 1;
  auto selection  = QItemSelection{m_tracksModel->index(0, 0), m_tracksModel->index(numRows - 1, numColumns - 1)};

  for (auto row = 0; row < numRows; ++row) {
    auto &track      = *m_config.m_tracks[row];
    auto numAppended = track.m_appendedTracks.count();

    if (!numAppended)
      continue;

    auto rowIdx = m_tracksModel->index(row, 0);
    selection.select(m_tracksModel->index(0, 0, rowIdx), m_tracksModel->index(numAppended - 1, numColumns - 1, rowIdx));
  }

  ui->tracks->selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect);
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
}

}}}
