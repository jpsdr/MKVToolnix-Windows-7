#pragma once

#include "common/common_pch.h"

#include "mkvtoolnix-gui/main_window/tool_base.h"
#include "mkvtoolnix-gui/merge/attached_file_model.h"
#include "mkvtoolnix-gui/merge/attachment_model.h"
#include "mkvtoolnix-gui/merge/mux_config.h"
#include "mkvtoolnix-gui/merge/source_file_model.h"
#include "mkvtoolnix-gui/merge/track_model.h"
#include "mkvtoolnix-gui/util/settings.h"

#include <QList>

class QComboBox;
class QLineEdit;
class QMenu;
class QTreeView;

namespace mtx { namespace gui { namespace Merge {

namespace Ui {
class Tab;
}

enum class InitialDirMode {
    ContentLastOpenDir
  , ContentFirstInputFileLastOpenDir
};

class FileIdentificationThread;

class Tab : public QWidget {
  Q_OBJECT;

protected:
  // non-UI stuff:
  MuxConfig m_config;

  int m_lastAddAppendFileIdx;

  // UI stuff:
  std::unique_ptr<Ui::Tab> ui;
  QStringList m_filesToAddDelayed;
  Qt::MouseButtons m_mouseButtonsForFilesToAddDelayed;

  // "Input" tab:
  SourceFileModel *m_filesModel;
  TrackModel *m_tracksModel;

  QList<QWidget *> m_audioControls, m_videoControls, m_subtitleControls, m_chapterControls, m_typeIndependentControls, m_allInputControls, m_splitControls, m_notIfAppendingControls;
  QList<QComboBox *> m_comboBoxControls;
  bool m_currentlySettingInputControlValues;

  QAction *m_addFilesAction, *m_appendFilesAction, *m_addAdditionalPartsAction, *m_addFilesAction2, *m_appendFilesAction2, *m_addAdditionalPartsAction2;
  QAction *m_removeFilesAction, *m_removeAllFilesAction, *m_setDestinationFileNameAction, *m_selectAllTracksAction, *m_enableAllTracksAction, *m_disableAllTracksAction;
  QAction *m_selectAllVideoTracksAction, *m_selectAllAudioTracksAction, *m_selectAllSubtitlesTracksAction, *m_openFilesInMediaInfoAction, *m_openTracksInMediaInfoAction, *m_selectTracksFromFilesAction;
  QAction *m_enableAllAttachedFilesAction, *m_disableAllAttachedFilesAction, *m_enableSelectedAttachedFilesAction, *m_disableSelectedAttachedFilesAction;
  QAction *m_startMuxingLeaveAsIs, *m_startMuxingCreateNewSettings, *m_startMuxingCloseSettings, *m_startMuxingRemoveInputFiles;
  QAction *m_addToJobQueueLeaveAsIs, *m_addToJobQueueCreateNewSettings, *m_addToJobQueueCloseSettings, *m_addToJobQueueRemoveInputFiles;
  QMenu *m_filesMenu, *m_tracksMenu, *m_attachedFilesMenu, *m_attachmentsMenu, *m_selectTracksOfTypeMenu, *m_addFilesMenu, *m_startMuxingMenu, *m_addToJobQueueMenu;

  // "Attachments" tab:
  AttachedFileModel *m_attachedFilesModel;
  AttachmentModel *m_attachmentsModel;
  QAction *m_addAttachmentsAction, *m_removeAttachmentsAction, *m_removeAllAttachmentsAction, *m_selectAllAttachmentsAction;

  QString m_savedState, m_emptyState;

  FileIdentificationThread *m_identifier;

  debugging_option_c m_debugTrackModel;

public:
  explicit Tab(QWidget *parent);
  ~Tab();

  virtual bool hasBeenModified();
  virtual bool isEmpty();
  virtual bool hasSourceFiles() const;
  virtual bool hasDestinationFileName() const;
  virtual bool hasTitle() const;

