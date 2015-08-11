#ifndef MTX_MKVTOOLNIX_GUI_MERGE_TAB_H
#define MTX_MKVTOOLNIX_GUI_MERGE_TAB_H

#include "common/common_pch.h"

#include "mkvtoolnix-gui/main_window/tool_base.h"
#include "mkvtoolnix-gui/merge/attachment_model.h"
#include "mkvtoolnix-gui/merge/mux_config.h"
#include "mkvtoolnix-gui/merge/source_file_model.h"
#include "mkvtoolnix-gui/merge/track_model.h"

#include <QList>

class QComboBox;
class QLineEdit;
class QMenu;
class QTreeView;

namespace mtx { namespace gui { namespace Merge {

namespace Ui {
class Tab;
}

class Tab : public QWidget {
  Q_OBJECT;

protected:
  // non-UI stuff:
  MuxConfig m_config;

  // UI stuff:
  std::unique_ptr<Ui::Tab> ui;
  QStringList m_filesToAddDelayed;

  // "Input" tab:
  SourceFileModel *m_filesModel;
  TrackModel *m_tracksModel;

  QList<QWidget *> m_audioControls, m_videoControls, m_subtitleControls, m_chapterControls, m_typeIndependantControls, m_allInputControls, m_splitControls, m_notIfAppendingControls;
  QList<QComboBox *> m_comboBoxControls;
  bool m_currentlySettingInputControlValues;

  QAction *m_addFilesAction, *m_appendFilesAction, *m_addAdditionalPartsAction, *m_removeFilesAction, *m_removeAllFilesAction, *m_selectAllTracksAction, *m_enableAllTracksAction, *m_disableAllTracksAction;
  QAction *m_selectAllVideoTracksAction, *m_selectAllAudioTracksAction, *m_selectAllSubtitlesTracksAction;
  QMenu *m_filesMenu, *m_tracksMenu, *m_attachmentsMenu, *m_selectTracksOfTypeMenu;

  // "Attachments" tab:
  AttachmentModel *m_attachmentsModel;
  QAction *m_addAttachmentsAction, *m_removeAttachmentsAction;

  QString m_savedState;

  debugging_option_c m_debugTrackModel;

public:
  explicit Tab(QWidget *parent);
  ~Tab();

  virtual bool hasBeenModified();

  virtual QString const &fileName() const;
  virtual QString title() const;
  virtual void load(QString const &fileName);
  virtual void cloneConfig(MuxConfig const &config);

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
  virtual void onAddToJobQueue();
  virtual void onStartMuxing();
  virtual void onShowCommandLine();

  virtual void selectAllTracks();
  virtual void selectAllVideoTracks();
  virtual void selectAllAudioTracks();
  virtual void selectAllSubtitlesTracks();
  virtual void enableAllTracks();
  virtual void disableAllTracks();

  virtual void toggleMuxThisForSelectedTracks();

  virtual void onTrackSelectionChanged();

  virtual void onMoveFilesUp();
  virtual void onMoveFilesDown();
  virtual void onMoveTracksUp();
  virtual void onMoveTracksDown();
  virtual void onMoveAttachmentsUp();
  virtual void onMoveAttachmentsDown();
  virtual void setupMoveUpDownButtons();
  virtual void enableMoveFilesButtons();

  virtual void onTrackNameEdited(QString newValue);
  virtual void onTrackItemChanged(QStandardItem *item);
  virtual void onMuxThisChanged(int selected);
  virtual void onTrackLanguageChanged(int newValue);
  virtual void onDefaultTrackFlagChanged(int newValue);
  virtual void onForcedTrackFlagChanged(int newValue);
  virtual void onCompressionChanged(int newValue);
  virtual void onTrackTagsEdited(QString newValue);
  virtual void onDelayEdited(QString newValue);
  virtual void onStretchByEdited(QString newValue);
  virtual void onDefaultDurationEdited(QString newValue);
  virtual void onTimecodesEdited(QString newValue);
  virtual void onBrowseTimecodes();
  virtual void onFixBitstreamTimingInfoChanged(bool newValue);
  virtual void onBrowseTrackTags();
  virtual void onSetAspectRatio();
  virtual void onSetDisplayDimensions();
  virtual void onAspectRatioEdited(QString newValue);
  virtual void onDisplayWidthEdited(QString newValue);
  virtual void onDisplayHeightEdited(QString newValue);
  virtual void onStereoscopyChanged(int newValue);
  virtual void onNaluSizeLengthChanged(int newValue);
  virtual void onCroppingEdited(QString newValue);
  virtual void onAacIsSBRChanged(int newValue);
  virtual void onReduceAudioToCoreChanged(bool newValue);
  virtual void onSubtitleCharacterSetChanged(int newValue);
  virtual void onCuesChanged(int newValue);
  virtual void onAdditionalTrackOptionsEdited(QString newValue);

  virtual void resizeFilesColumnsToContents() const;
  virtual void resizeTracksColumnsToContents() const;
  virtual void reinitFilesTracksControls();

