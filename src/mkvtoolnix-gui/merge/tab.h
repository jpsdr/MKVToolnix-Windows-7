#ifndef MTX_MKVTOOLNIX_GUI_MERGE_TAB_H
#define MTX_MKVTOOLNIX_GUI_MERGE_TAB_H

#include "common/common_pch.h"

#include "mkvtoolnix-gui/main_window/tool_base.h"
#include "mkvtoolnix-gui/merge/attachment_model.h"
#include "mkvtoolnix-gui/merge/mux_config.h"
#include "mkvtoolnix-gui/merge/source_file_model.h"
#include "mkvtoolnix-gui/merge/track_model.h"
#include "mkvtoolnix-gui/util/files_drag_drop_handler.h"

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
  mtx::gui::Util::FilesDragDropHandler m_filesDDHandler;

  // "Input" tab:
  SourceFileModel *m_filesModel;
  TrackModel *m_tracksModel;

  QList<QWidget *> m_audioControls, m_videoControls, m_subtitleControls, m_chapterControls, m_typeIndependantControls, m_allInputControls, m_splitControls;
  QList<QComboBox *> m_comboBoxControls;
  bool m_currentlySettingInputControlValues;

  QAction *m_addFilesAction, *m_appendFilesAction, *m_addAdditionalPartsAction, *m_removeFilesAction, *m_removeAllFilesAction;

  // "Attachments" tab:
  AttachmentModel *m_attachmentsModel;
  QAction *m_addAttachmentsAction, *m_removeAttachmentsAction;

  debugging_option_c m_debugTrackModel;

public:
  explicit Tab(QWidget *parent);
  ~Tab();

  virtual QString title() const;
  virtual void load(QString const &fileName);

  virtual void dragEnterEvent(QDragEnterEvent *event) override;
  virtual void dropEvent(QDropEvent *event) override;

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

  virtual void onFileSelectionChanged();
  virtual void onTrackSelectionChanged();

  virtual void onTrackNameEdited(QString newValue);
  virtual void onMuxThisChanged(int newValue);
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

  // Output tab:
  virtual void setDestination(QString const &newValue);

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

protected:
  virtual void setupAttachmentsControls();
  virtual void setupControlLists();
  virtual void setupInputControls();
  virtual void setupOutputControls();

  virtual void setupInputToolTips();
  virtual void setupOutputToolTips();

  virtual void retranslateInputUI();
  virtual void retranslateAttachmentsUI();

  virtual QStringList selectFilesToAdd(QString const &title);
  virtual QStringList selectAttachmentsToAdd();
  virtual void addOrAppendFiles(bool append);
  virtual void addOrAppendFiles(bool append, QStringList const &fileNames, QModelIndex const &sourceFileIdx);
  virtual void enableFilesActions();
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

  virtual QModelIndex selectedSourceFile() const;
  virtual QList<SourceFile *> selectedSourceFiles() const;
  virtual QList<Track *> selectedTracks() const;

  virtual void addToJobQueue(bool startNow);

  virtual bool isReadyForMerging();

  virtual void setTitleMaybe(QList<SourceFilePtr> const &files);
  virtual void setTitleMaybe(QString const &title);

  virtual void setOutputFileNameMaybe(QString const &fileName);
  virtual QString suggestOutputFileNameExtension() const;
};

}}}

#endif // MTX_MKVTOOLNIX_GUI_MERGE_TAB_H