  virtual QString const &fileName() const;
  virtual QString title() const;
  virtual void load(QString const &fileName);
  virtual void cloneConfig(MuxConfig const &config);
  virtual void addFiles(QStringList const &fileNames);

signals:
  void removeThisTab();
  void titleChanged();

public slots:
  // Input tab:
  virtual void onSaveConfig();
  virtual void onSaveConfigAs();
  virtual void onSaveOptionFile();
  virtual void onAddFiles();
  virtual void onAddAdditionalParts();
  virtual void onAppendFiles();
  virtual void onRemoveFiles();
  virtual void onRemoveAllFiles();
  virtual void onShowCommandLine();

  virtual void selectAllTracks();
  virtual void selectAllVideoTracks();
  virtual void selectAllAudioTracks();
  virtual void selectAllSubtitlesTracks();
  virtual void selectAllTracksFromSelectedFiles();
  virtual void enableAllTracks();
  virtual void disableAllTracks();

  virtual void toggleMuxThisForSelectedTracks();
  virtual void toggleMuxThisForSelectedAttachedFiles();

  virtual void onOpenFilesInMediaInfo();
  virtual void onOpenTracksInMediaInfo();

  virtual void onTrackSelectionChanged();

  virtual void onMoveFilesUp();
  virtual void onMoveFilesDown();
  virtual void onMoveTracksUp();
  virtual void onMoveTracksDown();
  virtual void onMoveAttachmentsUp();
  virtual void onMoveAttachmentsDown();
  virtual void setupMoveUpDownButtons();
  virtual void enableMoveFilesButtons();

  virtual void setupInputLayout();
  virtual void setupFileIdentificationThread();

  virtual void onTrackNameChanged(QString newValue);
  virtual void onTrackItemChanged(QStandardItem *item);
  virtual void onMuxThisChanged(int selected);
  virtual void onTrackLanguageChanged(int newValue);
  virtual void onDefaultTrackFlagChanged(int newValue);
  virtual void onForcedTrackFlagChanged(int newValue);
  virtual void onCompressionChanged(int newValue);
  virtual void onTrackTagsChanged(QString newValue);
  virtual void onDelayChanged(QString newValue);
  virtual void onStretchByChanged(QString newValue);
  virtual void onDefaultDurationChanged(QString newValue);
  virtual void onTimestampsChanged(QString newValue);
  virtual void onBrowseTimestamps();
  virtual void onFixBitstreamTimingInfoChanged(bool newValue);
  virtual void onBrowseTrackTags();
  virtual void onSetAspectRatio();
  virtual void onSetDisplayDimensions();
  virtual void onAspectRatioChanged(QString newValue);
  virtual void onDisplayWidthChanged(QString newValue);
  virtual void onDisplayHeightChanged(QString newValue);
  virtual void onStereoscopyChanged(int newValue);
  virtual void onNaluSizeLengthChanged(int newValue);
  virtual void onCroppingChanged(QString newValue);
  virtual void onAacIsSBRChanged(int newValue);
  virtual void onReduceAudioToCoreChanged(bool newValue);
  virtual void onSubtitleCharacterSetChanged(int newValue);
  virtual void onCuesChanged(int newValue);
  virtual void onAdditionalTrackOptionsChanged(QString newValue);
  virtual void onPreviewChapterCharacterSet();
  virtual void setChapterCharacterSet(QString const &characterSet);
  virtual void onCopyFirstFileNameToTitle();
  virtual void onCopyOutputFileNameToTitle();
  virtual void onCopyTitleToOutputFileName();
  virtual void setDestinationFileNameFromSelectedFile();

  virtual void addToJobQueue(bool startNow, boost::optional<Util::Settings::ClearMergeSettingsAction> clearSettings = boost::none);

  virtual void resizeFilesColumnsToContents() const;
  virtual void resizeTracksColumnsToContents() const;
  virtual void reinitFilesTracksControls();

  virtual void onFileRowsInserted(QModelIndex const &parentIdx, int first, int last);
  virtual void onTrackRowsInserted(QModelIndex const &parentIdx, int first, int last);
  virtual void addOrAppendDroppedFiles(QStringList const &fileNamesToAddOrAppend, Qt::MouseButtons mouseButtons);
  virtual void addOrAppendDroppedFilesDelayed();
  virtual void addFilesToBeAddedOrAppendedDelayed(QStringList const &fileNames, Qt::MouseButtons mouseButtons);
  virtual void addOrAppendIdentifiedFiles(QList<SourceFilePtr> const &identifiedFiles, bool append, QModelIndex const &sourceFileIdx);

