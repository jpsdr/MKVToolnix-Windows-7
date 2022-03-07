#pragma once

#include "common/common_pch.h"

#include "mkvtoolnix-gui/main_window/tool_base.h"
#include "mkvtoolnix-gui/merge/file_identification_pack.h"
#include "mkvtoolnix-gui/util/settings.h"

#include <QList>

class QLineEdit;
class QStandardItem;

namespace mtx::bcp47 {
class language_c;
}

namespace mtx::bluray::disc_library {
struct info_t;
}

namespace mtx::gui::Merge {

enum class InitialDirMode {
    ContentLastOpenDir
  , ContentFirstInputFileLastOpenDir
};

class MuxConfig;

class TabPrivate;
class Tab : public QWidget {
  Q_OBJECT

protected:
  MTX_DECLARE_PRIVATE(TabPrivate)

  std::unique_ptr<TabPrivate> const p_ptr;

public:
  explicit Tab(QWidget *parent);
  ~Tab();

  virtual bool hasBeenModified();
  virtual bool isEmpty();
  virtual bool hasSourceFiles() const;
  virtual bool hasDestinationFileName() const;
  virtual bool hasTitle() const;
  virtual bool hasSelectedNotAppendedRegularTracks() const;

  virtual QString const &fileName() const;
  virtual QString title() const;
  virtual void load(QString const &fileName);
  virtual void cloneConfig(MuxConfig const &config);

  virtual void setChaptersFileName(QString const &fileName);
  virtual void setTagsFileName(QString const &fileName);
  virtual void setSegmentInfoFileName(QString const &fileName);

  virtual QList<SourceFilePtr> const &sourceFiles();
  virtual QModelIndex fileModelIndexForFileNum(unsigned int num);

  virtual void addOrAppendIdentifiedFiles(QVector<SourceFilePtr> const &identifiedFiles, QModelIndex const &fileModelIdx, IdentificationPack::AddMode addMode);
  virtual void addIdentifiedFilesAsAdditionalParts(QVector<SourceFilePtr> const &identifiedFiles, QModelIndex const &fileModelIdx);

  virtual void toggleSpecificTrackFlag(unsigned int wantedId);
  virtual void changeTrackLanguage(QString const &formattedLanguage, QString const &trackName);

Q_SIGNALS:
  void removeThisTab();
  void titleChanged();

public Q_SLOTS:
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

  virtual void onTrackNameChanged(QString newValue);
  virtual void onTrackItemChanged(QStandardItem *item);
  virtual void onMuxThisChanged(int selected);
  virtual void onTrackLanguageChanged(mtx::bcp47::language_c const &newLanguage);
  virtual void onDefaultTrackFlagChanged(int newValue);
  virtual void onForcedTrackFlagChanged(int newValue);
  virtual void onTrackEnabledFlagChanged(int newValue);
  virtual void onHearingImpairedFlagChanged(int newValue);
  virtual void onVisualImpairedFlagChanged(int newValue);
  virtual void onTextDescriptionsFlagChanged(int newValue);
  virtual void onOriginalFlagChanged(int newValue);
  virtual void onCommentaryFlagChanged(int newValue);
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
  virtual void onCroppingChanged(QString newValue);
  virtual void onAacIsSBRChanged(int newValue);
  virtual void onReduceAudioToCoreChanged(bool newValue);
  virtual void onRemoveDialogNormalizationGainChanged(bool newValue);
  virtual void onSubtitleCharacterSetChanged(int newValue);
  virtual void onCuesChanged(int newValue);
  virtual void onAdditionalTrackOptionsChanged(QString newValue);
  virtual void onPreviewChapterCharacterSet();
  virtual void setChapterCharacterSet(QString const &characterSet);
  virtual void onCopyFirstFileNameToTitle();
  virtual void onCopyOutputFileNameToTitle();
  virtual void onCopyTitleToOutputFileName();
  virtual void setDestinationFileNameFromSelectedFile();

  virtual void addToJobQueue(bool startNow, std::optional<Util::Settings::ClearMergeSettingsAction> clearSettings = std::nullopt);

  virtual void reinitFilesTracksControls();

  virtual void onFileRowsInserted(QModelIndex const &parentIdx, int first, int last);
  virtual void onTrackRowsInserted(QModelIndex const &parentIdx, int first, int last);

