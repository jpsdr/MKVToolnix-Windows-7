#include "common/common_pch.h"

#include <QColorDialog>
#include <QFileDialog>
#include <QIcon>
#include <QInputDialog>
#include <QItemSelectionModel>
#include <QModelIndex>
#include <QStandardItem>
#include <QTabWidget>
#include <QVector>

#include "common/bcp47.h"
#include "common/chapters/chapters.h"
#include "common/qt.h"
#include "common/translation.h"
#include "mkvtoolnix-gui/app.h"
#include "mkvtoolnix-gui/chapter_editor/tool.h"
#include "mkvtoolnix-gui/forms/main_window/preferences_dialog.h"
#include "mkvtoolnix-gui/main_window/preferences_dialog.h"
#include "mkvtoolnix-gui/main_window/prefs_language_shortcut_dialog.h"
#include "mkvtoolnix-gui/main_window/prefs_run_program_widget.h"
#include "mkvtoolnix-gui/merge/adding_directories_dialog.h"
#include "mkvtoolnix-gui/merge/additional_command_line_options_dialog.h"
#include "mkvtoolnix-gui/merge/command_line_dialog.h"
#include "mkvtoolnix-gui/merge/source_file.h"
#include "mkvtoolnix-gui/util/container.h"
#include "mkvtoolnix-gui/util/file_dialog.h"
#include "mkvtoolnix-gui/util/message_box.h"
#include "mkvtoolnix-gui/util/model.h"
#include "mkvtoolnix-gui/util/side_by_side_multi_select.h"
#include "mkvtoolnix-gui/util/string_list_configuration_widget.h"
#include "mkvtoolnix-gui/util/widget.h"
#include "qregularexpression.h"