  virtual void showFilesContextMenu(QPoint const &pos);
  virtual void showTracksContextMenu(QPoint const &pos);
  virtual void showAttachedFilesContextMenu(QPoint const &pos);
  virtual void showAttachmentsContextMenu(QPoint const &pos);

  virtual void fileIdentificationStarted();
  virtual void fileIdentificationFinished();
  virtual void handleIdentifiedXmlOrSimpleChapters(QString const &fileName);
  virtual void handleIdentifiedXmlSegmentInfo(QString const &fileName);
  virtual void handleIdentifiedXmlTags(QString const &fileName);
  virtual void showFileIdentificationError(QString const &errorTitle, QString const &errorText);
  virtual void showScanningPlaylistDialog(int numFilesToScan);
  virtual void selectScanPlaylistPolicy(SourceFilePtr const &sourceFile, QFileInfoList const &files);
  virtual void selectPlaylistToAdd(QList<SourceFilePtr> const &identifiedPlaylists);

  // Output tab:
  virtual void setupOutputFileControls();
  virtual void setDestination(QString const &newValue);
  virtual void clearDestination();
  virtual void clearDestinationMaybe();
  virtual void clearTitle();
  virtual void clearTitleMaybe();

  virtual void onTitleChanged(QString newValue);
  virtual void onBrowseOutput();
  virtual void onGlobalTagsChanged(QString newValue);
  virtual void onBrowseGlobalTags();
  virtual void onSegmentInfoChanged(QString newValue);
  virtual void onBrowseSegmentInfo();
  virtual void onSplitModeChanged(int newMode);
  virtual void onSplitOptionsChanged(QString newValue);
  virtual void onLinkFilesClicked(bool newValue);
  virtual void onSplitMaxFilesChanged(int newValue);
  virtual void onSegmentUIDsChanged(QString newValue);
  virtual void onPreviousSegmentUIDChanged(QString newValue);
  virtual void onNextSegmentUIDChanged(QString newValue);
  virtual void onChaptersChanged(QString newValue);
  virtual void onBrowseSegmentUID();
  virtual void onBrowsePreviousSegmentUID();
  virtual void onBrowseNextSegmentUID();
  virtual void onBrowseChapters();
  virtual void onChapterLanguageChanged(int newValue);
  virtual void onChapterCharacterSetChanged(QString newValue);
  virtual void onChapterCueNameFormatChanged(QString newValue);
  virtual void onWebmClicked(bool newValue);
  virtual void onAdditionalOptionsChanged(QString newValue);
  virtual void onEditAdditionalOptions();
  virtual void onPreviewSubtitleCharacterSet();
  virtual void setSubtitleCharacterSet(QString const &characterSet);
  virtual void onChapterGenerationModeChanged();
  virtual void onChapterGenerationNameTemplateChanged();
  virtual void onChapterGenerationIntervalChanged();

  // Attachments tab:
  virtual void onAttachmentSelectionChanged();
  virtual void onAttachmentNameChanged(QString newValue);
  virtual void onAttachmentDescriptionChanged(QString newValue);
  virtual void onAttachmentMIMETypeChanged(QString newValue);
  virtual void onAttachmentStyleChanged(int newValue);
  virtual void onAddAttachments();
  virtual void onRemoveAttachments();
  virtual void onRemoveAllAttachments();
  virtual void onSelectAllAttachments();
  virtual void addAttachments(QStringList const &fileNames);

  virtual void enableDisableAllAttachedFiles(bool enable);
  virtual void enableDisableSelectedAttachedFiles(bool enable);
  virtual void attachedFileItemChanged(QStandardItem *item);

  virtual void resizeAttachedFilesColumnsToContents() const;
  virtual void resizeAttachmentsColumnsToContents() const;

  virtual void retranslateUi();
  virtual void handleClearingMergeSettings(Util::Settings::ClearMergeSettingsAction action);
  virtual void setupTabPositions();

