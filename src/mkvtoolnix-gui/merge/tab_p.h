#pragma once

#include "common/common_pch.h"

#include "mkvtoolnix-gui/merge/attached_file_model.h"
#include "mkvtoolnix-gui/merge/attachment_model.h"
#include "mkvtoolnix-gui/merge/mux_config.h"
#include "mkvtoolnix-gui/merge/source_file_model.h"
#include "mkvtoolnix-gui/merge/track_model.h"

class QComboBox;
class QMenu;

namespace mtx::gui::Merge {

class Tab;

namespace Ui {
class Tab;
}

class TabPrivate {
  friend class Tab;

  // non-UI stuff:
  MuxConfig config;

  // UI stuff:
  std::unique_ptr<Ui::Tab> ui;

  // "Input" tab:
  SourceFileModel *filesModel{};
  TrackModel *tracksModel{};

  QList<QWidget *> audioControls, videoControls, subtitleControls, chapterControls, typeIndependentControls, allInputControls, splitControls, notIfAppendingControls;
  QList<QComboBox *> comboBoxControls;
  bool currentlySettingInputControlValues{};

  QAction *addFilesAction{}, *appendFilesAction{}, *addAdditionalPartsAction{}, *addFilesAction2{}, *appendFilesAction2{}, *addAdditionalPartsAction2{};
  QAction *removeFilesAction{}, *removeAllFilesAction{}, *setDestinationFileNameAction{}, *selectAllTracksAction{}, *enableAllTracksAction{}, *disableAllTracksAction{};
  QAction *selectAllVideoTracksAction{}, *selectAllAudioTracksAction{}, *selectAllSubtitlesTracksAction{}, *openFilesInMediaInfoAction{}, *openTracksInMediaInfoAction{}, *selectTracksFromFilesAction{};
  QAction *enableAllAttachedFilesAction{}, *disableAllAttachedFilesAction{}, *enableSelectedAttachedFilesAction{}, *disableSelectedAttachedFilesAction{};
  QAction *startMuxingLeaveAsIs{}, *startMuxingCreateNewSettings{}, *startMuxingCloseSettings{}, *startMuxingRemoveInputFiles{};
  QAction *addToJobQueueLeaveAsIs{}, *addToJobQueueCreateNewSettings{}, *addToJobQueueCloseSettings{}, *addToJobQueueRemoveInputFiles{};
  QMenu *filesMenu{}, *tracksMenu{}, *attachedFilesMenu{}, *attachmentsMenu{}, *selectTracksOfTypeMenu{}, *addFilesMenu{}, *startMuxingMenu{}, *addToJobQueueMenu{};

  // "Attachments" tab:
  AttachedFileModel *attachedFilesModel{};
  AttachmentModel *attachmentsModel{};
  QAction *addAttachmentsAction{}, *removeAttachmentsAction{}, *removeAllAttachmentsAction{}, *selectAllAttachmentsAction{};

  QString savedState, emptyState;

  debugging_option_c debugTrackModel{"track_model"};

public:
  TabPrivate(QWidget *parent);
  virtual ~TabPrivate() = default;
};

}