namespace mtx::gui {

PreferencesDialog::Page PreferencesDialog::ms_previouslySelectedPage{PreferencesDialog::Page::Gui};

PreferencesDialog::PreferencesDialog(QWidget *parent,
                                     Page pageToShow)
  : QDialog{parent, Qt::Dialog | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint}
  , ui{new Ui::PreferencesDialog}
  , m_cfg(Util::Settings::get())
  , m_previousUiLocale{m_cfg.m_uiLocale}
  , m_previousUiFontFamily{m_cfg.m_uiFontFamily}
  , m_previousUiFontPointSize{m_cfg.m_uiFontPointSize}
  , m_previousDisableToolTips{m_cfg.m_uiDisableToolTips}
  , m_previousProbeRangePercentage{m_cfg.m_probeRangePercentage}
  , m_previousOftenUsedLanguages{m_cfg.m_oftenUsedLanguages}
  , m_previousOftenUsedRegions{m_cfg.m_oftenUsedRegions}
  , m_previousOftenUsedCharacterSets{m_cfg.m_oftenUsedCharacterSets}
  , m_ignoreNextCurrentChange{}
{
  ui->setupUi(this);

  setupPageSelector(pageToShow == Page::PreviouslySelected ? ms_previouslySelectedPage : pageToShow);

  ui->tbOftenUsedXYZ->setTabPosition(m_cfg.m_tabPosition);

  Util::restoreWidgetGeometry(this);

  // GUI page
  ui->cbGuiDisableToolTips->setChecked(m_cfg.m_uiDisableToolTips);
  ui->cbGuiCheckForUpdates->setChecked(m_cfg.m_checkForUpdates);
  ui->cbGuiShowDebuggingMenu->setChecked(m_cfg.m_showDebuggingMenu);
  ui->cbGuiShowToolSelector->setChecked(m_cfg.m_showToolSelector);
  ui->cbGuiShowMoveUpDownButtons->setChecked(m_cfg.m_showMoveUpDownButtons);
  ui->cbGuiElideTabHeaderLabels->setChecked(m_cfg.m_elideTabHeaderLabels);
  ui->cbGuiUseLegacyFontMIMETypes->setChecked(m_cfg.m_useLegacyFontMIMETypes);
  ui->cbGuiWarnBeforeClosingModifiedTabs->setChecked(m_cfg.m_warnBeforeClosingModifiedTabs);
  ui->cbGuiWarnBeforeAbortingJobs->setChecked(m_cfg.m_warnBeforeAbortingJobs);
  ui->cbGuiWarnBeforeOverwriting->setChecked(m_cfg.m_warnBeforeOverwriting);
  ui->sbGuiNumRecentlyUsedStrings->setValue(m_cfg.m_numRecentlyUsedStringsToRemember);
  setupFontAndScaling();
  setupPalette();
  setupInterfaceLanguage();
  setupTabPositions();
  setupDefaultCommandLineEscapeMode();
  setupBCP47LanguageEditMode();
  setupBCP47NormalizationMode();
  setupDerivingTrackLanguagesFromFileName();
  setupWhenToSetDefaultLanguage();
  setupLanguageShortcuts();

  ui->cbGuiUseDefaultJobDescription->setChecked(m_cfg.m_useDefaultJobDescription);
  ui->cbGuiShowOutputOfAllJobs->setChecked(m_cfg.m_showOutputOfAllJobs);
  ui->cbGuiSwitchToJobOutputAfterStarting->setChecked(m_cfg.m_switchToJobOutputAfterStarting);
  ui->cbGuiResetJobWarningErrorCountersOnExit->setChecked(m_cfg.m_resetJobWarningErrorCountersOnExit);
  ui->cbGuiRemoveOutputFileOnJobFailure->setChecked(m_cfg.m_removeOutputFileOnJobFailure);
  ui->cbGuiRemoveOldJobs->setChecked(m_cfg.m_removeOldJobs);
  ui->sbGuiRemoveOldJobsDays->setValue(m_cfg.m_removeOldJobsDays);
  ui->sbGuiMaximumConcurrentJobs->setValue(m_cfg.m_maximumConcurrentJobs);
  adjustRemoveOldJobsControls();
  setupJobRemovalPolicy();

  setupCommonLanguages(m_cfg.m_useISO639_3Languages);
  setupCommonRegions();
  setupCommonCharacterSets();

  ui->cbUseISO639_3Languages->setChecked(m_cfg.m_useISO639_3Languages);

  // Merge page
  if (!m_cfg.m_mediaInfoExe.isEmpty())
    ui->leMMediaInfoExe->setText(QDir::toNativeSeparators(m_cfg.m_mediaInfoExe));
  ui->cbMEnsureAtLeastOneTrackEnabled->setChecked(m_cfg.m_mergeEnsureAtLeastOneTrackEnabled);
  ui->cbMShowDNDZones->setChecked(m_cfg.m_mergeShowDNDZones);
  ui->cbMSortFilesTracksByTypeWhenAdding->setChecked(m_cfg.m_mergeSortFilesTracksByTypeWhenAdding);
  ui->cbMReconstructSequencesWhenAdding->setChecked(m_cfg.m_mergeReconstructSequencesWhenAdding);
  ui->cbMAutoSetFileTitle->setChecked(m_cfg.m_autoSetFileTitle);
  ui->cbMAutoClearFileTitle->setChecked(m_cfg.m_autoClearFileTitle);
  ui->cbMSetAudioDelayFromFileName->setChecked(m_cfg.m_setAudioDelayFromFileName);
  ui->cbMDisableCompressionForAllTrackTypes->setChecked(m_cfg.m_disableCompressionForAllTrackTypes);
  ui->cbMDisableDefaultTrackForSubtitles->setChecked(m_cfg.m_disableDefaultTrackForSubtitles);
  ui->cbMEnableDialogNormGainRemoval->setChecked(m_cfg.m_mergeEnableDialogNormGainRemoval);
  ui->cbMAlwaysShowOutputFileControls->setChecked(m_cfg.m_mergeAlwaysShowOutputFileControls);
  ui->cbMClearMergeSettings->setCurrentIndex(static_cast<int>(m_cfg.m_clearMergeSettings));
  ui->ldwMDefaultAudioTrackLanguage->setLanguage(m_cfg.m_defaultAudioTrackLanguage);
  ui->ldwMDefaultAudioTrackLanguage->registerBuddyLabel(*ui->lDefaultAudioTrackLanguage);
  ui->ldwMDefaultVideoTrackLanguage->setLanguage(m_cfg.m_defaultVideoTrackLanguage);
  ui->ldwMDefaultVideoTrackLanguage->registerBuddyLabel(*ui->lDefaultVideoTrackLanguage);
  ui->ldwMDefaultSubtitleTrackLanguage->setLanguage(m_cfg.m_defaultSubtitleTrackLanguage);
  ui->ldwMDefaultSubtitleTrackLanguage->registerBuddyLabel(*ui->lDefaultSubtitleTrackLanguage);
  ui->cbMDefaultSubtitleCharset->setAdditionalItems(m_cfg.m_defaultSubtitleCharset).setup(true, QY("– No selection by default –")).setCurrentByData(m_cfg.m_defaultSubtitleCharset);
  ui->leMDefaultAdditionalCommandLineOptions->setText(m_cfg.m_defaultAdditionalMergeOptions);
  ui->cbMProbeRangePercentage->setValue(m_cfg.m_probeRangePercentage);
  ui->cbMAddBlurayCovers->setChecked(m_cfg.m_mergeAddBlurayCovers);
  ui->cbMAttachmentAlwaysSkipForExistingName->setChecked(m_cfg.m_mergeAttachmentsAlwaysSkipForExistingName);
  ui->cbMDeriveFlagsFromTrackNames->setChecked(m_cfg.m_deriveFlagsFromTrackNames);

  setupDeriveForcedDisplayFlagSubtitles();
  setupDeriveHearingImpairedFlag();
  setupDeriveCommentaryFlag();
  setupSetOriginalLanguageFlag();
  setupFileColorsControls();
  setupProcessPriority();
  setupPlaylistScanningPolicy();
  setupOutputFileNamePolicy();
  setupRecentDestinationDirectoryList();
  setupEnableMuxingTracksByType();
  setupEnableMuxingTracksByLanguage();
  ui->cbMEnableMuxingForcedSubtitleTracks->setChecked(m_cfg.m_enableMuxingForcedSubtitleTracks);
  setupMergeAddingAppendingFilesPolicy();
  setupMergeAddingAppendingDirectoriesPolicy();
  setupMergeWarnMissingAudioTrack();
  setupMergePredefinedItems();
  setupTrackPropertiesLayout();

  // Info tool page
  ui->wIDefaultJobSettings->setFileNameVisible(false);
  ui->wIDefaultJobSettings->setSettings(m_cfg.m_defaultInfoJobSettings);

  // Chapter editor page
  ui->cbCEDropLastFromBlurayPlaylist->setChecked(m_cfg.m_dropLastChapterFromBlurayPlaylist);
  ui->cbCETextFileCharacterSet->setAdditionalItems(m_cfg.m_ceTextFileCharacterSet).setup(true, QY("Always ask the user")).setCurrentByData(m_cfg.m_ceTextFileCharacterSet);
  ui->leCENameTemplate->setText(m_cfg.m_chapterNameTemplate);
  ui->ldwCEDefaultLanguage->setLanguage(m_cfg.m_defaultChapterLanguage);

  // Header editor page
  setupHeaderEditorDroppedFilesPolicy();
  ui->cbHEDateTimeInUTC->setChecked(m_cfg.m_headerEditorDateTimeInUTC);

  setupJobsRunPrograms();

  setupToolTips();
  setupConnections();

  Util::preventScrollingWithoutFocus(this);
}

PreferencesDialog::~PreferencesDialog() {
  Util::saveWidgetGeometry(this);
}

bool
PreferencesDialog::uiLocaleChanged()
  const {
  return m_previousUiLocale != m_cfg.m_uiLocale;
}

bool
PreferencesDialog::uiFontChanged()
  const {
  return (m_previousUiFontFamily    != m_cfg.m_uiFontFamily)
      || (m_previousUiFontPointSize != m_cfg.m_uiFontPointSize);
}

bool
PreferencesDialog::disableToolTipsChanged()
  const {
  return m_previousDisableToolTips != m_cfg.m_uiDisableToolTips;
}

bool
PreferencesDialog::probeRangePercentageChanged()
  const {
  return m_previousProbeRangePercentage != m_cfg.m_probeRangePercentage;
}

bool
PreferencesDialog::languageRegionCharacterSetSettingsChanged()
  const {
  return (m_previousOftenUsedLanguages     != m_cfg.m_oftenUsedLanguages)
      || (m_previousOftenUsedRegions       != m_cfg.m_oftenUsedRegions)
      || (m_previousOftenUsedCharacterSets != m_cfg.m_oftenUsedCharacterSets);
}

void
PreferencesDialog::pageSelectionChanged(QItemSelection const &selection) {
  if (selection.indexes().isEmpty())
    return;

  auto current = selection.indexes().first();
  if (!current.isValid())
    return;

  if (m_ignoreNextCurrentChange) {
    m_ignoreNextCurrentChange = false;
    return;
  }

  auto page = qobject_cast<QStandardItemModel *>(ui->pageSelector->model())
    ->itemFromIndex(current.sibling(current.row(), 0))
    ->data()
    .value<int>();
  ui->pages->setCurrentIndex(page);
}

QModelIndex
PreferencesDialog::modelIndexForPage(int page) {
  auto &model  = *qobject_cast<QStandardItemModel *>(ui->pageSelector->model());
  auto pageIdx = Util::findIndex(model, [page, &model](QModelIndex const &idxToTest) {
    return model.itemFromIndex(idxToTest)->data().value<int>() == page;
  });

  return pageIdx;
}

void
PreferencesDialog::setupPageSelector(Page pageToShow) {
  m_cfg.handleSplitterSizes(ui->pagesSplitter);

  auto pageIndex = 0;
  auto model     = new QStandardItemModel{this};

  ui->pageSelector->setModel(model);
  ui->pageSelector->setIconSize({ 16, 16 });

  auto addItem = [this, model, &pageIndex](Page pageType, QStandardItem *parent, QString const &text, QString const &icon = QString{}) -> QStandardItem * {
    auto item = new QStandardItem{text};

    item->setData(pageIndex);
    m_pageIndexes[pageType] = pageIndex++;

    if (!icon.isEmpty())
      item->setIcon(Util::fixStandardItemIcon(QIcon::fromTheme(icon)));

    if (parent)
      parent->appendRow(item);
    else
      model->appendRow(item);

    return item;
  };

  auto pGui      = addItem(Page::Gui,                 nullptr, QY("GUI"),              "mkvtoolnix-gui");
                   addItem(Page::OftenUsedSelections, pGui,    QY("Often used selections"));
                   addItem(Page::Languages,           pGui,    QY("Languages"));
  auto pMerge    = addItem(Page::Merge,               nullptr, QY("Multiplexer"),      "merge");
                   addItem(Page::MergeLayout,         pMerge,  QY("Layout"));
                   addItem(Page::PredefinedValues,    pMerge,  QY("Predefined values"));
                   addItem(Page::DefaultValues,       pMerge,  QY("Default values"));
                   addItem(Page::DeriveTrackLanguage, pMerge,  QY("Deriving track languages"));
                   addItem(Page::Output,              pMerge,  QY("Destination file name"));
                   addItem(Page::EnablingTracks,      pMerge,  QY("Enabling items"));
                   addItem(Page::Playlists,           pMerge,  QY("Playlists & Blu-rays"));
                   addItem(Page::Info,                nullptr, QY("Info tool"),        "document-preview-archive");
                   addItem(Page::HeaderEditor,        nullptr, QY("Header editor"),    "document-edit");
                   addItem(Page::ChapterEditor,       nullptr, QY("Chapter editor"),   "story-editor");
  auto pJobs     = addItem(Page::Jobs,                nullptr, QY("Jobs & job queue"), "view-task");
                   addItem(Page::RunPrograms,         pJobs,   QY("Executing actions"));

  for (auto row = 0, numRows = model->rowCount(); row < numRows; ++row)
    ui->pageSelector->setExpanded(model->index(row, 0), true);

  ui->pageSelector->setMinimumSize(ui->pageSelector->minimumSizeHint());

  showPage(pageToShow);

  m_ignoreNextCurrentChange = false;

  connect(ui->pageSelector->selectionModel(), &QItemSelectionModel::selectionChanged, this, &PreferencesDialog::pageSelectionChanged);
}

void
PreferencesDialog::setupToolTips() {
  if (m_cfg.m_uiDisableToolTips)
    return;

  // GUI page
  Util::setToolTip(ui->cbGuiDisableHighDPIScaling,
                   Q("%1 %2")
                   .arg(QY("If enabled, automatic scaling for high DPI displays will be disabled."))
                   .arg(QY("Changes to this option will only take effect the next time the application is started.")));

  Util::setToolTip(ui->cbGuiDisableToolTips, QY("If checked, the GUI will not show helpful usage tips in popup windows while hovering over a control — such as this one."));

  Util::setToolTip(ui->sbGuiNumRecentlyUsedStrings, QY("This affects functions such as the selector of recently used destination directories in the multiplexer."));

  Util::setToolTip(ui->cbGuiCheckForUpdates,
                   Q("%1 %2 %3")
                   .arg(QY("If enabled, the program will check online whether or not a new release of MKVToolNix is available on the home page."))
                   .arg(QY("This is done at startup and at most once within 24 hours."))
                   .arg(QY("No information is transmitted to the server.")));

  Util::setToolTip(ui->cbGuiWarnBeforeClosingModifiedTabs,
                   Q("%1 %2")
                   .arg(QY("If checked, the program will ask for confirmation before closing or reloading tabs that have been modified."))
                   .arg(QY("This is also done when quitting the application.")));
  Util::setToolTip(ui->cbGuiWarnBeforeAbortingJobs,
                   Q("%1 %2")
                   .arg(QY("If checked, the program will ask for confirmation before aborting a running job."))
                   .arg(QY("This happens when clicking the \"abort job\" button in a \"job output\" tab and when quitting the application.")));

  Util::setToolTip(ui->cbGuiShowMoveUpDownButtons,
                   Q("%1 %2")
                   .arg(QY("Normally selected entries in list view can be moved around via drag & drop and with keyboard shortcuts (Ctrl+Up, Ctrl+Down)."))
                   .arg(QY("If checked, additional buttons for moving selected entries up and down will be shown next to several list views.")));

  Util::setToolTip(ui->cbGuiElideTabHeaderLabels, QY("If enabled, the names of tab headers will be shortened so that all tab headers fit into the window's width."));

  Util::setToolTip(ui->cbGuiUseLegacyFontMIMETypes,
                   Q("%1 %2")
                   .arg(QY("If enabled, the GUI will use legacy MIME types when detecting the MIME type of font attachments instead of the current standard MIME types."))
                   .arg(QY("This mostly affects TrueType fonts for which the legacy MIME type ('application/x-truetype-font') might be more widely supported than the standard MIME types ('font/sfnt' and 'font/ttf').")));

  Util::setToolTip(ui->cbMEnsureAtLeastOneTrackEnabled,
                   Q("%1 %2 %3")
                   .arg(QY("If enabled, the GUI checks the state of the 'track enabled' flag of all video, audio & subtitle tracks when starting to multiplex or adding a job to the job queue."))
                   .arg(QY("For each track type the GUI determines if at least one track has the flag turned on."))
                   .arg(QY("If not, it will turn on the flag for the first track of the current type.")));

  Util::setToolTip(ui->cbGuiPalette, QY("Changes to this option will only take effect the next time the application is started."));

  Util::setToolTip(ui->cbGuiWarnBeforeOverwriting, QY("If enabled, the program will ask for confirmation before overwriting files and jobs."));

  Util::setToolTip(ui->cbGuiUseDefaultJobDescription, QY("If disabled, the GUI will let you enter a description for a job when adding it to the queue."));
  Util::setToolTip(ui->cbGuiShowOutputOfAllJobs,      QY("If enabled, the first tab in the \"job output\" tool will not be cleared when a new job starts."));
  Util::setToolTip(ui->cbGuiSwitchToJobOutputAfterStarting,     QY("If enabled, the GUI will automatically switch to the job output tool whenever you start a job (e.g. by pressing \"start multiplexing\")."));
  Util::setToolTip(ui->cbGuiResetJobWarningErrorCountersOnExit, QY("If enabled, the warning and error counters of all jobs and the global counters in the status bar will be reset to 0 when the program exits."));
  Util::setToolTip(ui->cbGuiRemoveOutputFileOnJobFailure,       QY("If enabled, the GUI will remove the output file created by a job if that job ends with an error or if the user aborts the job."));
  Util::setToolTip(ui->cbGuiRemoveOldJobs,                      QY("If enabled, the GUI will remove completed jobs older than the configured number of days no matter their status on exit."));
  Util::setToolTip(ui->sbGuiRemoveOldJobsDays,                  QY("If enabled, the GUI will remove completed jobs older than the configured number of days no matter their status on exit."));

  Util::setToolTip(ui->cbGuiRemoveJobs,
                   Q("%1 %2")
                   .arg(QY("Normally completed jobs stay in the queue even over restarts until the user clears them out manually."))
                   .arg(QY("You can opt for having them removed automatically under certain conditions.")));

  Util::setToolTip(ui->leCENameTemplate, ChapterEditor::Tool::chapterNameTemplateToolTip());
  Util::setToolTip(ui->ldwCEDefaultLanguage, QY("This is the language that newly added chapter names get assigned automatically."));
  Util::setToolTip(ui->cbCEDropLastFromBlurayPlaylist,
                   Q("%1 %2")
                   .arg(QY("Blu-ray discs often contain a chapter entry very close to the end of the movie."))
                   .arg(QY("If enabled, the last entry will be skipped when loading chapters from such playlists in the chapter editor if it is located within five seconds of the end of the movie.")));
  Util::setToolTip(ui->cbCETextFileCharacterSet,
                   Q("%1 %2 %3")
                   .arg(QY("The chapter editor needs to know the character set a text chapter file uses in order to display all characters properly."))
                   .arg(QY("By default it always asks the user which character set to use when opening a file for which it cannot be recognized automatically."))
                   .arg(QY("If a character set is selected here, it will be used instead of asking the user.")));

  // Merge page
  Util::setToolTip(ui->cbMAutoSetFileTitle,
                   Q("<p>%1 %2</p><p>%3</p>")
                   .arg(QYH("Certain file formats have 'title' property."))
                   .arg(QYH("When the user adds a file containing such a title, the program will copy the title into the \"file title\" input box if this option is enabled."))
                   .arg(QYH("Note that even if the option is disabled mkvmerge will copy a source file's title property unless a title is manually set by the user.")));
  Util::setToolTip(ui->cbMAutoClearFileTitle, QY("If this option is enabled, the GUI will always clear the \"file title\" input box whenever the last source file is removed."));

  auto widgets = QList<mtx::gui::Util::StringListConfigurationWidget *>{} << ui->lwMPredefinedVideoTrackNames << ui->lwMPredefinedAudioTrackNames << ui->lwMPredefinedSubtitleTrackNames;
  for (auto const &widget : widgets)
    widget->setToolTips(Q("%1 %2 %3")
                        .arg(QY("If you often use the same names for tracks, you can enter them here."))
                        .arg(QY("The names will be available for easy selection in both the multiplexer and the header editor."))
                        .arg(QY("You can still enter track names not present in this list manually in both tools.")));

  ui->lwMPredefinedSplitSizes->setToolTips(Q("%1 %2 %3")
                                           .arg(QY("If you often use the same values when splitting by size, you can enter them here."))
                                           .arg(QY("The values will be available for easy selection in the multiplexer."))
                                           .arg(QY("You can still enter values not present in this list manually in the multiplexer.")));

  ui->lwMPredefinedSplitDurations->setToolTips(Q("%1 %2 %3")
                                               .arg(QY("If you often use the same values when splitting by duration, you can enter them here."))
                                               .arg(QY("The values will be available for easy selection in the multiplexer."))
                                               .arg(QY("You can still enter values not present in this list manually in the multiplexer.")));

  Util::setToolTip(ui->cbMSetAudioDelayFromFileName,
                   Q("%1 %2")
                   .arg(QY("When a file is added its name is scanned."))
                   .arg(QY("If it contains the word 'DELAY' followed by a number, this number is automatically put into the 'delay' input field for any audio track found in the file.")));

  Util::setToolTip(ui->cbMDisableDefaultTrackForSubtitles, QY("If enabled, all subtitle tracks will have their \"default track\" flag set to \"no\" when they're added."));

  Util::setToolTip(ui->cbMDisableCompressionForAllTrackTypes,
                   Q("%1 %2")
                   .arg(QY("Normally mkvmerge will apply additional lossless compression for subtitle tracks for certain codecs."))
                   .arg(QY("Checking this option causes the GUI to set that compression to \"none\" by default for all track types when adding files.")));

  Util::setToolTip(ui->cbMEnableDialogNormGainRemoval, QY("If enabled, removal of dialog normalization gain will be enabled for all audio tracks for which removal is supported."));

  Util::setToolTip(ui->cbMProbeRangePercentage,
                   Q("%1 %2 %3")
                   .arg(QY("File types such as MPEG program and transport streams (.vob, .m2ts) require parsing a certain amount of data in order to detect all tracks contained in the file."))
                   .arg(QY("This amount is 0.3% of the source file's size or 10 MB, whichever is higher."))
                   .arg(QY("If tracks are known to be present but not found, the percentage to probe can be changed here.")));

  Util::setToolTip(ui->cbMDefaultCommandLineEscapeMode, QY("Sets how to escape arguments by default in the 'Show command line' dialog."));

  Util::setToolTip(ui->cbMSortFilesTracksByTypeWhenAdding,
                   Q("<p>%1 %2</p><p>%3 %4</p><p>%5</p><p>%6</p>")
                   .arg(QY("If enabled, files and tracks will be sorted by track types when they're added to multiplex settings."))
                   .arg(QY("The order is: video first followed by audio, subtitles and other types."))
                   .arg(QY("For example, a file containing audio tracks but no video tracks will be inserted before the first file that contains neither video nor audio tracks."))
                   .arg(QY("Similarly an audio track will be inserted before the first track that's neither a video nor an audio track."))
                   .arg(QY("If disabled, files and tracks will be inserted after all existing files and tracks."))
                   .arg(QY("This only determines the initial order which can still be changed manually later.")));

  Util::setToolTip(ui->cbMReconstructSequencesWhenAdding,
                   Q("<p>%1 %2 %3</p><p>%4</p><p>%5 %6</p><p>%7</p><p>%8</p>")
                   .arg(QY("If enabled, the GUI will analyze the file names when you add multiple files at once."))
                   .arg(QY("It tries to detect sequences of names that likely belong to the same movie by splitting the name into three parts: a prefix, a running number and a suffix that doesn't contain digits."))
                   .arg(QY("Names are considered to be in sequence when the previous file name's prefix & suffix match the current file name's prefix & suffix and the running number is incremented by one."))
                   .arg(QY("An example: 'movie.001.mp4', 'movie.002.mp4' and 'movie.003.mp4'"))
                   .arg(QY("The format used by GoPro cameras for chaptered video files is recognized as well."))
                   .arg(QY("Both of the schemes used by older models (e.g. GOPR1234.mp4, GP011234.mp4, GP021234.mp4…) & by current ones (e.g. GH019876.mp4, GH029876.mp4, GH039876.mp4…) are supported."))
                   .arg(QY("If such a sequence is detected, only the first file is added while the second and following files in the sequence are appended to the first one."))
                   .arg(QY("If disabled, all files selected for adding will always be added regardless of their names.")));

  Util::setToolTip(ui->cbMAlwaysShowOutputFileControls,
                   Q("%1 %2")
                   .arg(QY("If enabled, the destination file name controls will always be visible no matter which tab is currently shown."))
                   .arg(QY("Otherwise they're shown on the 'output' tab.")));

  Util::setToolTip(ui->cbMShowDNDZones,
                   Q("<p>%1 %2</p><p>%3 %4</p>")
                   .arg(QYH("When the user drags & drops files from an external application onto the multiplex tool the GUI can take different actions."))
                   .arg(QYH("Normally the program will chose what to do with the dropped files based on the preferences, e.g. always add them to the current multiplex settings or ask the user interactively what to do."))
                   .arg(QYH("If enabled, zones will be shown that files can be dropped on to circumvent the preferences."))
                   .arg(QYH("Each zone corresponds to and selects one of the possible actions.")));

  auto controls = QWidgetList{} << ui->rbMTrackPropertiesLayoutHorizontalScrollArea << ui->rbMTrackPropertiesLayoutHorizontalTwoColumns << ui->rbMTrackPropertiesLayoutVerticalTabWidget;
  for (auto const &control : controls)
    Util::setToolTip(control,
                     Q("<p>%1 %2</p><p>%3 %4</p>")
                     .arg(QYH("The track properties on the \"input\" tab can be laid out in three different way in order to serve different workflows."))
                     .arg(QYH("In the most compact layout the track properties are located to the right of the files and tracks lists in a scrollable single column."))
                     .arg(QYH("The other two layouts available are: in two fixed columns to the right or in a tab widget below the files and tracks lists."))
                     .arg(QYH("The horizontal layout with two fixed columns results in a wider window while the vertical tab widget layout results in a higher window.")));

  Util::setToolTip(ui->cbMUseFileAndTrackColors,
                   Q("<p>%1 %2</p>")
                   .arg(QYH("If enabled, small colored boxes will be shown in the file and track lists as a visual clue to help associating tracks with the files they come from."))
                   .arg(QYH("If there are more entries than configured colors, random colors will be used temporarily.")));


  Util::setToolTip(ui->cbMClearMergeSettings,
                   Q("<p>%1</p><ol><li>%2 %3</li><li>%4 %5</li><li>%6</li></ol>")
                   .arg(QYH("The GUI can help you start your next multiplex settings after having started a job or having added a one to the job queue."))
                   .arg(QYH("With \"create new settings\" a new set of multiplex settings will be added."))
                   .arg(QYH("The current multiplex settings will be closed."))
                   .arg(QYH("With \"remove source files\" all source files will be removed."))
                   .arg(QYH("Most of the other settings on the output tab will be kept intact, though."))
                   .arg(QYH("With \"close current settings\" the current multiplex settings will be closed without opening new ones.")));

  Util::setToolTip(ui->cbMAddingAppendingFilesPolicy,
                   Q("%1 %2 %3")
                   .arg(QY("When the user drags & drops files from an external application onto the multiplex tool the GUI can take different actions."))
                   .arg(QY("The default is to always add all the files to the current multiplex settings."))
                   .arg(QY("The GUI can also ask the user what to do each time, e.g. appending them instead of adding them, or creating new multiplex settings and adding them to those.")));

  QString fullText;

  for (auto const &text : Merge::AddingDirectoriesDialog::optionDescriptions())
    fullText += Q("<li>%1</li>").arg(text.toHtmlEscaped());

  Util::setToolTip(ui->cbMDragAndDropDirectoriesPolicy,
                   Q("<p>%1</p><ol>%2</ol>")
                   .arg(QY("When the user drags & drops directories from an external application onto the multiplex tool the GUI can take different actions."))
                   .arg(fullText));

  Util::setToolTip(ui->cbMWarnMissingAudioTrack, QY("The GUI can ask for confirmation when you're about to create a file without audio tracks in it."));

  Util::setToolTip(ui->cbMDeriveAudioTrackLanguageFromFileName,
                   Q("<p>%1 %2</p><p>%3 %4</p>")
                   .arg(QYH("Certain file formats have a 'language' property for their tracks."))
                   .arg(QYH("When the user adds such a file the track's language input is set to the language property from the source file."))
                   .arg(QYH("If the source file contains no such property for an audio track, then the language can be derived from the file name if it matches certain patterns (e.g. '…[ger]…' for German)."))
                   .arg(QYH("Depending on this setting the language can also be derived from the file name if the language in the source file is 'undetermined' ('und').")));
  Util::setToolTip(ui->cbMDeriveVideoTrackLanguageFromFileName,
                   Q("<p>%1 %2</p><p>%3 %4</p>")
                   .arg(QYH("Certain file formats have a 'language' property for their tracks."))
                   .arg(QYH("When the user adds such a file the track's language input is set to the language property from the source file."))
                   .arg(QYH("If the source file contains no such property for a video track, then the language can be derived from the file name if it matches certain patterns (e.g. '…[ger]…' for German)."))
                   .arg(QYH("Depending on this setting the language can also be derived from the file name if the language in the source file is 'undetermined' ('und').")));
  Util::setToolTip(ui->cbMDeriveSubtitleTrackLanguageFromFileName,
                   Q("<p>%1 %2</p><p>%3 %4</p>")
                   .arg(QYH("Certain file formats have a 'language' property for their tracks."))
                   .arg(QYH("When the user adds such a file the track's language input is set to the language property from the source file."))
                   .arg(QYH("If the source file contains no such property for a subtitle track, then the language can be derived from the file name if it matches certain patterns (e.g. '…[ger]…' for German)."))
                   .arg(QYH("Depending on this setting the language can also be derived from the file name if the language in the source file is 'undetermined' ('und').")));
  Util::setToolTip(ui->leMDeriveTrackLanguageBoundaryChars,
                   Q("<p>%1 %2</p>")
                   .arg(QYH("When deriving the track language from the file name, the file name is split into parts on the characters in this list."))
                   .arg(QYH("Each part is then matched against the list of languages selected below to determine whether or not to use it as the track language.")));
  Util::setToolTip(ui->pbMDeriveTrackLanguageRevertBoundaryChars, QY("Revert the entry to its default value."));
  ui->tbMDeriveTrackLanguageRecognizedLanguages->setToolTips(QY("Only the languages in the 'selected' list on the right will be recognized as track languages in file names."));

  Util::setToolTip(ui->ldwMDefaultAudioTrackLanguage,
                   Q("<p>%1 %2</p><p>%3 %4</p>")
                   .arg(QYH("Certain file formats have a 'language' property for their tracks."))
                   .arg(QYH("When the user adds such a file the track's language input is set to the language property from the source file."))
                   .arg(QYH("The language selected here is used for audio tracks for which their source file contains no such property and for which the language has not been derived from the file name."))
                   .arg(QYH("Depending on the setting below this language can also be used if the language in the source file is 'undetermined' ('und').")));
  Util::setToolTip(ui->ldwMDefaultVideoTrackLanguage,
                   Q("<p>%1 %2</p><p>%3 %4</p>")
                   .arg(QYH("Certain file formats have a 'language' property for their tracks."))
                   .arg(QYH("When the user adds such a file the track's language input is set to the language property from the source file."))
                   .arg(QYH("The language selected here is used for video tracks for which their source file contains no such property and for which the language has not been derived from the file name."))
                   .arg(QYH("Depending on the setting below this language can also be used if the language in the source file is 'undetermined' ('und').")));
  Util::setToolTip(ui->ldwMDefaultSubtitleTrackLanguage,
                   Q("<p>%1 %2</p><p>%3 %4</p>")
                   .arg(QYH("Certain file formats have a 'language' property for their tracks."))
                   .arg(QYH("When the user adds such a file the track's language input is set to the language property from the source file."))
                   .arg(QYH("The language selected here is used for subtitle tracks for which their source file contains no such property and for which the language has not been derived from the file name."))
                   .arg(QYH("Depending on the setting below this language can also be used if the language in the source file is 'undetermined' ('und').")));

  auto text = Q("<p>%1 %2</p>")
    .arg(QYH("Audio and subtitle files often contain the words 'cc' or 'sdh' in their file name to signal that they're intended for hearing impaired people."))
    .arg(QYH("The GUI can set the 'hearing impaired' flag for such tracks if the file name matches this regular expression."));

  Util::setToolTip(ui->cbMDeriveHearingImpairedFlag,   text);
  Util::setToolTip(ui->leMDeriveHearingImpairedFlagRE, text);
  Util::setToolTip(ui->pbMDeriveHearingImpairedFlagRERevert, QY("Revert the entry to its default value."));

  text = Q("<p>%1 %2</p>")
    .arg(QYH("Audio and subtitle files often contain the words 'comments' or 'commentary' in their file name to signal that they contain content such as a movie director's comments."))
    .arg(QYH("The GUI can set the 'commentary' flag for such tracks if the file name matches this regular expression."));

  Util::setToolTip(ui->cbMDeriveCommentaryFlag,   text);
  Util::setToolTip(ui->leMDeriveCommentaryFlagRE, text);
  Util::setToolTip(ui->pbMDeriveCommentaryFlagRERevert, QY("Revert the entry to its default value."));

  text = QYH("The GUI can set the 'original language' flag for audio and subtitle tracks if the track's language matches this language.");

  Util::setToolTip(ui->cbMSetOriginalLanguageFlagLanguage, text);
  Util::setToolTip(ui->ldwMSetOriginalLanguageFlagLanguage, text);

  text = Q("<p>%1 %2</p>")
    .arg(QYH("Subtitle files often contain the word 'forced' in their file name to signal that they're intended for 'forced display' only (e.g. when they speak Elfish in 'Lord of the Rings')."))
    .arg(QYH("The GUI can set the 'forced display' flag for such tracks if the file name matches this regular expression."));

  Util::setToolTip(ui->cbMDeriveForcedDisplayFlagSubtitles,   text);
  Util::setToolTip(ui->leMDeriveForcedDisplayFlagSubtitlesRE, text);
  Util::setToolTip(ui->pbMDeriveForcedDisplayFlagSubtitlesRERevert, QY("Revert the entry to its default value."));

  Util::setToolTip(ui->cbMDeriveFlagsFromTrackNames, QYH("If enabled, the flags mentioned above will be derived from track names as well, not just from file names."));

  Util::setToolTip(ui->cbMDefaultSubtitleCharset, QY("If a character set is selected here, the program will automatically set the character set input to this value for newly added text subtitle tracks."));

  Util::setToolTip(ui->leMDefaultAdditionalCommandLineOptions, QY("The options entered here are set for all new multiplex jobs by default."));

  Util::setToolTip(ui->cbMScanPlaylistsPolicy,
                   Q("<p>%1 %2</p><p>%3</p>")
                   .arg(QYH("Whenever the user adds a playlist the program can automatically scan the directory for other playlists and present the user with a detailed list of the playlists found."))
                   .arg(QYH("The user can then select which playlist to actually add."))
                   .arg(QYH("This is useful for situations like Blu-ray discs on which a multitude of playlists exists in the same directory and where it is not obvious which feature (e.g. main movie, extras etc.) "
                            "a playlist belongs to.")));

  Util::setToolTip(ui->sbMMinPlaylistDuration, QY("Only playlists whose duration are at least this long are considered and offered to the user for selection."));
  Util::setToolTip(ui->cbMIgnorePlaylistsForMenus,
                   Q("<p>%1 %2</p>")
                   .arg(QY("Ignores playlists which are likely meant for menus."))
                   .arg(QY("This is considered to be the case when a playlist contains the same item at least five times.")));
  Util::setToolTip(ui->cbMAddBlurayCovers, QY("If enabled, the largest cover image of a Blu-ray will be added as an attachment when adding a Blu-ray playlist."));
  Util::setToolTip(ui->cbMAttachmentAlwaysSkipForExistingName,
                   Q("<p>%1 %2 %3</p>")
                   .arg(QY("When adding new files as attachments the GUI will check if there are other attachments with the same name."))
                   .arg(QY("If one is found, the GUI will ask whether to skip the file or to add it anyway."))
                   .arg(QY("If enabled, such files will always be skipped without asking.")));

  Util::setToolTip(ui->cbMAutoSetDestinationFileName,
                   Q("%1 %2")
                   .arg(QY("If this option is enabled and if there is currently no destination file name set, the program will set one for you when you add a source file."))
                   .arg(QY("The generated destination file name has the same base name as the source file name but with an extension based on the track types present in the file.")));

  Util::setToolTip(ui->cbMAutoSetDestinationOnlyForVideoFiles,
                   Q("%1 %2")
                   .arg(QY("If this option is enabled, only source files containing video tracks will be used for setting the destination file name."))
                   .arg(QY("Other files are ignored when they're added.")));

  Util::setToolTip(ui->cbMAutoSetDestinationFromTitle,
                   Q("%1 %2")
                   .arg(QY("If this option is enabled, the file title will be used as the basis for the destination file name if a file title is set."))
                   .arg(QY("Otherwise the destination file name is derived from the source file names.")));

  Util::setToolTip(ui->cbMAutoSetDestinationFromDirectory,
                   Q("%1 %2")
                   .arg(QY("If this option is enabled, the name of the directory the file is located in will be used as the basis for the destination file name."))
                   .arg(QY("Otherwise the destination file name is derived from the source file names.")));

  Util::setToolTip(ui->cbMUniqueOutputFileNames,
                   Q("%1 %2")
                   .arg(QY("If checked, the program makes sure the suggested destination file name is unique by adding a number (e.g. ' (1)') to the end of the file name."))
                   .arg(QY("This is done only if there is already a file whose name matches the unmodified destination file name.")));
  Util::setToolTip(ui->cbMAutoClearOutputFileName, QY("If this option is enabled, the GUI will always clear the \"destination file name\" input box whenever the last source file is removed."));

  ui->tbMEnableMuxingTracksByType->setToolTips(QY("Only items whose type is in the 'selected' list on the right will be set to be copied by default."));
  Util::setToolTip(ui->cbMEnableMuxingTracksByLanguage,
                   Q("<p>%1 %2 %3</p><p>%4</p>")
                   .arg(QYH("When adding source files all tracks are normally set to be copied into the destination file."))
                   .arg(QYH("If this option is enabled, only those tracks will be set to be copied whose language is selected below."))
                   .arg(QYH("You can exempt certain track types from this restriction by checking the corresponding check box below, e.g. for video tracks."))
                   .arg(QYH("Note that the language \"Undetermined (und)\" is assumed for tracks for which no language is known (e.g. those read from SRT subtitle files).")));
  Util::setToolTip(ui->cbMEnableMuxingAllVideoTracks,    QY("If enabled, tracks of this type will always be set to be copied regardless of their language."));
  Util::setToolTip(ui->cbMEnableMuxingAllAudioTracks,    QY("If enabled, tracks of this type will always be set to be copied regardless of their language."));
  Util::setToolTip(ui->cbMEnableMuxingAllSubtitleTracks, QY("If enabled, tracks of this type will always be set to be copied regardless of their language."));
  ui->tbMEnableMuxingTracksByLanguage->setToolTips(QY("Tracks with a language in this list will be set not to be copied by default."),
                                                   QY("Only tracks with a language in this list will be set to be copied by default."));

  Util::setToolTip(ui->cbMEnableMuxingForcedSubtitleTracks,
                   Q("<p>%1 %2</p><ul><li>%3</li><li>%4</li><li>%5</li></ul>")
                   .arg(QYH("If enabled, forced subtitle tracks will always be set to be copied."))
                   .arg(QYH("A subtitle track is considered to be 'forced' track if any of the following conditions are met:"))
                   .arg(QYH("Its 'forced display' property is set in the source file."))
                   .arg(QYH("Its name contains the word 'forced' (in English)."))
                   .arg(QYH("Deriving the 'forced display' flag from file names is active & the file name matches the corresponding pattern.")));

  // Often used XYZ page
  ui->tbOftenUsedLanguages->setToolTips(QY("The languages in the 'selected' list on the right will be shown at the top of all the language drop-down boxes in the program."));
  Util::setToolTip(ui->cbOftenUsedLanguagesOnly,
                   Q("%1 %2")
                   .arg(QYH("If checked, only the list of often used entries will be included in the selections in the program."))
                   .arg(QYH("Otherwise the often used entries will be included first and the full list of all entries afterwards.")));

  ui->tbOftenUsedRegions->setToolTips(QY("The entries in the 'selected' list on the right will be shown at the top of all the drop-down boxes with countries and regions in the program."));
  Util::setToolTip(ui->cbOftenUsedRegionsOnly,
                   Q("%1 %2")
                   .arg(QYH("If checked, only the list of often used entries will be included in the selections in the program."))
                   .arg(QYH("Otherwise the often used entries will be included first and the full list of all entries afterwards.")));

  ui->tbOftenUsedCharacterSets->setToolTips(QY("The character sets in the 'selected' list on the right will be shown at the top of all the character set drop-down boxes in the program."));
  Util::setToolTip(ui->cbOftenUsedCharacterSetsOnly,
                   Q("%1 %2")
                   .arg(QYH("If checked, only the list of often used entries will be included in the selections in the program."))
                   .arg(QYH("Otherwise the often used entries will be included first and the full list of all entries afterwards.")));

  // Header editor  page
  Util::setToolTip(ui->cbHEDroppedFilesPolicy,
                   Q("%1 %2 %3")
                   .arg(QY("When the user drags & drops files from an external application onto a header editor tab the GUI can take different actions."))
                   .arg(QY("The default is to ask the user what to do with the dropped files."))
                   .arg(QY("Apart from asking the GUI can always open the dropped files as new tabs or it can always add them as new attachments to the current tab.")));
}

void
PreferencesDialog::setupLineEditWithDefaultText(QPushButton *revertButton,
                                                QLineEdit *edit,
                                                QString const &defaultText) {
  connect(edit, &QLineEdit::textChanged, this, [revertButton, defaultText](QString const &newText) {
    revertButton->setEnabled(newText != defaultText);
  });

  connect(revertButton, &QPushButton::clicked, this, [edit, defaultText]() {
    edit->setText(defaultText);
  });

  revertButton->setEnabled(edit->text() != defaultText);
}

void
PreferencesDialog::setupConnections() {
  connect(ui->cbUseISO639_3Languages,                      &QCheckBox::toggled,                                           this,                                 &PreferencesDialog::setupCommonLanguages);

  connect(ui->twGuiLanguageShortcuts,                      &QTreeWidget::itemSelectionChanged,                            this,                                 &PreferencesDialog::enableLanguageShortcutControls);
  connect(ui->twGuiLanguageShortcuts,                      &QTreeWidget::itemDoubleClicked,                               this,                                 &PreferencesDialog::editLanguageShortcut);
  connect(ui->pbGuiAddLanguageShortcut,                    &QPushButton::clicked,                                         this,                                 &PreferencesDialog::addLanguageShortcut);
  connect(ui->pbGuiEditLanguageShortcut,                   &QPushButton::clicked,                                         this,                                 &PreferencesDialog::editLanguageShortcut);
  connect(ui->pbGuiRemoveLanguageShortcuts,                &QPushButton::clicked,                                         this,                                 &PreferencesDialog::removeLanguageShortcuts);

  connect(ui->pbMEditDefaultAdditionalCommandLineOptions,  &QPushButton::clicked,                                         this,                                 &PreferencesDialog::editDefaultAdditionalCommandLineOptions);

  connect(ui->pbMBrowseMediaInfoExe,                       &QPushButton::clicked,                                         this,                                 &PreferencesDialog::browseMediaInfoExe);
  connect(ui->cbMAutoSetDestinationFileName,               &QCheckBox::toggled,                                           this,                                 &PreferencesDialog::enableOutputFileNameControls);
  connect(ui->rbMAutoSetDestinationSameDirectory,          &QRadioButton::toggled,                                        this,                                 &PreferencesDialog::enableOutputFileNameControls);
  connect(ui->rbMAutoSetDestinationRelativeDirectory,      &QRadioButton::toggled,                                        this,                                 &PreferencesDialog::enableOutputFileNameControls);
  connect(ui->rbMAutoSetDestinationPreviousDirectory,      &QRadioButton::toggled,                                        this,                                 &PreferencesDialog::enableOutputFileNameControls);
  connect(ui->rbMAutoSetDestinationFixedDirectory,         &QRadioButton::toggled,                                        this,                                 &PreferencesDialog::enableOutputFileNameControls);
  connect(ui->pbMAutoSetDestinationBrowseFixedDirectory,   &QPushButton::clicked,                                         this,                                 &PreferencesDialog::browseFixedOutputDirectory);

  connect(ui->cbMEnableMuxingTracksByLanguage,             &QCheckBox::toggled,                                           ui->lMEnableMuxingAllTracksOfType,    &QLabel::setEnabled);
  connect(ui->cbMEnableMuxingTracksByLanguage,             &QCheckBox::toggled,                                           ui->cbMEnableMuxingAllVideoTracks,    &QLabel::setEnabled);
  connect(ui->cbMEnableMuxingTracksByLanguage,             &QCheckBox::toggled,                                           ui->cbMEnableMuxingAllAudioTracks,    &QLabel::setEnabled);
  connect(ui->cbMEnableMuxingTracksByLanguage,             &QCheckBox::toggled,                                           ui->cbMEnableMuxingAllSubtitleTracks, &QLabel::setEnabled);
  connect(ui->cbMEnableMuxingTracksByLanguage,             &QCheckBox::toggled,                                           ui->tbMEnableMuxingTracksByLanguage,  &QLabel::setEnabled);

  connect(ui->cbMDeriveCommentaryFlag,                     &QCheckBox::toggled,                                           this,                                 &PreferencesDialog::enableDeriveCommentaryFlagControls);
  connect(ui->cbMDeriveHearingImpairedFlag,                &QCheckBox::toggled,                                           this,                                 &PreferencesDialog::enableDeriveHearingImpairedFlagControls);
  connect(ui->cbMDeriveForcedDisplayFlagSubtitles,         &QCheckBox::toggled,                                           this,                                 &PreferencesDialog::enableDeriveForcedDisplayFlagSubtitlesControls);
  connect(ui->cbMSetOriginalLanguageFlagLanguage,          &QCheckBox::toggled,                                           this,                                 &PreferencesDialog::enableSetOriginalLanguageFlagControls);

  connect(ui->cbGuiRemoveJobs,                             &QCheckBox::toggled,                                           ui->cbGuiJobRemovalPolicy,            &QComboBox::setEnabled);
  connect(ui->cbGuiRemoveJobsOnExit,                       &QCheckBox::toggled,                                           ui->cbGuiJobRemovalOnExitPolicy,      &QComboBox::setEnabled);
  connect(ui->cbGuiRemoveOldJobs,                          &QCheckBox::toggled,                                           this,                                 &PreferencesDialog::adjustRemoveOldJobsControls);
  connect(ui->sbGuiRemoveOldJobsDays,                      static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this,                                 &PreferencesDialog::adjustRemoveOldJobsControls);

  connect(ui->sbMMinPlaylistDuration,                      static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this,                                 &PreferencesDialog::adjustPlaylistControls);

  connect(ui->buttons,                                     &QDialogButtonBox::accepted,                                   this,                                 &PreferencesDialog::accept);
  connect(ui->buttons,                                     &QDialogButtonBox::rejected,                                   this,                                 &PreferencesDialog::reject);

  connect(ui->tbOftenUsedLanguages,                        &Util::SideBySideMultiSelect::listsChanged,                    this,                                 &PreferencesDialog::enableOftendUsedLanguagesOnly);
  connect(ui->tbOftenUsedRegions,                          &Util::SideBySideMultiSelect::listsChanged,                    this,                                 &PreferencesDialog::enableOftendUsedRegionsOnly);
  connect(ui->tbOftenUsedCharacterSets,                    &Util::SideBySideMultiSelect::listsChanged,                    this,                                 &PreferencesDialog::enableOftendUsedCharacterSetsOnly);

  setupLineEditWithDefaultText(ui->pbMDeriveTrackLanguageRevertBoundaryChars,   ui->leMDeriveTrackLanguageBoundaryChars,   Util::Settings::defaultBoundaryCharsForDerivingLanguageFromFileName());
  setupLineEditWithDefaultText(ui->pbMDeriveCommentaryFlagRERevert,             ui->leMDeriveCommentaryFlagRE,             Util::Settings::defaultRegexForDerivingCommentaryFlagFromFileName());
  setupLineEditWithDefaultText(ui->pbMDeriveHearingImpairedFlagRERevert,        ui->leMDeriveHearingImpairedFlagRE,        Util::Settings::defaultRegexForDerivingHearingImpairedFlagFromFileName());
  setupLineEditWithDefaultText(ui->pbMDeriveForcedDisplayFlagSubtitlesRERevert, ui->leMDeriveForcedDisplayFlagSubtitlesRE, Util::Settings::defaultRegexForDerivingForcedDisplayFlagForSubtitlesFromFileName());
}

void
PreferencesDialog::setupInterfaceLanguage() {
#if defined(HAVE_LIBINTL_H)
  using TranslationSorter = std::pair<QString, QString>;
  auto translations       = std::vector<TranslationSorter>{};

  for (auto const &translation : translation_c::ms_available_translations)
    translations.emplace_back(Q("%1 (%2)").arg(Q(translation.m_translated_name)).arg(Q(translation.m_english_name)), Q(translation.get_locale()));

  std::sort(translations.begin(), translations.end());

  for (auto const &translation : translations)
    ui->cbGuiInterfaceLanguage->addItem(translation.first, translation.second);

  if (!Util::setComboBoxTextByData(ui->cbGuiInterfaceLanguage, m_cfg.m_uiLocale))
    Util::setComboBoxTextByData(ui->cbGuiInterfaceLanguage, Q(translation_c::ms_available_translations[0].get_locale()));

  Util::fixComboBoxViewWidth(*ui->cbGuiInterfaceLanguage);
#endif  // HAVE_LIBINTL_H
}

void
PreferencesDialog::setupPalette() {
#if defined(SYS_WINDOWS)
  ui->cbGuiPalette->setCurrentIndex(static_cast<int>(m_cfg.m_uiPalette));

#else
  ui->cbGuiPalette->hide();
  ui->cbGuiPaletteLabel->hide();
#endif
}

void
PreferencesDialog::setupJobRemovalPolicy() {
  auto doIt = [](Util::Settings::JobRemovalPolicy policy,
                 QCheckBox &checkBox,
                 QComboBox &comboBox) {
    auto doRemove = Util::Settings::JobRemovalPolicy::Never != policy;
    auto idx      = std::max(static_cast<int>(policy), 1) - 1;

    checkBox.setChecked(doRemove);
    comboBox.setEnabled(doRemove);
    comboBox.setCurrentIndex(idx);
  };

  doIt(m_cfg.m_jobRemovalPolicy,       *ui->cbGuiRemoveJobs,       *ui->cbGuiJobRemovalPolicy);
  doIt(m_cfg.m_jobRemovalOnExitPolicy, *ui->cbGuiRemoveJobsOnExit, *ui->cbGuiJobRemovalOnExitPolicy);
}

void
PreferencesDialog::setupCommonLanguages(bool withISO639_3) {
  auto &allLanguages = withISO639_3 ? App::iso639Languages() : App::iso639_2Languages();
  auto languageItems = QList<Util::SideBySideMultiSelect::Item>::fromVector(Util::stdVectorToQVector<Util::SideBySideMultiSelect::Item>(allLanguages));

  ui->tbOftenUsedLanguages->setItems(languageItems, m_cfg.m_oftenUsedLanguages);
  ui->cbOftenUsedLanguagesOnly->setChecked(m_cfg.m_oftenUsedLanguagesOnly && !m_cfg.m_oftenUsedLanguages.isEmpty());
  enableOftendUsedLanguagesOnly();

  ui->tbMEnableMuxingTracksByLanguage->setItems(languageItems, m_cfg.m_enableMuxingTracksByTheseLanguages);
  ui->tbMDeriveTrackLanguageRecognizedLanguages->setItems(languageItems, m_cfg.m_recognizedTrackLanguagesInFileNames);
}

void
PreferencesDialog::setupCommonRegions() {
  auto &allRegions = App::regions();

  ui->tbOftenUsedRegions->setItems(QList<Util::SideBySideMultiSelect::Item>::fromVector(Util::stdVectorToQVector<Util::SideBySideMultiSelect::Item>(allRegions)), m_cfg.m_oftenUsedRegions);
  ui->cbOftenUsedRegionsOnly->setChecked(m_cfg.m_oftenUsedRegionsOnly && !m_cfg.m_oftenUsedRegions.isEmpty());
  enableOftendUsedRegionsOnly();
}

void
PreferencesDialog::setupCommonCharacterSets() {
  ui->tbOftenUsedCharacterSets->setItems(QList<QString>::fromVector(Util::stdVectorToQVector<QString>(App::characterSets())), m_cfg.m_oftenUsedCharacterSets);
  ui->cbOftenUsedCharacterSetsOnly->setChecked(m_cfg.m_oftenUsedCharacterSetsOnly && !m_cfg.m_oftenUsedCharacterSets.isEmpty());
  enableOftendUsedCharacterSetsOnly();
}

void
PreferencesDialog::setupProcessPriority() {
#if defined(SYS_WINDOWS)
  ui->cbMProcessPriority->addItem(QY("Highest priority"), static_cast<int>(Util::Settings::HighestPriority)); // value 4, index 0
  ui->cbMProcessPriority->addItem(QY("Higher priority"),  static_cast<int>(Util::Settings::HighPriority));    // value 3, index 1
#endif
  ui->cbMProcessPriority->addItem(QY("Normal priority"),  static_cast<int>(Util::Settings::NormalPriority));  // value 2, index 2/0
  ui->cbMProcessPriority->addItem(QY("Lower priority"),   static_cast<int>(Util::Settings::LowPriority));     // value 1, index 3/1
  ui->cbMProcessPriority->addItem(QY("Lowest priority"),  static_cast<int>(Util::Settings::LowestPriority));  // value 0, index 4/2

  auto numPrios = ui->cbMProcessPriority->count();
  auto selected = 4 - static_cast<int>(m_cfg.m_priority) - (5 - numPrios);
  selected      = std::min<int>(std::max<int>(0, selected), numPrios);

  ui->cbMProcessPriority->setCurrentIndex(selected);
}

void
PreferencesDialog::setupPlaylistScanningPolicy() {
  auto selected = std::max(std::min(static_cast<int>(m_cfg.m_scanForPlaylistsPolicy), 2), 0);

  ui->cbMScanPlaylistsPolicy->setCurrentIndex(selected);
  ui->sbMMinPlaylistDuration->setValue(m_cfg.m_minimumPlaylistDuration);
  ui->cbMIgnorePlaylistsForMenus->setChecked(m_cfg.m_ignorePlaylistsForMenus);

  adjustPlaylistControls();

  Util::fixComboBoxViewWidth(*ui->cbMScanPlaylistsPolicy);
}

void
PreferencesDialog::setupOutputFileNamePolicy() {
  auto isChecked = Util::Settings::DontSetOutputFileName != m_cfg.m_outputFileNamePolicy;
  auto rbToCheck = Util::Settings::ToPreviousDirectory        == m_cfg.m_outputFileNamePolicy ? ui->rbMAutoSetDestinationPreviousDirectory
                 : Util::Settings::ToFixedDirectory           == m_cfg.m_outputFileNamePolicy ? ui->rbMAutoSetDestinationFixedDirectory
                 : Util::Settings::ToRelativeOfFirstInputFile == m_cfg.m_outputFileNamePolicy ? ui->rbMAutoSetDestinationRelativeDirectory
                 :                                                                              ui->rbMAutoSetDestinationSameDirectory;

  auto dFixed    = QDir::toNativeSeparators(m_cfg.m_fixedOutputDir.path());
  auto dRelative = QDir::toNativeSeparators(m_cfg.m_relativeOutputDir.path());

  m_cfg.m_mergeLastFixedOutputDirs.add(dFixed);
  m_cfg.m_mergeLastRelativeOutputDirs.add(dRelative);

  ui->cbMAutoSetDestinationFileName->setChecked(isChecked);
  ui->cbMAutoSetDestinationOnlyForVideoFiles->setChecked(m_cfg.m_autoDestinationOnlyForVideoFiles);
  ui->cbMAutoSetDestinationFromTitle->setChecked(m_cfg.m_mergeSetDestinationFromTitle);
  ui->cbMAutoSetDestinationFromDirectory->setChecked(m_cfg.m_mergeSetDestinationFromDirectory);
  rbToCheck->setChecked(true);
  ui->cbMAutoSetDestinationRelativeDirectory->addItems(m_cfg.m_mergeLastRelativeOutputDirs.items());
  ui->cbMAutoSetDestinationRelativeDirectory->setCurrentText(dRelative);
  ui->cbMAutoSetDestinationRelativeDirectory->lineEdit()->setClearButtonEnabled(true);
  ui->cbMAutoSetDestinationFixedDirectory->addItems(m_cfg.m_mergeLastFixedOutputDirs.items());
  ui->cbMAutoSetDestinationFixedDirectory->setCurrentText(dFixed);
  ui->cbMAutoSetDestinationFixedDirectory->lineEdit()->setClearButtonEnabled(true);
  ui->cbMUniqueOutputFileNames->setChecked(m_cfg.m_uniqueOutputFileNames);
  ui->cbMAutoClearOutputFileName->setChecked(m_cfg.m_autoClearOutputFileName);

  enableOutputFileNameControls();
}

void
PreferencesDialog::setupRecentDestinationDirectoryList() {
  ui->lwMRecentDestinationDirectories->setItems(m_cfg.m_mergeLastOutputDirs.items());
  ui->lwMRecentDestinationDirectories->setMaximumNumItems(m_cfg.m_mergeLastOutputDirs.maximumNumItems());
  ui->lwMRecentDestinationDirectories->setAddItemDialogTexts(QY("Select a directory"), {});
  ui->lwMRecentDestinationDirectories->setItemType(Util::StringListConfigurationWidget::ItemType::Directory);
}

void
PreferencesDialog::setupEnableMuxingTracksByType() {
  auto allTypes      = Util::SideBySideMultiSelect::ItemList{};
  auto selectedTypes = QStringList{};

  for (auto type = static_cast<int>(Merge::TrackType::Min); type <= static_cast<int>(Merge::TrackType::Max); ++type)
    allTypes << std::make_pair(Merge::Track::nameForType(static_cast<Merge::TrackType>(type)), QString::number(type));

  for (auto type : m_cfg.m_enableMuxingTracksByTheseTypes)
    selectedTypes << QString::number(static_cast<int>(type));

  ui->tbMEnableMuxingTracksByType->setItems(allTypes, selectedTypes);
}

void
PreferencesDialog::setupEnableMuxingTracksByLanguage() {
  auto widgets = QList<QWidget *>{} << ui->lMEnableMuxingAllTracksOfType << ui->cbMEnableMuxingAllVideoTracks << ui->cbMEnableMuxingAllAudioTracks << ui->cbMEnableMuxingAllSubtitleTracks << ui->tbMEnableMuxingTracksByLanguage;
  for (auto const &widget : widgets)
    widget->setEnabled(m_cfg.m_enableMuxingTracksByLanguage);

  ui->cbMEnableMuxingTracksByLanguage->setChecked(m_cfg.m_enableMuxingTracksByLanguage);
  ui->cbMEnableMuxingAllVideoTracks->setChecked(m_cfg.m_enableMuxingAllVideoTracks);
  ui->cbMEnableMuxingAllAudioTracks->setChecked(m_cfg.m_enableMuxingAllAudioTracks);
  ui->cbMEnableMuxingAllSubtitleTracks->setChecked(m_cfg.m_enableMuxingAllSubtitleTracks);
}

void
PreferencesDialog::setupMergeAddingAppendingFilesPolicy() {
  auto setup = [](QComboBox &cb, Util::Settings::MergeAddingAppendingFilesPolicy policy) {
    cb.addItem(QY("Always ask the user"),                                           static_cast<int>(Util::Settings::MergeAddingAppendingFilesPolicy::Ask));
    cb.addItem(QY("Add all files to the current multiplex settings"),               static_cast<int>(Util::Settings::MergeAddingAppendingFilesPolicy::Add));
    cb.addItem(QY("Create one new multiplex settings tab and add all files there"), static_cast<int>(Util::Settings::MergeAddingAppendingFilesPolicy::AddToNew));
    cb.addItem(QY("Create one new multiplex settings tab for each file"),           static_cast<int>(Util::Settings::MergeAddingAppendingFilesPolicy::AddEachToNew));

    Util::setComboBoxIndexIf(&cb, [policy](QString const &, QVariant const &data) {
      return data.isValid() && (static_cast<Util::Settings::MergeAddingAppendingFilesPolicy>(data.toInt()) == policy);
    });

    Util::fixComboBoxViewWidth(cb);
  };

  setup(*ui->cbMAddingAppendingFilesPolicy, m_cfg.m_mergeAddingAppendingFilesPolicy);
  setup(*ui->cbMDragAndDropFilesPolicy,     m_cfg.m_mergeDragAndDropFilesPolicy);

  ui->cbMAlwaysCreateSettingsForVideoFiles->setChecked(m_cfg.m_mergeAlwaysCreateNewSettingsForVideoFiles);
}

void
PreferencesDialog::setupMergeAddingAppendingDirectoriesPolicy() {
  ui->cbMDragAndDropDirectoriesPolicy->addItem(QY("Always ask the user"),                                             static_cast<int>(Util::Settings::MergeAddingDirectoriesPolicy::Ask));
  ui->cbMDragAndDropDirectoriesPolicy->addItem(QY("Handle all files from all directories as a single list of files"), static_cast<int>(Util::Settings::MergeAddingDirectoriesPolicy::Flat));
  ui->cbMDragAndDropDirectoriesPolicy->addItem(QY("Create a new multiplex settings tab for each directory"),          static_cast<int>(Util::Settings::MergeAddingDirectoriesPolicy::AddEachDirectoryToNew));

  Util::setComboBoxIndexIf(ui->cbMDragAndDropDirectoriesPolicy, [this](QString const &, QVariant const &data) {
    return data.isValid() && (static_cast<Util::Settings::MergeAddingDirectoriesPolicy>(data.toInt()) == m_cfg.m_mergeDragAndDropDirectoriesPolicy);
  });

  Util::fixComboBoxViewWidth(*ui->cbMDragAndDropDirectoriesPolicy);
}

void
PreferencesDialog::setupMergeWarnMissingAudioTrack() {
  ui->cbMWarnMissingAudioTrack->addItem(QY("Never"),                                           static_cast<int>(Util::Settings::MergeMissingAudioTrackPolicy::Never));
  ui->cbMWarnMissingAudioTrack->addItem(QY("If audio tracks are present but none is enabled"), static_cast<int>(Util::Settings::MergeMissingAudioTrackPolicy::IfAudioTrackPresent));
  ui->cbMWarnMissingAudioTrack->addItem(QY("Even if no audio tracks are present"),             static_cast<int>(Util::Settings::MergeMissingAudioTrackPolicy::Always));

  Util::setComboBoxIndexIf(ui->cbMWarnMissingAudioTrack, [this](QString const &, QVariant const &data) {
    return data.isValid() && (static_cast<Util::Settings::MergeMissingAudioTrackPolicy>(data.toInt()) == m_cfg.m_mergeWarnMissingAudioTrack);
  });

  Util::fixComboBoxViewWidth(*ui->cbMWarnMissingAudioTrack);
}

void
PreferencesDialog::setupMergePredefinedItems() {
  auto &cfg = Util::Settings::get();

  ui->lwMPredefinedVideoTrackNames->setItems(cfg.m_mergePredefinedVideoTrackNames);
  ui->lwMPredefinedAudioTrackNames->setItems(cfg.m_mergePredefinedAudioTrackNames);
  ui->lwMPredefinedSubtitleTrackNames->setItems(cfg.m_mergePredefinedSubtitleTrackNames);
  ui->lwMPredefinedSplitSizes->setItems(cfg.m_mergePredefinedSplitSizes);
  ui->lwMPredefinedSplitDurations->setItems(cfg.m_mergePredefinedSplitDurations);

  auto widgets = QList<mtx::gui::Util::StringListConfigurationWidget *>{} << ui->lwMPredefinedVideoTrackNames << ui->lwMPredefinedAudioTrackNames << ui->lwMPredefinedSubtitleTrackNames;
  for (auto const &widget : widgets)
    widget->setAddItemDialogTexts(QY("Enter predefined track name"), QY("Please enter the new predefined track name."));

  ui->lwMPredefinedSplitSizes->setAddItemDialogTexts(QY("Enter predefined split size"), QY("Please enter the new predefined split size."));
  ui->lwMPredefinedSplitDurations->setAddItemDialogTexts(QY("Enter predefined split duration"), QY("Please enter the new predefined split duration."));
}

void
PreferencesDialog::setupHeaderEditorDroppedFilesPolicy() {
  ui->cbHEDroppedFilesPolicy->addItem(QY("Always ask the user"),                                 static_cast<int>(Util::Settings::HeaderEditorDroppedFilesPolicy::Ask));
  ui->cbHEDroppedFilesPolicy->addItem(QY("Open all files as tabs in the header editor"),         static_cast<int>(Util::Settings::HeaderEditorDroppedFilesPolicy::Open));
  ui->cbHEDroppedFilesPolicy->addItem(QY("Add all files as new attachments to the current tab"), static_cast<int>(Util::Settings::HeaderEditorDroppedFilesPolicy::AddAttachments));

  Util::setComboBoxIndexIf(ui->cbHEDroppedFilesPolicy, [this](QString const &, QVariant const &data) {
    return data.isValid() && (static_cast<Util::Settings::HeaderEditorDroppedFilesPolicy>(data.toInt()) == m_cfg.m_headerEditorDroppedFilesPolicy);
  });

  Util::fixComboBoxViewWidth(*ui->cbHEDroppedFilesPolicy);
}

void
PreferencesDialog::setupTrackPropertiesLayout() {
  auto rbToCheck = Util::Settings::TrackPropertiesLayout::HorizontalScrollArea == m_cfg.m_mergeTrackPropertiesLayout ? ui->rbMTrackPropertiesLayoutHorizontalScrollArea
                 : Util::Settings::TrackPropertiesLayout::HorizontalTwoColumns == m_cfg.m_mergeTrackPropertiesLayout ? ui->rbMTrackPropertiesLayoutHorizontalTwoColumns
                 :                                                                                                     ui->rbMTrackPropertiesLayoutVerticalTabWidget;

  rbToCheck->setChecked(true);
}

void
PreferencesDialog::setupBCP47LanguageEditMode() {
  ui->cbGuiBCP47LanguageEditingMode->clear();
  ui->cbGuiBCP47LanguageEditingMode->addItem(QY("Free-form input"),                  static_cast<int>(Util::Settings::BCP47LanguageEditingMode::FreeForm));
  ui->cbGuiBCP47LanguageEditingMode->addItem(QY("Individually selected components"), static_cast<int>(Util::Settings::BCP47LanguageEditingMode::Components));

  Util::setComboBoxIndexIf(ui->cbGuiBCP47LanguageEditingMode, [this](auto const &, auto const &data) {
    return data.toInt() == static_cast<int>(m_cfg.m_bcp47LanguageEditingMode);
  });
}

void
PreferencesDialog::setupBCP47NormalizationMode() {
  ui->cbGuiBCP47NormalizationMode->clear();
  ui->cbGuiBCP47NormalizationMode->addItem(QY("no normalization"),               static_cast<int>(mtx::bcp47::normalization_mode_e::none));
  ui->cbGuiBCP47NormalizationMode->addItem(QY("canonical form"),                 static_cast<int>(mtx::bcp47::normalization_mode_e::canonical));
  ui->cbGuiBCP47NormalizationMode->addItem(QY("extended language subtags form"), static_cast<int>(mtx::bcp47::normalization_mode_e::extlang));

  Util::setComboBoxIndexIf(ui->cbGuiBCP47NormalizationMode, [this](auto const &, auto const &data) {
    return data.toInt() == static_cast<int>(m_cfg.m_bcp47NormalizationMode);
  });
}

void
PreferencesDialog::setupTabPositions() {
  ui->cbGuiTabPositions->clear();
  ui->cbGuiTabPositions->addItem(QY("Top"),    static_cast<int>(QTabWidget::North));
  ui->cbGuiTabPositions->addItem(QY("Bottom"), static_cast<int>(QTabWidget::South));
  ui->cbGuiTabPositions->addItem(QY("Left"),   static_cast<int>(QTabWidget::West));
  ui->cbGuiTabPositions->addItem(QY("Right"),  static_cast<int>(QTabWidget::East));

  Util::setComboBoxIndexIf(ui->cbGuiTabPositions, [this](QString const &, QVariant const &data) {
    return data.toInt() == static_cast<int>(m_cfg.m_tabPosition);
  });
}

void
PreferencesDialog::setupDefaultCommandLineEscapeMode() {
  ui->cbMDefaultCommandLineEscapeMode->clear();
  for (auto const &mode : Merge::CommandLineDialog::supportedModes())
    ui->cbMDefaultCommandLineEscapeMode->addItem(mode.title);

  ui->cbMDefaultCommandLineEscapeMode->setCurrentIndex(m_cfg.m_mergeDefaultCommandLineEscapeMode);
}

void
PreferencesDialog::setupDeriveCommentaryFlag() {
  ui->cbMDeriveCommentaryFlag->setChecked(m_cfg.m_deriveCommentaryFlagFromFileNames);
  ui->leMDeriveCommentaryFlagRE->setText(m_cfg.m_regexForDerivingCommentaryFlagFromFileNames);

  enableDeriveCommentaryFlagControls();
}

void
PreferencesDialog::setupDeriveHearingImpairedFlag() {
  ui->cbMDeriveHearingImpairedFlag->setChecked(m_cfg.m_deriveHearingImpairedFlagFromFileNames);
  ui->leMDeriveHearingImpairedFlagRE->setText(m_cfg.m_regexForDerivingHearingImpairedFlagFromFileNames);

  enableDeriveHearingImpairedFlagControls();
}

void
PreferencesDialog::setupDeriveForcedDisplayFlagSubtitles() {
  ui->cbMDeriveForcedDisplayFlagSubtitles->setChecked(m_cfg.m_deriveSubtitlesForcedFlagFromFileNames);
  ui->leMDeriveForcedDisplayFlagSubtitlesRE->setText(m_cfg.m_regexForDerivingSubtitlesForcedFlagFromFileNames);

  enableDeriveForcedDisplayFlagSubtitlesControls();
}

void
PreferencesDialog::setupSetOriginalLanguageFlag() {
  ui->cbMSetOriginalLanguageFlagLanguage->setChecked(m_cfg.m_defaultSetOriginalLanguageFlagLanguage.is_valid());
  ui->ldwMSetOriginalLanguageFlagLanguage->setLanguage(m_cfg.m_defaultSetOriginalLanguageFlagLanguage);

  enableSetOriginalLanguageFlagControls();
}

void
PreferencesDialog::enableDeriveCommentaryFlagControls() {
  auto enable = ui->cbMDeriveCommentaryFlag->isChecked();

  ui->leMDeriveCommentaryFlagRE->setEnabled(enable);
  ui->pbMDeriveCommentaryFlagRERevert->setEnabled(enable);
}

void
PreferencesDialog::enableDeriveHearingImpairedFlagControls() {
  auto enable = ui->cbMDeriveHearingImpairedFlag->isChecked();

  ui->leMDeriveHearingImpairedFlagRE->setEnabled(enable);
  ui->pbMDeriveHearingImpairedFlagRERevert->setEnabled(enable);
}

void
PreferencesDialog::enableDeriveForcedDisplayFlagSubtitlesControls() {
  auto enable = ui->cbMDeriveForcedDisplayFlagSubtitles->isChecked();

  ui->leMDeriveForcedDisplayFlagSubtitlesRE->setEnabled(enable);
  ui->pbMDeriveForcedDisplayFlagSubtitlesRERevert->setEnabled(enable);
}

void
PreferencesDialog::enableSetOriginalLanguageFlagControls() {
  auto enable = ui->cbMSetOriginalLanguageFlagLanguage->isChecked();

  ui->ldwMSetOriginalLanguageFlagLanguage->setEnabled(enable);
}

void
PreferencesDialog::setupDerivingTrackLanguagesFromFileName() {
  auto setupComboBox = [](QComboBox &cb, Util::Settings::DeriveLanguageFromFileNamePolicy policy) {
    cb.clear();
    cb.addItem(QY("Never"),                                          static_cast<int>(Util::Settings::DeriveLanguageFromFileNamePolicy::Never));
    cb.addItem(QY("Only if the source doesn't contain a language"),  static_cast<int>(Util::Settings::DeriveLanguageFromFileNamePolicy::OnlyIfAbsent));
    cb.addItem(QY("Also if the language is 'undetermined' ('und')"), static_cast<int>(Util::Settings::DeriveLanguageFromFileNamePolicy::IfAbsentOrUndetermined));

    Util::fixComboBoxViewWidth(cb);

    Util::setComboBoxIndexIf(&cb, [policy](QString const &, QVariant const &data) {
      return data.toInt() == static_cast<int>(policy);
    });
  };

  setupComboBox(*ui->cbMDeriveAudioTrackLanguageFromFileName,    m_cfg.m_deriveAudioTrackLanguageFromFileNamePolicy);
  setupComboBox(*ui->cbMDeriveVideoTrackLanguageFromFileName,    m_cfg.m_deriveVideoTrackLanguageFromFileNamePolicy);
  setupComboBox(*ui->cbMDeriveSubtitleTrackLanguageFromFileName, m_cfg.m_deriveSubtitleTrackLanguageFromFileNamePolicy);

  ui->leMDeriveTrackLanguageBoundaryChars->setText(m_cfg.m_boundaryCharsForDerivingTrackLanguagesFromFileNames);
}

void
PreferencesDialog::setupWhenToSetDefaultLanguage() {
  ui->cbMWhenToSetDefaultLanguage->clear();
  ui->cbMWhenToSetDefaultLanguage->addItem(QY("Only if the source doesn't contain a language"),  static_cast<int>(Util::Settings::SetDefaultLanguagePolicy::OnlyIfAbsent));
  ui->cbMWhenToSetDefaultLanguage->addItem(QY("Also if the language is 'undetermined' ('und')"), static_cast<int>(Util::Settings::SetDefaultLanguagePolicy::IfAbsentOrUndetermined));
  ui->cbMWhenToSetDefaultLanguage->addItem(QY("Always"),                                         static_cast<int>(Util::Settings::SetDefaultLanguagePolicy::Always));

  Util::setComboBoxIndexIf(ui->cbMWhenToSetDefaultLanguage, [this](QString const &, QVariant const &data) {
    return data.toInt() == static_cast<int>(m_cfg.m_whenToSetDefaultLanguage);
  });
}

void
PreferencesDialog::setupJobsRunPrograms() {
  ui->twJobsPrograms->setTabPosition(m_cfg.m_tabPosition);

  for (auto const &runProgramConfig : m_cfg.m_runProgramConfigurations) {
    auto widget = new PrefsRunProgramWidget{ui->twJobsPrograms, *runProgramConfig};
    ui->twJobsPrograms->addTab(widget, {});

    setTabTitleForRunProgramWidget(ui->twJobsPrograms->count() - 1, runProgramConfig->name());

    connect(widget, &PrefsRunProgramWidget::titleChanged, this, &PreferencesDialog::setSendersTabTitleForRunProgramWidget);
  }

  if (!m_cfg.m_runProgramConfigurations.isEmpty())
    ui->swJobsPrograms->setCurrentIndex(1);

  connect(ui->pbJobsAddProgram, &QPushButton::clicked,          this, &PreferencesDialog::addProgramToExecute);
  connect(ui->twJobsPrograms,   &QTabWidget::tabCloseRequested, this, &PreferencesDialog::removeProgramToExecute);
}

void
PreferencesDialog::setupFontAndScaling() {
  auto font = App::font();
  ui->fcbGuiFontFamily->setCurrentFont(font);
  ui->sbGuiFontPointSize->setValue(font.pointSize());

  ui->cbGuiDisableHighDPIScaling->setChecked(m_cfg.m_uiDisableHighDPIScaling);
  ui->cbGuiDisableHighDPIScaling->setVisible(false);

  ui->cbGuiStayOnTop->setChecked(m_cfg.m_uiStayOnTop);
}

void
PreferencesDialog::setupFileColorsControls() {
  ui->cbMUseFileAndTrackColors->setChecked(m_cfg.m_mergeUseFileAndTrackColors);
  setupFileColors(m_cfg.m_mergeFileColors);

  enableFileColorsControls();

  connect(ui->cbMUseFileAndTrackColors,     &QCheckBox::toggled,                this, &PreferencesDialog::enableFileColorsControls);
  connect(ui->lwMFileColors,                &QListWidget::itemSelectionChanged, this, &PreferencesDialog::enableFileColorsControls);
  connect(ui->lwMFileColors,                &QListWidget::itemDoubleClicked,    this, &PreferencesDialog::editFileColor);
  connect(ui->pbMAddFileColor,              &QPushButton::clicked,              this, &PreferencesDialog::addFileColor);
  connect(ui->pbMRemoveFileColors,          &QPushButton::clicked,              this, &PreferencesDialog::removeFileColors);
  connect(ui->pbMEditFileColor,             &QPushButton::clicked,              this, &PreferencesDialog::editSelectedFileColor);
  connect(ui->pbMRevertFileColorsToDefault, &QPushButton::clicked,              this, &PreferencesDialog::revertFileColorsToDefault);
}

QListWidgetItem &
PreferencesDialog::setupFileColorItem(QListWidgetItem &item,
                                      QColor const &color) {
  auto luminance  = (0.299 * color.red() + 0.587 * color.green() + 0.114 * color.blue()) / 255.0;
  auto foreground = luminance > 0.5 ?  0 : 255;

  item.setForeground(QColor{foreground, foreground, foreground});
  item.setBackground(color);
  item.setText(Q(fmt::format("0x{0:02x}{1:02x}{2:02x}", color.red(), color.green(), color.blue())));

  return item;
}

void
PreferencesDialog::setupFileColors(QVector<QColor> const &colors) {
  ui->lwMFileColors->clear();

  for (auto const &color : colors)
    ui->lwMFileColors->addItem(&setupFileColorItem(*new QListWidgetItem, color));

  enableFileColorsControls();
}

void
PreferencesDialog::saveFileColors() {
  auto &colors = m_cfg.m_mergeFileColors;
  auto numRows = ui->lwMFileColors->count();

  colors.clear();
  colors.reserve(numRows);

  for (int row = 0; row < numRows; ++row)
    colors << ui->lwMFileColors->item(row)->background().color();

  m_cfg.m_mergeUseFileAndTrackColors = ui->cbMUseFileAndTrackColors->isChecked();
}

void
PreferencesDialog::enableFileColorsControls() {
  auto useColors = ui->cbMUseFileAndTrackColors->isChecked();
  auto items     = ui->lwMFileColors->selectedItems();

  ui->lwMFileColors->setEnabled(useColors);
  ui->pbMAddFileColor->setEnabled(useColors);
  ui->pbMRevertFileColorsToDefault->setEnabled(useColors);
  ui->pbMRemoveFileColors->setEnabled(useColors && !items.isEmpty());
  ui->pbMEditFileColor->setEnabled(useColors && (items.size() == 1));
}

void
PreferencesDialog::addFileColor() {
  QColorDialog dlg{this};

  if (dlg.exec())
    ui->lwMFileColors->addItem(&setupFileColorItem(*new QListWidgetItem, dlg.currentColor()));
}

void
PreferencesDialog::removeFileColors() {
  for (auto const &item : ui->lwMFileColors->selectedItems())
    delete item;

  enableFileColorsControls();
}

void
PreferencesDialog::editFileColor(QListWidgetItem *item) {
  if (!item)
    return;

  QColorDialog dlg{item->background().color(), this};

  if (dlg.exec())
    setupFileColorItem(*item, dlg.currentColor());
}

void
PreferencesDialog::editSelectedFileColor() {
  auto items = ui->lwMFileColors->selectedItems();
  if (!items.isEmpty())
    editFileColor(items.first());
}

void
PreferencesDialog::revertFileColorsToDefault() {
  setupFileColors(Util::Settings::defaultFileColors());
}

void
PreferencesDialog::save() {
  // GUI page:
  m_cfg.m_uiLocale                                            = ui->cbGuiInterfaceLanguage->currentData().toString();
  m_cfg.m_tabPosition                                         = static_cast<QTabWidget::TabPosition>(ui->cbGuiTabPositions->currentData().toInt());
  m_cfg.m_bcp47LanguageEditingMode                            = static_cast<Util::Settings::BCP47LanguageEditingMode>(ui->cbGuiBCP47LanguageEditingMode->currentData().toInt());
  m_cfg.m_bcp47NormalizationMode                              = static_cast<mtx::bcp47::normalization_mode_e>(ui->cbGuiBCP47NormalizationMode->currentData().toInt());
  m_cfg.m_uiFontFamily                                        = ui->fcbGuiFontFamily->currentFont().family();
  m_cfg.m_uiFontPointSize                                     = ui->sbGuiFontPointSize->value();
  m_cfg.m_uiPalette                                           = static_cast<Util::Settings::AppPalette>(ui->cbGuiPalette->currentIndex());
  m_cfg.m_numRecentlyUsedStringsToRemember                    = ui->sbGuiNumRecentlyUsedStrings->value();
  m_cfg.m_uiStayOnTop                                         = ui->cbGuiStayOnTop->isChecked();
  m_cfg.m_uiDisableHighDPIScaling                             = ui->cbGuiDisableHighDPIScaling->isChecked();
  m_cfg.m_uiDisableToolTips                                   = ui->cbGuiDisableToolTips->isChecked();
  m_cfg.m_checkForUpdates                                     = ui->cbGuiCheckForUpdates->isChecked();
  m_cfg.m_showDebuggingMenu                                   = ui->cbGuiShowDebuggingMenu->isChecked();
  m_cfg.m_showToolSelector                                    = ui->cbGuiShowToolSelector->isChecked();
  m_cfg.m_showMoveUpDownButtons                               = ui->cbGuiShowMoveUpDownButtons->isChecked();
  m_cfg.m_elideTabHeaderLabels                                = ui->cbGuiElideTabHeaderLabels->isChecked();
  m_cfg.m_useLegacyFontMIMETypes                              = ui->cbGuiUseLegacyFontMIMETypes->isChecked();
  m_cfg.m_warnBeforeClosingModifiedTabs                       = ui->cbGuiWarnBeforeClosingModifiedTabs->isChecked();
  m_cfg.m_warnBeforeAbortingJobs                              = ui->cbGuiWarnBeforeAbortingJobs->isChecked();
  m_cfg.m_warnBeforeOverwriting                               = ui->cbGuiWarnBeforeOverwriting->isChecked();
  m_cfg.m_useDefaultJobDescription                            = ui->cbGuiUseDefaultJobDescription->isChecked();
  m_cfg.m_showOutputOfAllJobs                                 = ui->cbGuiShowOutputOfAllJobs->isChecked();
  m_cfg.m_switchToJobOutputAfterStarting                      = ui->cbGuiSwitchToJobOutputAfterStarting->isChecked();
  m_cfg.m_resetJobWarningErrorCountersOnExit                  = ui->cbGuiResetJobWarningErrorCountersOnExit->isChecked();
  m_cfg.m_removeOutputFileOnJobFailure                        = ui->cbGuiRemoveOutputFileOnJobFailure->isChecked();
  auto idx                                                    = !ui->cbGuiRemoveJobs      ->isChecked() ? 0 : ui->cbGuiJobRemovalPolicy      ->currentIndex() + 1;
  auto idxOnExit                                              = !ui->cbGuiRemoveJobsOnExit->isChecked() ? 0 : ui->cbGuiJobRemovalOnExitPolicy->currentIndex() + 1;
  m_cfg.m_jobRemovalPolicy                                    = static_cast<Util::Settings::JobRemovalPolicy>(idx);
  m_cfg.m_jobRemovalOnExitPolicy                              = static_cast<Util::Settings::JobRemovalPolicy>(idxOnExit);
  m_cfg.m_removeOldJobs                                       = ui->cbGuiRemoveOldJobs->isChecked();
  m_cfg.m_removeOldJobsDays                                   = ui->sbGuiRemoveOldJobsDays->value();
  m_cfg.m_maximumConcurrentJobs                               = ui->sbGuiMaximumConcurrentJobs->value();

  m_cfg.m_chapterNameTemplate                                 = ui->leCENameTemplate->text();
  m_cfg.m_ceTextFileCharacterSet                              = ui->cbCETextFileCharacterSet->currentData().toString();
  m_cfg.m_defaultChapterLanguage                              = ui->ldwCEDefaultLanguage->language();
  m_cfg.m_dropLastChapterFromBlurayPlaylist                   = ui->cbCEDropLastFromBlurayPlaylist->isChecked();

  // Merge page:
  m_cfg.m_mergeDefaultCommandLineEscapeMode                   = ui->cbMDefaultCommandLineEscapeMode->currentIndex();
  m_cfg.m_mediaInfoExe                                        = ui->leMMediaInfoExe->text();
  m_cfg.m_mergeEnsureAtLeastOneTrackEnabled                   = ui->cbMEnsureAtLeastOneTrackEnabled->isChecked();
  m_cfg.m_autoSetFileTitle                                    = ui->cbMAutoSetFileTitle->isChecked();
  m_cfg.m_autoClearFileTitle                                  = ui->cbMAutoClearFileTitle->isChecked();
  m_cfg.m_setAudioDelayFromFileName                           = ui->cbMSetAudioDelayFromFileName->isChecked();
  m_cfg.m_disableCompressionForAllTrackTypes                  = ui->cbMDisableCompressionForAllTrackTypes->isChecked();
  m_cfg.m_disableDefaultTrackForSubtitles                     = ui->cbMDisableDefaultTrackForSubtitles->isChecked();
  m_cfg.m_mergeEnableDialogNormGainRemoval                    = ui->cbMEnableDialogNormGainRemoval->isChecked();
  m_cfg.m_mergeAddingAppendingFilesPolicy                     = static_cast<Util::Settings::MergeAddingAppendingFilesPolicy>(ui->cbMAddingAppendingFilesPolicy->currentData().toInt());
  m_cfg.m_mergeDragAndDropFilesPolicy                         = static_cast<Util::Settings::MergeAddingAppendingFilesPolicy>(ui->cbMDragAndDropFilesPolicy->currentData().toInt());
  m_cfg.m_mergeDragAndDropDirectoriesPolicy                   = static_cast<Util::Settings::MergeAddingDirectoriesPolicy>(ui->cbMDragAndDropDirectoriesPolicy->currentData().toInt());
  m_cfg.m_mergeAlwaysCreateNewSettingsForVideoFiles           = ui->cbMAlwaysCreateSettingsForVideoFiles->isChecked();
  m_cfg.m_mergeShowDNDZones                                   = ui->cbMShowDNDZones->isChecked();
  m_cfg.m_mergeSortFilesTracksByTypeWhenAdding                = ui->cbMSortFilesTracksByTypeWhenAdding->isChecked();
  m_cfg.m_mergeReconstructSequencesWhenAdding                 = ui->cbMReconstructSequencesWhenAdding->isChecked();
  m_cfg.m_mergeAlwaysShowOutputFileControls                   = ui->cbMAlwaysShowOutputFileControls->isChecked();
  m_cfg.m_mergeWarnMissingAudioTrack                          = static_cast<Util::Settings::MergeMissingAudioTrackPolicy>(ui->cbMWarnMissingAudioTrack->currentData().toInt());
  m_cfg.m_mergeTrackPropertiesLayout                          = ui->rbMTrackPropertiesLayoutHorizontalScrollArea->isChecked() ? Util::Settings::TrackPropertiesLayout::HorizontalScrollArea
                                                              : ui->rbMTrackPropertiesLayoutHorizontalTwoColumns->isChecked() ? Util::Settings::TrackPropertiesLayout::HorizontalTwoColumns
                                                              :                                                                 Util::Settings::TrackPropertiesLayout::VerticalTabWidget;
  m_cfg.m_clearMergeSettings                                  = static_cast<Util::Settings::ClearMergeSettingsAction>(ui->cbMClearMergeSettings->currentIndex());
  m_cfg.m_defaultAudioTrackLanguage                           = ui->ldwMDefaultAudioTrackLanguage->language();
  m_cfg.m_defaultVideoTrackLanguage                           = ui->ldwMDefaultVideoTrackLanguage->language();
  m_cfg.m_defaultSubtitleTrackLanguage                        = ui->ldwMDefaultSubtitleTrackLanguage->language();
  m_cfg.m_whenToSetDefaultLanguage                            = static_cast<Util::Settings::SetDefaultLanguagePolicy>(ui->cbMWhenToSetDefaultLanguage->currentData().toInt());
  m_cfg.m_deriveCommentaryFlagFromFileNames                   = ui->cbMDeriveCommentaryFlag->isChecked();
  m_cfg.m_regexForDerivingCommentaryFlagFromFileNames         = ui->leMDeriveCommentaryFlagRE->text();
  m_cfg.m_deriveHearingImpairedFlagFromFileNames              = ui->cbMDeriveHearingImpairedFlag->isChecked();
  m_cfg.m_regexForDerivingHearingImpairedFlagFromFileNames    = ui->leMDeriveHearingImpairedFlagRE->text();
  m_cfg.m_deriveSubtitlesForcedFlagFromFileNames              = ui->cbMDeriveForcedDisplayFlagSubtitles->isChecked();
  m_cfg.m_regexForDerivingSubtitlesForcedFlagFromFileNames    = ui->leMDeriveForcedDisplayFlagSubtitlesRE->text();
  m_cfg.m_deriveFlagsFromTrackNames                           = ui->cbMDeriveFlagsFromTrackNames->isChecked();
  m_cfg.m_defaultSetOriginalLanguageFlagLanguage              = ui->cbMSetOriginalLanguageFlagLanguage->isChecked() ? ui->ldwMSetOriginalLanguageFlagLanguage->language() : mtx::bcp47::language_c{};
  m_cfg.m_defaultSubtitleCharset                              = ui->cbMDefaultSubtitleCharset->currentData().toString();
  m_cfg.m_priority                                            = static_cast<Util::Settings::ProcessPriority>(ui->cbMProcessPriority->currentData().toInt());
  m_cfg.m_defaultAdditionalMergeOptions                       = ui->leMDefaultAdditionalCommandLineOptions->text();
  m_cfg.m_probeRangePercentage                                = ui->cbMProbeRangePercentage->value();

  m_cfg.m_deriveAudioTrackLanguageFromFileNamePolicy          = static_cast<Util::Settings::DeriveLanguageFromFileNamePolicy>(ui->cbMDeriveAudioTrackLanguageFromFileName   ->currentData().toInt());
  m_cfg.m_deriveVideoTrackLanguageFromFileNamePolicy          = static_cast<Util::Settings::DeriveLanguageFromFileNamePolicy>(ui->cbMDeriveVideoTrackLanguageFromFileName   ->currentData().toInt());
  m_cfg.m_deriveSubtitleTrackLanguageFromFileNamePolicy       = static_cast<Util::Settings::DeriveLanguageFromFileNamePolicy>(ui->cbMDeriveSubtitleTrackLanguageFromFileName->currentData().toInt());
  m_cfg.m_boundaryCharsForDerivingTrackLanguagesFromFileNames = ui->leMDeriveTrackLanguageBoundaryChars->text();
  m_cfg.m_recognizedTrackLanguagesInFileNames                 = ui->tbMDeriveTrackLanguageRecognizedLanguages->selectedItemValues();

  m_cfg.m_scanForPlaylistsPolicy                              = static_cast<Util::Settings::ScanForPlaylistsPolicy>(ui->cbMScanPlaylistsPolicy->currentIndex());
  m_cfg.m_minimumPlaylistDuration                             = ui->sbMMinPlaylistDuration->value();
  m_cfg.m_ignorePlaylistsForMenus                             = ui->cbMIgnorePlaylistsForMenus->isChecked();
  m_cfg.m_mergeAddBlurayCovers                                = ui->cbMAddBlurayCovers->isChecked();
  m_cfg.m_mergeAttachmentsAlwaysSkipForExistingName           = ui->cbMAttachmentAlwaysSkipForExistingName->isChecked();

  m_cfg.m_outputFileNamePolicy                                = !ui->cbMAutoSetDestinationFileName->isChecked()         ? Util::Settings::DontSetOutputFileName
                                                              : ui->rbMAutoSetDestinationRelativeDirectory->isChecked() ? Util::Settings::ToRelativeOfFirstInputFile
                                                              : ui->rbMAutoSetDestinationFixedDirectory->isChecked()    ? Util::Settings::ToFixedDirectory
                                                              : ui->rbMAutoSetDestinationPreviousDirectory->isChecked() ? Util::Settings::ToPreviousDirectory
                                                              :                                                           Util::Settings::ToSameAsFirstInputFile;
  m_cfg.m_autoDestinationOnlyForVideoFiles                    = ui->cbMAutoSetDestinationOnlyForVideoFiles->isChecked();
  m_cfg.m_mergeSetDestinationFromTitle                        = ui->cbMAutoSetDestinationFromTitle->isChecked();
  m_cfg.m_mergeSetDestinationFromDirectory                    = ui->cbMAutoSetDestinationFromDirectory->isChecked();
  m_cfg.m_relativeOutputDir.setPath(ui->cbMAutoSetDestinationRelativeDirectory->currentText());
  m_cfg.m_fixedOutputDir.setPath(ui->cbMAutoSetDestinationFixedDirectory->currentText());
  m_cfg.m_uniqueOutputFileNames                               = ui->cbMUniqueOutputFileNames->isChecked();
  m_cfg.m_autoClearOutputFileName                             = ui->cbMAutoClearOutputFileName->isChecked();

  m_cfg.m_mergeLastFixedOutputDirs.add(QDir::toNativeSeparators(m_cfg.m_fixedOutputDir.path()));
  m_cfg.m_mergeLastRelativeOutputDirs.add(QDir::toNativeSeparators(m_cfg.m_relativeOutputDir.path()));
  m_cfg.m_mergeLastOutputDirs.setItems(ui->lwMRecentDestinationDirectories->items());

  m_cfg.m_enableMuxingTracksByLanguage                        = ui->cbMEnableMuxingTracksByLanguage->isChecked();
  m_cfg.m_enableMuxingAllVideoTracks                          = ui->cbMEnableMuxingAllVideoTracks->isChecked();
  m_cfg.m_enableMuxingAllAudioTracks                          = ui->cbMEnableMuxingAllAudioTracks->isChecked();
  m_cfg.m_enableMuxingAllSubtitleTracks                       = ui->cbMEnableMuxingAllSubtitleTracks->isChecked();
  m_cfg.m_enableMuxingTracksByTheseLanguages                  = ui->tbMEnableMuxingTracksByLanguage->selectedItemValues();
  m_cfg.m_enableMuxingForcedSubtitleTracks                    = ui->cbMEnableMuxingForcedSubtitleTracks->isChecked();

  // Often used selections page:
  m_cfg.m_oftenUsedLanguages                                  = ui->tbOftenUsedLanguages->selectedItemValues();
  m_cfg.m_oftenUsedRegions                                    = ui->tbOftenUsedRegions->selectedItemValues();
  m_cfg.m_oftenUsedCharacterSets                              = ui->tbOftenUsedCharacterSets->selectedItemValues();

  m_cfg.m_oftenUsedLanguagesOnly                              = ui->cbOftenUsedLanguagesOnly    ->isChecked() && !m_cfg.m_oftenUsedLanguages    .isEmpty();
  m_cfg.m_oftenUsedRegionsOnly                                = ui->cbOftenUsedRegionsOnly      ->isChecked() && !m_cfg.m_oftenUsedRegions      .isEmpty();
  m_cfg.m_oftenUsedCharacterSetsOnly                          = ui->cbOftenUsedCharacterSetsOnly->isChecked() && !m_cfg.m_oftenUsedCharacterSets.isEmpty();
  m_cfg.m_useISO639_3Languages                                = ui->cbUseISO639_3Languages      ->isChecked();

  // Info tool page
  m_cfg.m_defaultInfoJobSettings                              = ui->wIDefaultJobSettings->settings();

  // Header editor page
  m_cfg.m_headerEditorDroppedFilesPolicy                      = static_cast<Util::Settings::HeaderEditorDroppedFilesPolicy>(ui->cbHEDroppedFilesPolicy->currentData().toInt());
  m_cfg.m_headerEditorDateTimeInUTC                           = ui->cbHEDateTimeInUTC->isChecked();

  // Run programs page:
  m_cfg.m_runProgramConfigurations.clear();
  for (auto tabIdx = 0, numTabs = ui->twJobsPrograms->count(); tabIdx < numTabs; ++tabIdx) {
    auto widget = static_cast<PrefsRunProgramWidget *>(ui->twJobsPrograms->widget(tabIdx));
    auto cfg    = widget->config();

    if (!cfg->m_active || cfg->isValid())
      m_cfg.m_runProgramConfigurations << cfg;
  }

  m_cfg.m_enableMuxingTracksByTheseTypes.clear();
  for (auto const &type : ui->tbMEnableMuxingTracksByType->selectedItemValues())
    m_cfg.m_enableMuxingTracksByTheseTypes << static_cast<Merge::TrackType>(type.toInt());

  m_cfg.m_mergePredefinedVideoTrackNames    = ui->lwMPredefinedVideoTrackNames->items();
  m_cfg.m_mergePredefinedAudioTrackNames    = ui->lwMPredefinedAudioTrackNames->items();
  m_cfg.m_mergePredefinedSubtitleTrackNames = ui->lwMPredefinedSubtitleTrackNames->items();
  m_cfg.m_mergePredefinedSplitSizes         = ui->lwMPredefinedSplitSizes->items();
  m_cfg.m_mergePredefinedSplitDurations     = ui->lwMPredefinedSplitDurations->items();

  saveLanguageShortcuts();
  saveFileColors();

  m_cfg.save();
  m_cfg.updateMaximumNumRecentlyUsedStrings();

  mtx::chapters::g_chapter_generation_name_template.override(to_utf8(m_cfg.m_chapterNameTemplate));
}

void
PreferencesDialog::rememberCurrentlySelectedPage() {
  auto pageIndex = ui->pages->currentIndex();
  auto pageTypes = m_pageIndexes.keys(pageIndex);

  if (!pageTypes.isEmpty())
    ms_previouslySelectedPage = pageTypes[0];
}

void
PreferencesDialog::enableOftendUsedLanguagesOnly() {
  ui->cbOftenUsedLanguagesOnly->setEnabled(!ui->tbOftenUsedLanguages->selectedItemValues().isEmpty());
}

void
PreferencesDialog::enableOftendUsedRegionsOnly() {
  ui->cbOftenUsedRegionsOnly->setEnabled(!ui->tbOftenUsedRegions->selectedItemValues().isEmpty());
}

void
PreferencesDialog::enableOftendUsedCharacterSetsOnly() {
  ui->cbOftenUsedCharacterSetsOnly->setEnabled(!ui->tbOftenUsedCharacterSets->selectedItemValues().isEmpty());
}

void
PreferencesDialog::enableOutputFileNameControls() {
  bool isChecked        = ui->cbMAutoSetDestinationFileName->isChecked();
  bool relativeSelected = ui->rbMAutoSetDestinationRelativeDirectory->isChecked();
  bool fixedSelected    = ui->rbMAutoSetDestinationFixedDirectory->isChecked();

  Util::enableWidgets(QList<QWidget *>{} << ui->gbMAutoSetDestinationDirectory << ui->cbMUniqueOutputFileNames << ui->cbMAutoSetDestinationOnlyForVideoFiles << ui->cbMAutoSetDestinationFromTitle
                      << ui->cbMAutoSetDestinationFromDirectory,
                      isChecked);
  Util::enableWidgets(QList<QWidget *>{} << ui->cbMAutoSetDestinationFixedDirectory << ui->pbMAutoSetDestinationBrowseFixedDirectory, isChecked && fixedSelected);
  ui->cbMAutoSetDestinationRelativeDirectory->setEnabled(isChecked && relativeSelected);
}

void
PreferencesDialog::browseFixedOutputDirectory() {
  auto dir = Util::getExistingDirectory(this, QY("Select destination directory"), ui->cbMAutoSetDestinationFixedDirectory->currentText());
  if (!dir.isEmpty())
    ui->cbMAutoSetDestinationFixedDirectory->setCurrentText(dir);
}

void
PreferencesDialog::browseMediaInfoExe() {
  auto filters = QStringList{};

#if defined(SYS_WINDOWS)
  filters << QY("Executable files") + Q(" (*.exe)");
#endif
  filters << QY("All files") + Q(" (*)");

  auto fileName = Util::getOpenFileName(this, QY("Select executable"), ui->leMMediaInfoExe->text(), filters.join(Q(";;")));
  if (!fileName.isEmpty())
    ui->leMMediaInfoExe->setText(fileName);
}

void
PreferencesDialog::editDefaultAdditionalCommandLineOptions() {
  Merge::AdditionalCommandLineOptionsDialog dlg{this, ui->leMDefaultAdditionalCommandLineOptions->text()};
  dlg.hideSaveAsDefaultCheckbox();
  if (dlg.exec())
    ui->leMDefaultAdditionalCommandLineOptions->setText(dlg.additionalOptions());
}

void
PreferencesDialog::addProgramToExecute() {
  auto programWidget = new PrefsRunProgramWidget{this, {}};
  ui->twJobsPrograms->addTab(programWidget, programWidget->config()->name());
  ui->twJobsPrograms->setCurrentIndex(ui->twJobsPrograms->count() - 1);

  ui->swJobsPrograms->setCurrentIndex(1);

  connect(programWidget, &PrefsRunProgramWidget::titleChanged, this, &PreferencesDialog::setSendersTabTitleForRunProgramWidget);
}

void
PreferencesDialog::removeProgramToExecute(int index) {
  if ((0  > index) || (ui->twJobsPrograms->count() <= index))
    return;

  auto tab = qobject_cast<QWidget *>(ui->twJobsPrograms->widget(index));
  if (!tab)
    return;

  ui->twJobsPrograms->removeTab(index);
  delete tab;

  if (!ui->twJobsPrograms->count())
    ui->swJobsPrograms->setCurrentIndex(0);
}

void
PreferencesDialog::setSendersTabTitleForRunProgramWidget() {
  auto widget = qobject_cast<PrefsRunProgramWidget *>(sender());
  auto title  = widget->config()->name();

  setTabTitleForRunProgramWidget(ui->twJobsPrograms->indexOf(widget), title);
}

void
PreferencesDialog::setTabTitleForRunProgramWidget(int tabIdx,
                                                  QString const &title) {
  if ((tabIdx < 0) || (tabIdx >= ui->twJobsPrograms->count()))
    return;

  ui->twJobsPrograms->setTabText(tabIdx, title);
}

void
PreferencesDialog::adjustPlaylistControls() {
  ui->sbMMinPlaylistDuration->setSuffix(QNY(" second", " seconds", ui->sbMMinPlaylistDuration->value()));
}

void
PreferencesDialog::adjustRemoveOldJobsControls() {
  ui->sbGuiRemoveOldJobsDays->setEnabled(ui->cbGuiRemoveOldJobs->isChecked());
  ui->sbGuiRemoveOldJobsDays->setSuffix(QNY(" day", " days", ui->sbGuiRemoveOldJobsDays->value()));
}

void
PreferencesDialog::showPage(Page page) {
  auto pageIndex      = m_pageIndexes[page];
  auto pageModelIndex = modelIndexForPage(pageIndex);

  if (!pageModelIndex.isValid())
    return;

  m_ignoreNextCurrentChange = true;

  ui->pageSelector->selectionModel()->select(pageModelIndex, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Current);
  ui->pages->setCurrentIndex(pageIndex);
}

bool
PreferencesDialog::verifyDeriveTrackLanguageSettings() {
  if (   (static_cast<Util::Settings::DeriveLanguageFromFileNamePolicy>(ui->cbMDeriveAudioTrackLanguageFromFileName   ->currentData().toInt()) == Util::Settings::DeriveLanguageFromFileNamePolicy::Never)
      && (static_cast<Util::Settings::DeriveLanguageFromFileNamePolicy>(ui->cbMDeriveVideoTrackLanguageFromFileName   ->currentData().toInt()) == Util::Settings::DeriveLanguageFromFileNamePolicy::Never)
      && (static_cast<Util::Settings::DeriveLanguageFromFileNamePolicy>(ui->cbMDeriveSubtitleTrackLanguageFromFileName->currentData().toInt()) == Util::Settings::DeriveLanguageFromFileNamePolicy::Never))
    return true;

  if (ui->leMDeriveTrackLanguageBoundaryChars->text().isEmpty()) {
    Util::MessageBox::critical(this)
      ->title(QY("Invalid settings"))
      .text(QY("The list of boundary characters for deriving the track language from file names must not be empty."))
      .exec();

    return false;
  }

  if (ui->tbMDeriveTrackLanguageRecognizedLanguages->selectedItemValues().isEmpty()) {
    showPage(Page::DeriveTrackLanguage);
    ui->tbMDeriveTrackLanguageRecognizedLanguages->setFocus();

    Util::MessageBox::critical(this)
      ->title(QY("Invalid settings"))
      .text(QY("The list of recognized track languages in file names must not be empty."))
      .exec();

    return false;
  }

  return true;
}

bool
PreferencesDialog::verifyDeriveCommentaryFlagSettings() {
  auto reText = ui->leMDeriveCommentaryFlagRE->text();
  QRegularExpression re{reText, QRegularExpression::CaseInsensitiveOption};

  if (!reText.isEmpty() && re.isValid())
    return true;

  Util::MessageBox::critical(this)
    ->title(QY("Invalid settings"))
    .text(QY("The value for deriving the 'commentary' flag from file names must be a valid regular expression."))
    .exec();

  return false;
}

bool
PreferencesDialog::verifyDeriveHearingImpairedFlagSettings() {
  auto reText = ui->leMDeriveHearingImpairedFlagRE->text();
  QRegularExpression re{reText, QRegularExpression::CaseInsensitiveOption};

  if (!reText.isEmpty() && re.isValid())
    return true;

  Util::MessageBox::critical(this)
    ->title(QY("Invalid settings"))
    .text(QY("The value for deriving the 'hearing impaired' flag from file names must be a valid regular expression."))
    .exec();

  return false;
}

bool
PreferencesDialog::verifyDeriveForcedDisplayFlagSettings() {
  auto reText = ui->leMDeriveForcedDisplayFlagSubtitlesRE->text();
  QRegularExpression re{reText, QRegularExpression::CaseInsensitiveOption};

  if (!reText.isEmpty() && re.isValid())
    return true;

  Util::MessageBox::critical(this)
    ->title(QY("Invalid settings"))
    .text(QY("The value for deriving the 'forced display' flag for subtitles from file names must be a valid regular expression."))
    .exec();

  return false;
}

bool
PreferencesDialog::verifyRunProgramConfigurations() {
  for (auto tabIdx = 0, numTabs = ui->twJobsPrograms->count(); tabIdx < numTabs; ++tabIdx) {
    auto tab   = qobject_cast<PrefsRunProgramWidget *>(ui->twJobsPrograms->widget(tabIdx));
    auto error = tab->validate();

    if (error.isEmpty())
      continue;

    showPage(Page::RunPrograms);
    ui->twJobsPrograms->setCurrentIndex(tabIdx);

    Util::MessageBox::critical(this)
      ->title(QY("Invalid settings"))
      .text(Q("<p>%1 %2</p>"
              "<p>%3</p>")
            .arg(QY("This configuration is currently invalid.").toHtmlEscaped())
            .arg(error.toHtmlEscaped())
            .arg(QY("Either fix the error or remove the configuration before closing the preferences dialog.").toHtmlEscaped()))
      .exec();

    return false;
  }

  return true;
}

void
PreferencesDialog::accept() {
  if (   verifyDeriveTrackLanguageSettings()
      && verifyDeriveCommentaryFlagSettings()
      && verifyDeriveHearingImpairedFlagSettings()
      && verifyDeriveForcedDisplayFlagSettings()
      && verifyRunProgramConfigurations()) {
    rememberCurrentlySelectedPage();
    QDialog::accept();
  }
}

void
PreferencesDialog::reject() {
  rememberCurrentlySelectedPage();
  QDialog::reject();
}

void
PreferencesDialog::setupLanguageShortcuts() {
  for (auto const &shortcut : m_cfg.m_languageShortcuts) {
    auto language = mtx::bcp47::language_c::parse(to_utf8(shortcut.m_language));
    if (!language.is_valid())
      continue;

    auto item = new QTreeWidgetItem{};

    item->setText(0, Q(language.format_long()));
    item->setText(1, shortcut.m_trackName);

    item->setData(0, Qt::UserRole, Q(language.format()));
    item->setData(1, Qt::UserRole, shortcut.m_trackName);

    item->setFlags(item->flags() & ~Qt::ItemIsDropEnabled);

    ui->twGuiLanguageShortcuts->addTopLevelItem(item);
  }

  Util::resizeViewColumnsToContents(ui->twGuiLanguageShortcuts);

  enableLanguageShortcutControls();
}

void
PreferencesDialog::saveLanguageShortcuts() {
  m_cfg.m_languageShortcuts.clear();

  for (auto row = 0, numItems = ui->twGuiLanguageShortcuts->topLevelItemCount(); row < numItems; ++row) {
    Util::Settings::LanguageShortcut shortcut;

    auto item            = ui->twGuiLanguageShortcuts->topLevelItem(row);
    shortcut.m_language  = item->data(0, Qt::UserRole).toString();
    shortcut.m_trackName = item->data(1, Qt::UserRole).toString();

    m_cfg.m_languageShortcuts << shortcut;
  }
}

void
PreferencesDialog::enableLanguageShortcutControls() {
  auto items = ui->twGuiLanguageShortcuts->selectedItems();

  ui->pbGuiEditLanguageShortcut->setEnabled(items.size() == 1);
  ui->pbGuiRemoveLanguageShortcuts->setEnabled(items.size() > 0);
}

void
PreferencesDialog::addLanguageShortcut() {
  mtx::gui::PrefsLanguageShortcutDialog dlg{this, true};

  if (!dlg.exec())
    return;

  auto language  = dlg.language();
  auto trackName = dlg.trackName();
  auto item      = new QTreeWidgetItem;

  item->setText(0, Q(language.format_long()));
  item->setText(1, trackName);

  item->setData(0, Qt::UserRole, Q(language.format()));
  item->setData(1, Qt::UserRole, trackName);

  item->setFlags(item->flags() & ~Qt::ItemIsDropEnabled);

  ui->twGuiLanguageShortcuts->addTopLevelItem(item);
}

void
PreferencesDialog::editLanguageShortcut() {
  auto items = ui->twGuiLanguageShortcuts->selectedItems();
  if (items.isEmpty())
    return;

  auto item      = items.first();
  auto language  = mtx::bcp47::language_c::parse(to_utf8(item->data(0, Qt::UserRole).toString()));
  auto trackName = item->data(1, Qt::UserRole).toString();

  mtx::gui::PrefsLanguageShortcutDialog dlg{this, false};

  if (language.is_valid())
    dlg.setLanguage(language);

  dlg.setTrackName(trackName);

  if (!dlg.exec())
    return;

  language  = dlg.language();
  trackName = dlg.trackName();

  item->setText(0, Q(language.format_long()));
  item->setText(1, trackName);

  item->setData(0, Qt::UserRole, Q(language.format()));
  item->setData(1, Qt::UserRole, trackName);
}

void
PreferencesDialog::removeLanguageShortcuts() {
  for (auto const &item : ui->twGuiLanguageShortcuts->selectedItems())
    delete item;

  enableLanguageShortcutControls();
}

}
