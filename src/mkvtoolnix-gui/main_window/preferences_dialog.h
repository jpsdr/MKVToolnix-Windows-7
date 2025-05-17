#pragma once

#include "common/common_pch.h"

#include <QDialog>

#include "mkvtoolnix-gui/util/settings.h"

class QItemSelection;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QModelIndex;

namespace mtx::gui {

namespace Ui {
class PreferencesDialog;
}

class PreferencesDialog : public QDialog {
  Q_OBJECT

public:
  enum class Page {
    Gui,
    OftenUsedSelections,
    Languages,
    Merge,
    MergeLayout,
    PredefinedValues,
    DefaultValues,
    DeriveTrackLanguage,
    Output,
    EnablingTracks,
    Playlists,
    Info,
    HeaderEditor,
    ChapterEditor,
    Jobs,
    RunPrograms,

    PreviouslySelected,
  };

protected:
  static Page ms_previouslySelectedPage;

  // UI stuff:
  std::unique_ptr<Ui::PreferencesDialog> ui;
  Util::Settings &m_cfg;
  QString const m_previousUiLocale, m_previousUiFontFamily;
  int const m_previousUiFontPointSize;
  bool const m_previousDisableToolTips;
  double m_previousProbeRangePercentage;
  QStringList const m_previousOftenUsedLanguages, m_previousOftenUsedRegions, m_previousOftenUsedCharacterSets;
  QMap<Page, int> m_pageIndexes;
  bool m_ignoreNextCurrentChange;

public:
  explicit PreferencesDialog(QWidget *parent, Page pageToShow);
  ~PreferencesDialog();

  void save();
  bool uiLocaleChanged() const;
  bool uiFontChanged() const;
  bool disableToolTipsChanged() const;
  bool probeRangePercentageChanged() const;
  bool languageRegionCharacterSetSettingsChanged() const;

public Q_SLOTS:
  void editDefaultAdditionalCommandLineOptions();
  void enableOutputFileNameControls();
  void browseMediaInfoExe();
  void browseFixedOutputDirectory();
  void pageSelectionChanged(QItemSelection const &selection);
  void addProgramToExecute();
  void removeProgramToExecute(int index);
  void setSendersTabTitleForRunProgramWidget();
  void adjustPlaylistControls();
  void adjustRemoveOldJobsControls();
  void enableDeriveCommentaryFlagControls();
  void enableDeriveHearingImpairedFlagControls();
  void enableDeriveForcedDisplayFlagSubtitlesControls();
  void enableSetOriginalLanguageFlagControls();
  void setupCommonLanguages(bool withISO639_3);

  void setupLanguageShortcuts();
  void enableLanguageShortcutControls();
  void addLanguageShortcut();
  void editLanguageShortcut();
  void removeLanguageShortcuts();

  void addFileColor();
  void removeFileColors();
  void editSelectedFileColor();
  void editFileColor(QListWidgetItem *item);
  void revertFileColorsToDefault();
  void enableFileColorsControls();

  void enableOftendUsedLanguagesOnly();
  void enableOftendUsedRegionsOnly();
  void enableOftendUsedCharacterSetsOnly();

  virtual void accept() override;
  virtual void reject() override;

protected:
  void setupPageSelector(Page pageToShow);
  void setupToolTips();
  void setupConnections();

  void setupBCP47LanguageEditMode();
  void setupBCP47NormalizationMode();
  void setupInterfaceLanguage();
  void setupTabPositions();
  void setupDefaultCommandLineEscapeMode();
  void setupDerivingTrackLanguagesFromFileName();
  void setupDeriveForcedDisplayFlagSubtitles();
  void setupDeriveCommentaryFlag();
  void setupDeriveHearingImpairedFlag();
  void setupSetOriginalLanguageFlag();
  void setupWhenToSetDefaultLanguage();
  void setupJobRemovalPolicy();
  void setupCommonRegions();
  void setupCommonCharacterSets();
  void setupProcessPriority();
  void setupPlaylistScanningPolicy();
  void setupOutputFileNamePolicy();
  void setupRecentDestinationDirectoryList();
  void setupTrackPropertiesLayout();
  void setupEnableMuxingTracksByType();
  void setupEnableMuxingTracksByLanguage();
  void setupMergeAddingAppendingFilesPolicy();
  void setupMergeAddingAppendingDirectoriesPolicy();
  void setupMergeWarnMissingAudioTrack();
  void setupMergePredefinedItems();
  void setupHeaderEditorDroppedFilesPolicy();
  void setupJobsRunPrograms();
  void setupFontAndScaling();
  void setupPalette();
  void setupFileColorsControls();
  void setupFileColors(QVector<QColor> const &colors);
  void setupLineEditWithDefaultText(QPushButton *revertButton, QLineEdit *edit, QString const &defaultText);


  QListWidgetItem &setupFileColorItem(QListWidgetItem &item, QColor const &color);

  void showPage(Page page);

  void setTabTitleForRunProgramWidget(int tabIdx, QString const &title);

  QModelIndex modelIndexForPage(int pageIndex);

  bool verifyDeriveTrackLanguageSettings();
  bool verifyDeriveCommentaryFlagSettings();
  bool verifyDeriveHearingImpairedFlagSettings();
  bool verifyDeriveForcedDisplayFlagSettings();
  bool verifyRunProgramConfigurations();

  void rememberCurrentlySelectedPage();

  void saveLanguageShortcuts();
  void saveFileColors();
};

}
