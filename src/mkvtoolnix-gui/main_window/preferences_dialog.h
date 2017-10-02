#pragma once

#include "common/common_pch.h"

#include <QDialog>

#include "mkvtoolnix-gui/util/settings.h"

class QListWidget;
class QModelIndex;

namespace mtx { namespace gui {

namespace Ui {
class PreferencesDialog;
}

class PreferencesDialog : public QDialog {
  Q_OBJECT;

public:
  enum class Page {
    Gui,
    OftenUsedSelections,
    Merge,
    DefaultValues,
    Output,
    EnablingTracks,
    Playlists,
    HeaderEditor,
    ChapterEditor,
    Jobs,
    RunPrograms,

    Default = Gui,
  };

protected:
  // UI stuff:
  std::unique_ptr<Ui::PreferencesDialog> ui;
  Util::Settings &m_cfg;
  QString const m_previousUiLocale;
  double m_previousProbeRangePercentage;
  QMap<Page, int> m_pageIndexes;
  bool m_ignoreNextCurrentChange;

public:
  explicit PreferencesDialog(QWidget *parent, Page pageToShow);
  ~PreferencesDialog();

  void save();
  bool uiLocaleChanged() const;
  bool probeRangePercentageChanged() const;

public slots:
  void editDefaultAdditionalCommandLineOptions();
  void enableOutputFileNameControls();
  void browseFixedOutputDirectory();
  void pageSelectionChanged(QModelIndex const &current);
  void addProgramToExecute();
  void removeProgramToExecute(int index);
  void setSendersTabTitleForRunProgramWidget();
  void adjustPlaylistControls();
  void adjustRemoveOldJobsControls();

  void enableOftendUsedLanguagesOnly();
  void enableOftendUsedCountriesOnly();
  void enableOftendUsedCharacterSetsOnly();

  virtual void accept() override;

protected:
  void setupPageSelector(Page pageToShow);
  void setupToolTips();
  void setupConnections();

  void setupInterfaceLanguage();
  void setupTabPositions();
  void setupWhenToSetDefaultLanguage();
  void setupJobRemovalPolicy();
  void setupCommonLanguages();
  void setupCommonCountries();
  void setupCommonCharacterSets();
  void setupProcessPriority();
  void setupPlaylistScanningPolicy();
  void setupOutputFileNamePolicy();
  void setupTrackPropertiesLayout();
  void setupEnableMuxingTracksByLanguage();
  void setupMergeAddingAppendingFilesPolicy();
  void setupHeaderEditorDroppedFilesPolicy();
  void setupJobsRunPrograms();
  void setupFont();

  void showPage(Page page);

  void setTabTitleForRunProgramWidget(int tabIdx, QString const &title);

  QModelIndex modelIndexForPage(int pageIndex);
};

}}