  virtual void onFileRowsInserted(QModelIndex const &parentIdx, int first, int last);
  virtual void onTrackRowsInserted(QModelIndex const &parentIdx, int first, int last);
  virtual void addOrAppendDroppedFiles(QStringList const &fileNames);
  virtual void addOrAppendDroppedFilesDelayed();
  virtual void addFilesToBeAddedOrAppendedDelayed(QStringList const &fileNames);

  virtual void showFilesContextMenu(QPoint const &pos);
  virtual void showTracksContextMenu(QPoint const &pos);
  virtual void showAttachmentsContextMenu(QPoint const &pos);

  // Output tab:
  virtual void setupOutputFileControls();
  virtual void setDestination(QString const &newValue);
  virtual void clearDestination();
  virtual void clearDestinationMaybe();
  virtual void clearTitle();
  virtual void clearTitleMaybe();

  virtual void onTitleEdited(QString newValue);
  virtual void onBrowseOutput();
  virtual void onGlobalTagsEdited(QString newValue);
  virtual void onBrowseGlobalTags();
  virtual void onSegmentInfoEdited(QString newValue);
  virtual void onBrowseSegmentInfo();
  virtual void onSplitModeChanged(int newMode);
  virtual void onSplitOptionsEdited(QString newValue);
  virtual void onLinkFilesClicked(bool newValue);
  virtual void onSplitMaxFilesChanged(int newValue);
  virtual void onSegmentUIDsEdited(QString newValue);
  virtual void onPreviousSegmentUIDEdited(QString newValue);
  virtual void onNextSegmentUIDEdited(QString newValue);
  virtual void onChaptersEdited(QString newValue);
  virtual void onBrowseChapters();
  virtual void onChapterLanguageChanged(int newValue);
  virtual void onChapterCharacterSetChanged(QString newValue);
  virtual void onChapterCueNameFormatChanged(QString newValue);
  virtual void onWebmClicked(bool newValue);
  virtual void onAdditionalOptionsEdited(QString newValue);
  virtual void onEditAdditionalOptions();

  // Attachments tab:
  virtual void onAttachmentSelectionChanged();
  virtual void onAttachmentNameEdited(QString newValue);
  virtual void onAttachmentDescriptionEdited(QString newValue);
  virtual void onAttachmentMIMETypeEdited(QString newValue);
  virtual void onAttachmentStyleChanged(int newValue);
  virtual void onAddAttachments();
  virtual void onRemoveAttachments();
  virtual void addAttachments(QStringList const &fileNames);

  virtual void resizeAttachmentsColumnsToContents() const;

  virtual void retranslateUi();
  virtual void handleClearingMergeSettings();
  virtual void setupTabPositions();

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
  virtual void enableAttachmentsActions();
  virtual void enableAttachmentControls(bool enable);
  virtual void setInputControlValues(Track *track);
  virtual void setOutputControlValues();
  virtual void setAttachmentControlValues(Attachment *attachment);
  virtual void clearInputControlValues();
  virtual void setControlValuesFromConfig();
  virtual MuxConfig &updateConfigFromControlValues();
  virtual void withSelectedTracks(std::function<void(Track *)> code, bool notIfAppending = false, QWidget *widget = nullptr);
  virtual void withSelectedAttachments(std::function<void(Attachment *)> code);
  virtual void addOrRemoveEmptyComboBoxItem(bool add);
  virtual QString getOpenFileName(QString const &title, QString const &filter, QLineEdit *lineEdit);
  virtual QString getSaveFileName(QString const &title, QString const &filter, QLineEdit *lineEdit);

  virtual void moveSourceFilesUpOrDown(bool up);
  virtual void moveTracksUpOrDown(bool up);
  virtual void moveAttachmentsUpOrDown(bool up);

  virtual void moveOutputFileNameToGlobal();
  virtual void moveOutputFileNameToOutputTab();

  virtual QModelIndex selectedSourceFile() const;
  virtual QList<SourceFile *> selectedSourceFiles() const;
  virtual void selectSourceFiles(QList<SourceFile *> const &files);

  virtual QList<Track *> selectedTracks() const;
  virtual void selectTracks(QList<Track *> const &tracks);
  virtual void selectAllTracksOfType(boost::optional<Track::Type> type);

  virtual QList<Attachment *> selectedAttachments() const;
  virtual void selectAttachments(QList<Attachment *> const &attachments);

  virtual void addToJobQueue(bool startNow);

  virtual bool isReadyForMerging();

  virtual void setTitleMaybe(QList<SourceFilePtr> const &files);
  virtual void setTitleMaybe(QString const &title);

  virtual void setOutputFileNameMaybe();
  virtual QString suggestOutputFileNameExtension() const;

  virtual void enableDisableAllTracks(bool enable);

  virtual void ensureOneDefaultFlagOnly(Track *thisOneHasIt);

  virtual QString currentState();
};

}}}

#endif // MTX_MKVTOOLNIX_GUI_MERGE_TAB_H