  virtual void showFilesContextMenu(QPoint const &pos);
  virtual void showTracksContextMenu(QPoint const &pos);
  virtual void showAttachedFilesContextMenu(QPoint const &pos);
  virtual void showAttachmentsContextMenu(QPoint const &pos);

  // Output tab:
  virtual void setupOutputFileControls();
  virtual void setDestination(QString const &newValue);
  virtual void clearDestination();
  virtual void clearDestinationMaybe();
  virtual void clearTitle();
  virtual void clearTitleMaybe();

  virtual void onTitleChanged(QString newValue);
  virtual void onBrowseOutput();
  virtual void showRecentlyUsedOutputDirs();
  virtual void changeOutputDirectoryTo(QString const &directory);
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
  virtual void onChapterTitleNumberChanged(int newValue);
  virtual void onChapterLanguageChanged(mtx::bcp47::language_c const &newLanguage);
  virtual void onChapterCharacterSetChanged(QString newValue);
  virtual void onChapterDelayChanged(QString newValue);
  virtual void onChapterStretchByChanged(QString newValue);
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

  virtual void retranslateUi();
  virtual void handleClearingMergeSettings(Util::Settings::ClearMergeSettingsAction action);

  virtual void signalRemovalOfThisTab();

  virtual void setupHorizontalScrollAreaInputLayout();
  virtual void setupHorizontalTwoColumnsInputLayout();
  virtual void setupVerticalTabWidgetInputLayout();

protected:
  virtual void setupAttachmentsControls();
  virtual void setupControlLists();
  virtual void setupInputControls();
  virtual void setupOutputControls();
  virtual void setupPredefinedTrackNames();

  virtual void setupInputToolTips();
  virtual void setupOutputToolTips();
  virtual void setupSplitModeLabelAndToolTips();
  virtual void setupAttachmentsToolTips();

  virtual void retranslateInputUI();
  virtual void retranslateOutputUI();
  virtual void retranslateAttachmentsUI();

  virtual QStringList selectFilesToAdd(QString const &title);
  virtual QStringList selectAttachmentsToAdd();
  virtual void selectFilesAndIdentifyForAddingOrAppending(IdentificationPack::AddMode addMode);
  virtual void setDefaultsFromSettingsForAddedFiles(QVector<SourceFilePtr> const &files);

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
  virtual QString getSaveFileName(QString const &title, QString const &defaultFileName, QString const &filter, QLineEdit *lineEdit, QString const &defaultSuffix);
  virtual QString determineInitialDirForOpening(QLineEdit *lineEdit, InitialDirMode mode) const;
  virtual QString determineInitialDirForSaving(QLineEdit *lineEdit) const;
  virtual QString defaultFileNameForSaving(QString const &ext);

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
  virtual void selectAllTracksOfType(std::optional<TrackType> type);

  virtual QList<Track *> selectedAttachedFiles() const;
  virtual QList<Attachment *> selectedAttachments() const;
  virtual void selectAttachments(QList<Attachment *> const &attachments);
  virtual std::optional<QString> findExistingAttachmentFileName(QString const &fileName);
  virtual AttachmentPtr prepareFileForAttaching(QString const &fileName, bool alwaysAdd);
  virtual void addAttachmentsFromIdentifiedBluray(mtx::bluray::disc_library::info_t const &info);

  virtual bool isReadyForMerging();
  virtual bool checkIfOverwritingIsOK();
  virtual bool checkIfMissingAudioTrackIsOK();
  virtual QString findExistingDestination() const;

  virtual void setTitleMaybe(QVector<SourceFilePtr> const &files);
  virtual void setTitleMaybe(QString const &title);

  virtual void setOutputFileNameMaybe(bool force = false);
  virtual QString suggestOutputFileNameExtension() const;
  virtual QString generateUniqueOutputFileName(QString const &baseName, QDir const &outputDir, bool removeUniquenessSuffix = false);

  virtual void enableDisableAllTracks(bool enable);

  virtual QString currentState();

  virtual QString mediaInfoLocation();
  virtual void openFilesInMediaInfo(QStringList const &fileNames);

  virtual void addSegmentUIDFromFile(QLineEdit &lineEdit, bool append);
  virtual void addDataFromIdentifiedBlurayFiles(QVector<SourceFilePtr> const &files);
};

}