  virtual void signalRemovalOfThisTab();

protected:
  virtual void setupAttachmentsControls();
  virtual void setupControlLists();
  virtual void setupInputControls();
  virtual void setupOutputControls();

  virtual void setupInputToolTips();
  virtual void setupOutputToolTips();
  virtual void setupAttachmentsToolTips();

  virtual void retranslateInputUI();
  virtual void retranslateOutputUI();
  virtual void retranslateAttachmentsUI();

  virtual QStringList selectFilesToAdd(QString const &title);
  virtual QStringList selectAttachmentsToAdd();
  virtual void addOrAppendFiles(bool append);
  virtual void addOrAppendFiles(bool append, QStringList const &fileNames, QModelIndex const &sourceFileIdx);
  virtual void setDefaultsFromSettingsForAddedFiles(QList<SourceFilePtr> const &files);

  virtual void enableFilesActions();
  virtual void enableTracksActions();
  virtual void enableAttachedFilesActions();
  virtual void enableAttachmentsActions();
  virtual void enableAttachmentControls(bool enable);
  virtual void setInputControlValues(Track *track);
  virtual void setOutputControlValues();
  virtual void setAttachmentControlValues(Attachment *attachment);
  virtual void clearInputControlValues();
  virtual void setControlValuesFromConfig();
  virtual MuxConfig &updateConfigFromControlValues();
  virtual void withSelectedTracks(std::function<void(Track &)> code, bool notIfAppending = false, QWidget *widget = nullptr);
  virtual void withSelectedAttachedFiles(std::function<void(Track &)> code);
  virtual void withSelectedAttachments(std::function<void(Attachment &)> code);
  virtual void addOrRemoveEmptyComboBoxItem(bool add);
  virtual QString getOpenFileName(QString const &title, QString const &filter, QLineEdit *lineEdit, InitialDirMode intialDirMode = InitialDirMode::ContentLastOpenDir);
  virtual QString getSaveFileName(QString const &title, QString const &filter, QLineEdit *lineEdit);
  virtual QString determineInitialDir(QLineEdit *lineEdit, InitialDirMode mode) const;

  virtual void moveSourceFilesUpOrDown(bool up);
  virtual void moveTracksUpOrDown(bool up);
  virtual void moveAttachmentsUpOrDown(bool up);

  virtual void setupHorizontalScrollAreaInputLayout();
  virtual void setupHorizontalTwoColumnsInputLayout();
  virtual void setupVerticalTabWidgetInputLayout();

  virtual void moveOutputFileNameToGlobal();
  virtual void moveOutputFileNameToOutputTab();

  virtual QModelIndex selectedSourceFile() const;
  virtual QList<SourceFile *> selectedSourceFiles() const;
  virtual void selectSourceFiles(QList<SourceFile *> const &files);

  virtual QList<Track *> selectedTracks() const;
  virtual void selectTracks(QList<Track *> const &tracks);
  virtual void selectAllTracksOfType(boost::optional<Track::Type> type);

  virtual QList<Track *> selectedAttachedFiles() const;
  virtual QList<Attachment *> selectedAttachments() const;
  virtual void selectAttachments(QList<Attachment *> const &attachments);
  virtual boost::optional<QString> findExistingAttachmentFileName(QString const &fileName);

  virtual bool isReadyForMerging();
  virtual bool checkIfOverwritingIsOK();
  virtual QString findExistingDestination() const;

  virtual void setTitleMaybe(QList<SourceFilePtr> const &files);
  virtual void setTitleMaybe(QString const &title);

  virtual void setOutputFileNameMaybe(bool force = false);
  virtual QString suggestOutputFileNameExtension() const;

  virtual void enableDisableAllTracks(bool enable);

  virtual void ensureOneDefaultFlagOnly(Track *thisOneHasIt);

  virtual QString currentState();

  virtual QString mediaInfoLocation();
  virtual void openFilesInMediaInfo(QStringList const &fileNames);

  virtual void addSegmentUIDFromFile(QLineEdit &lineEdit, bool append);
};

}}}
