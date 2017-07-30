#include "common/common_pch.h"

#include <QFileDialog>
#include <QIcon>
#include <QItemSelectionModel>
#include <QModelIndex>
#include <QStandardItem>
#include <QTabWidget>
#include <QVector>

#include "common/extern_data.h"
#include "common/qt.h"
#include "common/translation.h"
#include "mkvtoolnix-gui/app.h"
#include "mkvtoolnix-gui/chapter_editor/tool.h"
#include "mkvtoolnix-gui/forms/main_window/preferences_dialog.h"
#include "mkvtoolnix-gui/main_window/preferences_dialog.h"
#include "mkvtoolnix-gui/main_window/prefs_run_program_widget.h"
#include "mkvtoolnix-gui/merge/additional_command_line_options_dialog.h"
#include "mkvtoolnix-gui/util/file_dialog.h"
#include "mkvtoolnix-gui/util/message_box.h"
#include "mkvtoolnix-gui/util/model.h"
#include "mkvtoolnix-gui/util/side_by_side_multi_select.h"
#include "mkvtoolnix-gui/util/widget.h"

namespace mtx { namespace gui {

PreferencesDialog::PreferencesDialog(QWidget *parent,
                                     Page pageToShow)
  : QDialog{parent, Qt::Dialog | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint}
  , ui{new Ui::PreferencesDialog}
  , m_cfg(Util::Settings::get())
  , m_previousUiLocale{m_cfg.m_uiLocale}
  , m_previousProbeRangePercentage{m_cfg.m_probeRangePercentage}
  , m_ignoreNextCurrentChange{}
{
  ui->setupUi(this);

  setupPageSelector(pageToShow);

  ui->tbOftenUsedXYZ->setTabPosition(m_cfg.m_tabPosition);

  Util::restoreWidgetGeometry(this);

  // GUI page
  ui->cbGuiCheckForUpdates->setChecked(m_cfg.m_checkForUpdates);
  ui->cbGuiDisableAnimations->setChecked(m_cfg.m_disableAnimations);
  ui->cbGuiShowToolSelector->setChecked(m_cfg.m_showToolSelector);
  ui->cbGuiWarnBeforeClosingModifiedTabs->setChecked(m_cfg.m_warnBeforeClosingModifiedTabs);
  ui->cbGuiWarnBeforeAbortingJobs->setChecked(m_cfg.m_warnBeforeAbortingJobs);
  ui->cbGuiWarnBeforeOverwriting->setChecked(m_cfg.m_warnBeforeOverwriting);
  ui->cbGuiShowMoveUpDownButtons->setChecked(m_cfg.m_showMoveUpDownButtons);
  setupFont();
  setupInterfaceLanguage();
  setupTabPositions();
  setupWhenToSetDefaultLanguage();

  ui->cbGuiUseDefaultJobDescription->setChecked(m_cfg.m_useDefaultJobDescription);
  ui->cbGuiShowOutputOfAllJobs->setChecked(m_cfg.m_showOutputOfAllJobs);
  ui->cbGuiSwitchToJobOutputAfterStarting->setChecked(m_cfg.m_switchToJobOutputAfterStarting);
  ui->cbGuiResetJobWarningErrorCountersOnExit->setChecked(m_cfg.m_resetJobWarningErrorCountersOnExit);
  ui->cbGuiRemoveOldJobs->setChecked(m_cfg.m_removeOldJobs);
  ui->sbGuiRemoveOldJobsDays->setValue(m_cfg.m_removeOldJobsDays);
  adjustRemoveOldJobsControls();
  setupJobRemovalPolicy();

  setupCommonLanguages();
  setupCommonCountries();
  setupCommonCharacterSets();

  // Merge page
  ui->cbMAutoSetFileTitle->setChecked(m_cfg.m_autoSetFileTitle);
  ui->cbMAutoClearFileTitle->setChecked(m_cfg.m_autoClearFileTitle);
  ui->cbMSetAudioDelayFromFileName->setChecked(m_cfg.m_setAudioDelayFromFileName);
  ui->cbMDisableCompressionForAllTrackTypes->setChecked(m_cfg.m_disableCompressionForAllTrackTypes);
  ui->cbMDisableDefaultTrackForSubtitles->setChecked(m_cfg.m_disableDefaultTrackForSubtitles);
  ui->cbMAlwaysShowOutputFileControls->setChecked(m_cfg.m_mergeAlwaysShowOutputFileControls);
  ui->cbMClearMergeSettings->setCurrentIndex(static_cast<int>(m_cfg.m_clearMergeSettings));
  ui->cbMDefaultAudioTrackLanguage->setAdditionalItems(m_cfg.m_defaultAudioTrackLanguage).setup().setCurrentByData(m_cfg.m_defaultAudioTrackLanguage);
  ui->cbMDefaultVideoTrackLanguage->setAdditionalItems(m_cfg.m_defaultVideoTrackLanguage).setup().setCurrentByData(m_cfg.m_defaultVideoTrackLanguage);
  ui->cbMDefaultSubtitleTrackLanguage->setAdditionalItems(m_cfg.m_defaultSubtitleTrackLanguage).setup().setCurrentByData(m_cfg.m_defaultSubtitleTrackLanguage);
  ui->cbMDefaultSubtitleCharset->setAdditionalItems(m_cfg.m_defaultSubtitleCharset).setup(true, QY("– No selection by default –")).setCurrentByData(m_cfg.m_defaultSubtitleCharset);
  ui->leMDefaultAdditionalCommandLineOptions->setText(m_cfg.m_defaultAdditionalMergeOptions);
  ui->cbMProbeRangePercentage->setValue(m_cfg.m_probeRangePercentage);

  setupProcessPriority();
  setupPlaylistScanningPolicy();
  setupOutputFileNamePolicy();
  setupEnableMuxingTracksByLanguage();
  setupMergeAddingAppendingFilesPolicy();
  setupTrackPropertiesLayout();

  // Chapter editor page
  ui->cbCEDropLastFromBlurayPlaylist->setChecked(m_cfg.m_dropLastChapterFromBlurayPlaylist);
  ui->cbCETextFileCharacterSet->setAdditionalItems(m_cfg.m_ceTextFileCharacterSet).setup(true, QY("Always ask the user")).setCurrentByData(m_cfg.m_ceTextFileCharacterSet);
  ui->leCENameTemplate->setText(m_cfg.m_chapterNameTemplate);
  ui->cbCEDefaultLanguage->setAdditionalItems(m_cfg.m_defaultChapterLanguage).setup().setCurrentByData(m_cfg.m_defaultChapterLanguage);
  ui->cbCEDefaultCountry->setAdditionalItems(m_cfg.m_defaultChapterCountry).setup(true, QY("– No selection by default –")).setCurrentByData(m_cfg.m_defaultChapterCountry);

  // Header editor page
  setupHeaderEditorDroppedFilesPolicy();

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
PreferencesDialog::probeRangePercentageChanged()
  const {
  return m_previousProbeRangePercentage != m_cfg.m_probeRangePercentage;
}

void
PreferencesDialog::pageSelectionChanged(QModelIndex const &current) {
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
  auto model     = new QStandardItemModel{};
  ui->pageSelector->setModel(model);

  auto addItem = [this, model, &pageIndex](Page pageType, QStandardItem *parent, QString const &text, QString const &icon = QString{}) -> QStandardItem * {
    auto item = new QStandardItem{text};

    item->setData(pageIndex);
    m_pageIndexes[pageType] = pageIndex++;

    if (!icon.isEmpty())
      item->setIcon(QIcon{Q(":/icons/16x16/%1.png").arg(icon)});

    if (parent)
      parent->appendRow(item);
    else
      model->appendRow(item);

    return item;
  };

  auto pGui      = addItem(Page::Gui,                 nullptr, QY("GUI"),                   "mkvtoolnix-gui");
                   addItem(Page::OftenUsedSelections, pGui,    QY("Often used selections"));
  auto pMerge    = addItem(Page::Merge,               nullptr, QY("Multiplexer"),           "merge");
                   addItem(Page::DefaultValues,       pMerge,  QY("Default values"));
                   addItem(Page::Output,              pMerge,  QY("Output"));
                   addItem(Page::EnablingTracks,      pMerge,  QY("Enabling tracks"));
                   addItem(Page::Playlists,           pMerge,  QY("Playlists"));
                   addItem(Page::HeaderEditor,        nullptr, QY("Header editor"),         "document-edit");
                   addItem(Page::ChapterEditor,       nullptr, QY("Chapter editor"),        "story-editor");
  auto pJobs     = addItem(Page::Jobs,                nullptr, QY("Jobs & job queue"),      "view-task");
                   addItem(Page::RunPrograms,         pJobs,   QY("Executing actions"));

  for (auto row = 0, numRows = model->rowCount(); row < numRows; ++row)
    ui->pageSelector->setExpanded(model->index(row, 0), true);

  ui->pageSelector->setMinimumSize(ui->pageSelector->minimumSizeHint());

  showPage(pageToShow);

  connect(ui->pageSelector->selectionModel(), &QItemSelectionModel::currentChanged, this, &PreferencesDialog::pageSelectionChanged);
}

void
PreferencesDialog::setupToolTips() {
  // GUI page
  Util::setToolTip(ui->cbGuiCheckForUpdates,
                   Q("%1 %2 %3")
                   .arg(QY("If enabled, the program will check online whether or not a new release of MKVToolNix is available on the home page."))
                   .arg(QY("This is done at startup and at most once within 24 hours."))
                   .arg(QY("No information is transmitted to the server.")));

  Util::setToolTip(ui->cbGuiDisableAnimations, QY("If checked, several short animations used throughout the program as visual clues for the user will be disabled."));
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

  Util::setToolTip(ui->cbGuiWarnBeforeOverwriting, QY("If enabled, the program will ask for confirmation before overwriting files and jobs."));

  Util::setToolTip(ui->cbGuiUseDefaultJobDescription, QY("If disabled, the GUI will let you enter a description for a job when adding it to the queue."));
  Util::setToolTip(ui->cbGuiShowOutputOfAllJobs,      QY("If enabled, the first tab in the \"job output\" tool will not be cleared when a new job starts."));
  Util::setToolTip(ui->cbGuiSwitchToJobOutputAfterStarting,     QY("If enabled, the GUI will automatically switch to the job output tool whenever you start a job (e.g. by pressing \"start multiplexing\")."));
  Util::setToolTip(ui->cbGuiResetJobWarningErrorCountersOnExit, QY("If enabled, the warning and error counters of all jobs and the global counters in the status bar will be reset to 0 when the program exits."));
  Util::setToolTip(ui->cbGuiRemoveOldJobs,                      QY("If enabled, the GUI will remove completed jobs older than the configured number of days no matter their status on exit."));
  Util::setToolTip(ui->sbGuiRemoveOldJobsDays,                  QY("If enabled, the GUI will remove completed jobs older than the configured number of days no matter their status on exit."));

  Util::setToolTip(ui->cbGuiRemoveJobs,
                   Q("%1 %2")
                   .arg(QY("Normally completed jobs stay in the queue even over restarts until the user clears them out manually."))
                   .arg(QY("You can opt for having them removed automatically under certain conditions.")));

  Util::setToolTip(ui->leCENameTemplate, ChapterEditor::Tool::chapterNameTemplateToolTip());
  Util::setToolTip(ui->cbCEDefaultLanguage, QY("This is the language that newly added chapter names get assigned automatically."));
  Util::setToolTip(ui->cbCEDefaultCountry, QY("This is the country that newly added chapter names get assigned automatically."));
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

  Util::setToolTip(ui->cbMSetAudioDelayFromFileName,
                   Q("%1 %2")
                   .arg(QY("When a file is added its name is scanned."))
                   .arg(QY("If it contains the word 'DELAY' followed by a number, this number is automatically put into the 'delay' input field for any audio track found in the file.")));

  Util::setToolTip(ui->cbMDisableDefaultTrackForSubtitles, QY("If enabled, all subtitle tracks will have their \"default track\" flag set to \"no\" when they're added."));

  Util::setToolTip(ui->cbMDisableCompressionForAllTrackTypes,
                   Q("%1 %2")
                   .arg(QY("Normally mkvmerge will apply additional lossless compression for subtitle tracks for certain codecs."))
                   .arg(QY("Checking this option causes the GUI to set that compression to \"none\" by default for all track types when adding files.")));

  Util::setToolTip(ui->cbMProbeRangePercentage,
                   Q("%1 %2 %3")
                   .arg(QY("File types such as MPEG program and transport streams (.vob, .m2ts) require parsing a certain amount of data in order to detect all tracks contained in the file."))
                   .arg(Q(YF("This amount is 0.3% of the source file's size or 10 MB, whichever is higher.")))
                   .arg(QY("If tracks are known to be present but not found, the percentage to probe can be changed here.")));

  Util::setToolTip(ui->cbMAlwaysShowOutputFileControls,
                   Q("%1 %2")
                   .arg(QY("If enabled, the destination file name controls will always be visible no matter which tab is currently shown."))
                   .arg(QY("Otherwise they're shown on the 'output' tab.")));

  auto controls = QWidgetList{} << ui->rbMTrackPropertiesLayoutHorizontalScrollArea << ui->rbMTrackPropertiesLayoutHorizontalTwoColumns << ui->rbMTrackPropertiesLayoutVerticalTabWidget;
  for (auto const &control : controls)
    Util::setToolTip(control,
                     Q("<p>%1 %2</p><p>%3 %4</p>")
                     .arg(QYH("The track properties on the \"input\" tab can be laid out in three different way in order to serve different workflows."))
                     .arg(QYH("In the most compact layout the track properties are located to the right of the files and tracks lists in a scrollable single column."))
                     .arg(QYH("The other two layouts available are: in two fixed columns to the right or in a tab widget below the files and tracks lists."))
                     .arg(QYH("The horizontal layout with two fixed columns results in a wider window while the vertical tab widget layout results in a higher window.")));

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

  Util::setToolTip(ui->cbMDefaultAudioTrackLanguage,
                   Q("<p>%1 %2</p><p>%3 %4</p>")
                   .arg(QYH("Certain file formats have a 'language' property for their tracks."))
                   .arg(QYH("When the user adds such a file the track's language input is set to the language property from the source file."))
                   .arg(QYH("The language selected here is used for audio tracks for which their source file contains no such property."))
                   .arg(QYH("Depending on the setting below this language can also be used if the language in the source file is 'undetermined' ('und').")));
  Util::setToolTip(ui->cbMDefaultVideoTrackLanguage,
                   Q("<p>%1 %2</p><p>%3 %4</p>")
                   .arg(QYH("Certain file formats have a 'language' property for their tracks."))
                   .arg(QYH("When the user adds such a file the track's language input is set to the language property from the source file."))
                   .arg(QYH("The language selected here is used for video tracks for which their source file contains no such property."))
                   .arg(QYH("Depending on the setting below this language can also be used if the language in the source file is 'undetermined' ('und').")));
  Util::setToolTip(ui->cbMDefaultSubtitleTrackLanguage,
                   Q("<p>%1 %2</p><p>%3 %4</p>")
                   .arg(QYH("Certain file formats have a 'language' property for their tracks."))
                   .arg(QYH("When the user adds such a file the track's language input is set to the language property from the source file."))
                   .arg(QYH("The language selected here is used for subtitle tracks for which their source file contains no such property."))
                   .arg(QYH("Depending on the setting below this language can also be used if the language in the source file is 'undetermined' ('und').")));

  Util::setToolTip(ui->cbMDefaultSubtitleCharset, QY("If a character set is selected here, the program will automatically set the character set input to this value for newly added text subtitle tracks."));

  Util::setToolTip(ui->leMDefaultAdditionalCommandLineOptions, QY("The options entered here are set for all new multiplex jobs by default."));

  Util::setToolTip(ui->cbMScanPlaylistsPolicy,
                   Q("<p>%1 %2</p><p>%3</p>")
                   .arg(QYH("Whenever the user adds a playlist the program can automatically scan the directory for other playlists and present the user with a detailed list of the playlists found."))
                   .arg(QYH("The user can then select which playlist to actually add."))
                   .arg(QYH("This is useful for situations like Blu-ray discs on which a multitude of playlists exists in the same directory and where it is not obvious which feature (e.g. main movie, extras etc.) "
                            "a playlist belongs to.")));

  Util::setToolTip(ui->sbMMinPlaylistDuration, QY("Only playlists whose duration are at least this long are considered and offered to the user for selection."));

  Util::setToolTip(ui->cbMAutoSetOutputFileName,
                   Q("%1 %2")
                   .arg(QY("If this option is enabled and if there is currently no destination file name set, the program will set one for you when you add a source file."))
                   .arg(QY("The generated destination file name has the same base name as the source file name but with an extension based on the track types present in the file.")));

  Util::setToolTip(ui->cbMAutoDestinationOnlyForVideoFiles,
                   Q("%1 %2")
                   .arg(QY("If this option is enabled, only source files containing video tracks will be used for setting the destination file name."))
                   .arg(QY("Other files are ignored when they're added.")));

  Util::setToolTip(ui->cbMUniqueOutputFileNames,
                   Q("%1 %2")
                   .arg(QY("If checked, the program makes sure the suggested destination file name is unique by adding a number (e.g. ' (1)') to the end of the file name."))
                   .arg(QY("This is done only if there is already a file whose name matches the unmodified destination file name.")));
  Util::setToolTip(ui->cbMAutoClearOutputFileName, QY("If this option is enabled, the GUI will always clear the \"destination file name\" input box whenever the last source file is removed."));

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

  // Often used XYZ page
  ui->tbOftenUsedLanguages->setToolTips(QY("The languages in the 'selected' list on the right will be shown at the top of all the language drop-down boxes in the program."),
                                        QY("The languages in the 'selected' list on the right will be shown at the top of all the language drop-down boxes in the program."));
  Util::setToolTip(ui->cbOftenUsedLanguagesOnly,
                   Q("%1 %2")
                   .arg(QYH("If checked, only the list of often used entries will be included in the selections in the program."))
                   .arg(QYH("Otherwise the often used entries will be included first and the full list of all entries afterwards.")));

  ui->tbOftenUsedCountries->setToolTips(QY("The countries in the 'selected' list on the right will be shown at the top of all the country drop-down boxes in the program."),
                                        QY("The countries in the 'selected' list on the right will be shown at the top of all the country drop-down boxes in the program."));
  Util::setToolTip(ui->cbOftenUsedCountriesOnly,
                   Q("%1 %2")
                   .arg(QYH("If checked, only the list of often used entries will be included in the selections in the program."))
                   .arg(QYH("Otherwise the often used entries will be included first and the full list of all entries afterwards.")));

  ui->tbOftenUsedCharacterSets->setToolTips(QY("The character sets in the 'selected' list on the right will be shown at the top of all the character set drop-down boxes in the program."),
                                            QY("The character sets in the 'selected' list on the right will be shown at the top of all the character set drop-down boxes in the program."));
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
PreferencesDialog::setupConnections() {
  connect(ui->pbMEditDefaultAdditionalCommandLineOptions, &QPushButton::clicked,                                         this,                                 &PreferencesDialog::editDefaultAdditionalCommandLineOptions);

  connect(ui->cbMAutoSetOutputFileName,                   &QCheckBox::toggled,                                           this,                                 &PreferencesDialog::enableOutputFileNameControls);
  connect(ui->rbMAutoSetSameDirectory,                    &QRadioButton::toggled,                                        this,                                 &PreferencesDialog::enableOutputFileNameControls);
  connect(ui->rbMAutoSetRelativeDirectory,                &QRadioButton::toggled,                                        this,                                 &PreferencesDialog::enableOutputFileNameControls);
  connect(ui->rbMAutoSetPreviousDirectory,                &QRadioButton::toggled,                                        this,                                 &PreferencesDialog::enableOutputFileNameControls);
  connect(ui->rbMAutoSetFixedDirectory,                   &QRadioButton::toggled,                                        this,                                 &PreferencesDialog::enableOutputFileNameControls);
  connect(ui->pbMBrowseAutoSetFixedDirectory,             &QPushButton::clicked,                                         this,                                 &PreferencesDialog::browseFixedOutputDirectory);

  connect(ui->cbMEnableMuxingTracksByLanguage,            &QCheckBox::toggled,                                           ui->lMEnableMuxingAllTracksOfType,    &QLabel::setEnabled);
  connect(ui->cbMEnableMuxingTracksByLanguage,            &QCheckBox::toggled,                                           ui->cbMEnableMuxingAllVideoTracks,    &QLabel::setEnabled);
  connect(ui->cbMEnableMuxingTracksByLanguage,            &QCheckBox::toggled,                                           ui->cbMEnableMuxingAllAudioTracks,    &QLabel::setEnabled);
  connect(ui->cbMEnableMuxingTracksByLanguage,            &QCheckBox::toggled,                                           ui->cbMEnableMuxingAllSubtitleTracks, &QLabel::setEnabled);
  connect(ui->cbMEnableMuxingTracksByLanguage,            &QCheckBox::toggled,                                           ui->tbMEnableMuxingTracksByLanguage,  &QLabel::setEnabled);

  connect(ui->cbGuiRemoveJobs,                            &QCheckBox::toggled,                                           ui->cbGuiJobRemovalPolicy,            &QComboBox::setEnabled);
  connect(ui->cbGuiRemoveOldJobs,                         &QCheckBox::toggled,                                           this,                                 &PreferencesDialog::adjustRemoveOldJobsControls);
  connect(ui->sbGuiRemoveOldJobsDays,                     static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this,                                 &PreferencesDialog::adjustRemoveOldJobsControls);

  connect(ui->sbMMinPlaylistDuration,                     static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this,                                 &PreferencesDialog::adjustPlaylistControls);

  connect(ui->buttons,                                    &QDialogButtonBox::accepted,                                   this,                                 &PreferencesDialog::accept);
  connect(ui->buttons,                                    &QDialogButtonBox::rejected,                                   this,                                 &PreferencesDialog::reject);

  connect(ui->tbOftenUsedLanguages,                       &Util::SideBySideMultiSelect::listsChanged,                    this,                                 &PreferencesDialog::enableOftendUsedLanguagesOnly);
  connect(ui->tbOftenUsedCountries,                       &Util::SideBySideMultiSelect::listsChanged,                    this,                                 &PreferencesDialog::enableOftendUsedCountriesOnly);
  connect(ui->tbOftenUsedCharacterSets,                   &Util::SideBySideMultiSelect::listsChanged,                    this,                                 &PreferencesDialog::enableOftendUsedCharacterSetsOnly);
}

void
PreferencesDialog::setupInterfaceLanguage() {
#if defined(HAVE_LIBINTL_H)
  using TranslationSorter = std::pair<QString, QString>;
  auto translations       = std::vector<TranslationSorter>{};

  for (auto const &translation : translation_c::ms_available_translations)
    translations.emplace_back(Q("%1 (%2)").arg(Q(translation.m_translated_name)).arg(Q(translation.m_english_name)), Q(translation.get_locale()));

  brng::sort(translations);

  for (auto const &translation : translations)
    ui->cbGuiInterfaceLanguage->addItem(translation.first, translation.second);

  Util::setComboBoxTextByData(ui->cbGuiInterfaceLanguage, m_cfg.m_uiLocale);
  Util::fixComboBoxViewWidth(*ui->cbGuiInterfaceLanguage);
#endif  // HAVE_LIBINTL_H
}

void
PreferencesDialog::setupJobRemovalPolicy() {
  auto doRemove = Util::Settings::JobRemovalPolicy::Never != m_cfg.m_jobRemovalPolicy;
  auto idx      = std::max(static_cast<int>(m_cfg.m_jobRemovalPolicy), 1) - 1;

  ui->cbGuiRemoveJobs->setChecked(doRemove);
  ui->cbGuiJobRemovalPolicy->setEnabled(doRemove);
  ui->cbGuiJobRemovalPolicy->setCurrentIndex(idx);
}

void
PreferencesDialog::setupCommonLanguages() {
  auto &allLanguages = App::iso639Languages();

  ui->tbOftenUsedLanguages->setItems(QList<Util::SideBySideMultiSelect::Item>::fromVector(QVector<Util::SideBySideMultiSelect::Item>::fromStdVector(allLanguages)), m_cfg.m_oftenUsedLanguages);
  ui->cbOftenUsedLanguagesOnly->setChecked(m_cfg.m_oftenUsedLanguagesOnly && !m_cfg.m_oftenUsedLanguages.isEmpty());
  enableOftendUsedLanguagesOnly();
}

void
PreferencesDialog::setupCommonCountries() {
  auto &allCountries = App::topLevelDomainCountryCodes();

  ui->tbOftenUsedCountries->setItems(QList<Util::SideBySideMultiSelect::Item>::fromVector(QVector<Util::SideBySideMultiSelect::Item>::fromStdVector(allCountries)), m_cfg.m_oftenUsedCountries);
  ui->cbOftenUsedCountriesOnly->setChecked(m_cfg.m_oftenUsedCountriesOnly && !m_cfg.m_oftenUsedCountries.isEmpty());
  enableOftendUsedCountriesOnly();
}

void
PreferencesDialog::setupCommonCharacterSets() {
  ui->tbOftenUsedCharacterSets->setItems(QList<QString>::fromVector(QVector<QString>::fromStdVector(App::characterSets())), m_cfg.m_oftenUsedCharacterSets);
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

  adjustPlaylistControls();

  Util::fixComboBoxViewWidth(*ui->cbMScanPlaylistsPolicy);
}

void
PreferencesDialog::setupOutputFileNamePolicy() {
  auto isChecked = Util::Settings::DontSetOutputFileName != m_cfg.m_outputFileNamePolicy;
  auto rbToCheck = Util::Settings::ToPreviousDirectory        == m_cfg.m_outputFileNamePolicy ? ui->rbMAutoSetPreviousDirectory
                 : Util::Settings::ToFixedDirectory           == m_cfg.m_outputFileNamePolicy ? ui->rbMAutoSetFixedDirectory
                 : Util::Settings::ToRelativeOfFirstInputFile == m_cfg.m_outputFileNamePolicy ? ui->rbMAutoSetRelativeDirectory
                 :                                                                              ui->rbMAutoSetSameDirectory;

  ui->cbMAutoSetOutputFileName->setChecked(isChecked);
  ui->cbMAutoDestinationOnlyForVideoFiles->setChecked(m_cfg.m_autoDestinationOnlyForVideoFiles);
  rbToCheck->setChecked(true);
  ui->leMAutoSetRelativeDirectory->setText(QDir::toNativeSeparators(m_cfg.m_relativeOutputDir.path()));
  ui->leMAutoSetFixedDirectory->setText(QDir::toNativeSeparators(m_cfg.m_fixedOutputDir.path()));
  ui->cbMUniqueOutputFileNames->setChecked(m_cfg.m_uniqueOutputFileNames);
  ui->cbMAutoClearOutputFileName->setChecked(m_cfg.m_autoClearOutputFileName);

  enableOutputFileNameControls();
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

  auto &allLanguages = App::iso639Languages();
  ui->tbMEnableMuxingTracksByLanguage->setItems(QList<Util::SideBySideMultiSelect::Item>::fromVector(QVector<Util::SideBySideMultiSelect::Item>::fromStdVector(allLanguages)), m_cfg.m_enableMuxingTracksByTheseLanguages);
}

void
PreferencesDialog::setupMergeAddingAppendingFilesPolicy() {
  ui->cbMAddingAppendingFilesPolicy->addItem(QY("Always ask the user"),                                           static_cast<int>(Util::Settings::MergeAddingAppendingFilesPolicy::Ask));
  ui->cbMAddingAppendingFilesPolicy->addItem(QY("Add all files to the current multiplex settings"),               static_cast<int>(Util::Settings::MergeAddingAppendingFilesPolicy::Add));
  ui->cbMAddingAppendingFilesPolicy->addItem(QY("Create one new multiplex settings tab and add all files there"), static_cast<int>(Util::Settings::MergeAddingAppendingFilesPolicy::AddToNew));
  ui->cbMAddingAppendingFilesPolicy->addItem(QY("Create one new multiplex settings tab for each file"),           static_cast<int>(Util::Settings::MergeAddingAppendingFilesPolicy::AddEachToNew));

  Util::setComboBoxIndexIf(ui->cbMAddingAppendingFilesPolicy, [this](QString const &, QVariant const &data) {
    return data.isValid() && (static_cast<Util::Settings::MergeAddingAppendingFilesPolicy>(data.toInt()) == m_cfg.m_mergeAddingAppendingFilesPolicy);
  });

  Util::fixComboBoxViewWidth(*ui->cbMAddingAppendingFilesPolicy);
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
PreferencesDialog::setupWhenToSetDefaultLanguage() {
  ui->cbMWhenToSetDefaultLanguage->clear();
  ui->cbMWhenToSetDefaultLanguage->addItem(QY("Only if the source doesn't contain a language"),  static_cast<int>(Util::Settings::SetDefaultLanguagePolicy::OnlyIfAbsent));
  ui->cbMWhenToSetDefaultLanguage->addItem(QY("Also if the language is 'undetermined' ('und')"), static_cast<int>(Util::Settings::SetDefaultLanguagePolicy::IfAbsentOrUndetermined));

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
PreferencesDialog::setupFont() {
  auto font = App::font();
  ui->fcbGuiFontFamily->setCurrentFont(font);
  ui->sbGuiFontPointSize->setValue(font.pointSize());
}

void
PreferencesDialog::save() {
  // GUI page:
  m_cfg.m_uiLocale                           = ui->cbGuiInterfaceLanguage->currentData().toString();
  m_cfg.m_tabPosition                        = static_cast<QTabWidget::TabPosition>(ui->cbGuiTabPositions->currentData().toInt());
  m_cfg.m_uiFontFamily                       = ui->fcbGuiFontFamily->currentFont().family();
  m_cfg.m_uiFontPointSize                    = ui->sbGuiFontPointSize->value();
  m_cfg.m_checkForUpdates                    = ui->cbGuiCheckForUpdates->isChecked();
  m_cfg.m_disableAnimations                  = ui->cbGuiDisableAnimations->isChecked();
  m_cfg.m_showToolSelector                   = ui->cbGuiShowToolSelector->isChecked();
  m_cfg.m_warnBeforeClosingModifiedTabs      = ui->cbGuiWarnBeforeClosingModifiedTabs->isChecked();
  m_cfg.m_warnBeforeAbortingJobs             = ui->cbGuiWarnBeforeAbortingJobs->isChecked();
  m_cfg.m_warnBeforeOverwriting              = ui->cbGuiWarnBeforeOverwriting->isChecked();
  m_cfg.m_showMoveUpDownButtons              = ui->cbGuiShowMoveUpDownButtons->isChecked();
  m_cfg.m_useDefaultJobDescription           = ui->cbGuiUseDefaultJobDescription->isChecked();
  m_cfg.m_showOutputOfAllJobs                = ui->cbGuiShowOutputOfAllJobs->isChecked();
  m_cfg.m_switchToJobOutputAfterStarting     = ui->cbGuiSwitchToJobOutputAfterStarting->isChecked();
  m_cfg.m_resetJobWarningErrorCountersOnExit = ui->cbGuiResetJobWarningErrorCountersOnExit->isChecked();
  auto idx                                   = !ui->cbGuiRemoveJobs->isChecked() ? 0 : ui->cbGuiJobRemovalPolicy->currentIndex() + 1;
  m_cfg.m_jobRemovalPolicy                   = static_cast<Util::Settings::JobRemovalPolicy>(idx);
  m_cfg.m_removeOldJobs                      = ui->cbGuiRemoveOldJobs->isChecked();
  m_cfg.m_removeOldJobsDays                  = ui->sbGuiRemoveOldJobsDays->value();

  m_cfg.m_chapterNameTemplate                = ui->leCENameTemplate->text();
  m_cfg.m_ceTextFileCharacterSet             = ui->cbCETextFileCharacterSet->currentData().toString();
  m_cfg.m_defaultChapterLanguage             = ui->cbCEDefaultLanguage->currentData().toString();
  m_cfg.m_defaultChapterCountry              = ui->cbCEDefaultCountry->currentData().toString();
  m_cfg.m_dropLastChapterFromBlurayPlaylist  = ui->cbCEDropLastFromBlurayPlaylist->isChecked();

  // Merge page:
  m_cfg.m_autoSetFileTitle                   = ui->cbMAutoSetFileTitle->isChecked();
  m_cfg.m_autoClearFileTitle                 = ui->cbMAutoClearFileTitle->isChecked();
  m_cfg.m_setAudioDelayFromFileName          = ui->cbMSetAudioDelayFromFileName->isChecked();
  m_cfg.m_disableCompressionForAllTrackTypes = ui->cbMDisableCompressionForAllTrackTypes->isChecked();
  m_cfg.m_disableDefaultTrackForSubtitles    = ui->cbMDisableDefaultTrackForSubtitles->isChecked();
  m_cfg.m_mergeAddingAppendingFilesPolicy    = static_cast<Util::Settings::MergeAddingAppendingFilesPolicy>(ui->cbMAddingAppendingFilesPolicy->currentData().toInt());
  m_cfg.m_mergeAlwaysShowOutputFileControls  = ui->cbMAlwaysShowOutputFileControls->isChecked();
  m_cfg.m_mergeTrackPropertiesLayout         = ui->rbMTrackPropertiesLayoutHorizontalScrollArea->isChecked() ? Util::Settings::TrackPropertiesLayout::HorizontalScrollArea
                                             : ui->rbMTrackPropertiesLayoutHorizontalTwoColumns->isChecked() ? Util::Settings::TrackPropertiesLayout::HorizontalTwoColumns
                                             :                                                                 Util::Settings::TrackPropertiesLayout::VerticalTabWidget;
  m_cfg.m_clearMergeSettings                 = static_cast<Util::Settings::ClearMergeSettingsAction>(ui->cbMClearMergeSettings->currentIndex());
  m_cfg.m_defaultAudioTrackLanguage          = ui->cbMDefaultAudioTrackLanguage->currentData().toString();
  m_cfg.m_defaultVideoTrackLanguage          = ui->cbMDefaultVideoTrackLanguage->currentData().toString();
  m_cfg.m_defaultSubtitleTrackLanguage       = ui->cbMDefaultSubtitleTrackLanguage->currentData().toString();
  m_cfg.m_whenToSetDefaultLanguage           = static_cast<Util::Settings::SetDefaultLanguagePolicy>(ui->cbMWhenToSetDefaultLanguage->currentData().toInt());
  m_cfg.m_defaultSubtitleCharset             = ui->cbMDefaultSubtitleCharset->currentData().toString();
  m_cfg.m_priority                           = static_cast<Util::Settings::ProcessPriority>(ui->cbMProcessPriority->currentData().toInt());
  m_cfg.m_defaultAdditionalMergeOptions      = ui->leMDefaultAdditionalCommandLineOptions->text();
  m_cfg.m_probeRangePercentage               = ui->cbMProbeRangePercentage->value();

  m_cfg.m_scanForPlaylistsPolicy             = static_cast<Util::Settings::ScanForPlaylistsPolicy>(ui->cbMScanPlaylistsPolicy->currentIndex());
  m_cfg.m_minimumPlaylistDuration            = ui->sbMMinPlaylistDuration->value();

  m_cfg.m_outputFileNamePolicy               = !ui->cbMAutoSetOutputFileName->isChecked()   ? Util::Settings::DontSetOutputFileName
                                             : ui->rbMAutoSetRelativeDirectory->isChecked() ? Util::Settings::ToRelativeOfFirstInputFile
                                             : ui->rbMAutoSetFixedDirectory->isChecked()    ? Util::Settings::ToFixedDirectory
                                             : ui->rbMAutoSetPreviousDirectory->isChecked() ? Util::Settings::ToPreviousDirectory
                                             :                                                Util::Settings::ToSameAsFirstInputFile;
  m_cfg.m_autoDestinationOnlyForVideoFiles   = ui->cbMAutoDestinationOnlyForVideoFiles->isChecked();
  m_cfg.m_relativeOutputDir                  = ui->leMAutoSetRelativeDirectory->text();
  m_cfg.m_fixedOutputDir                     = ui->leMAutoSetFixedDirectory->text();
  m_cfg.m_uniqueOutputFileNames              = ui->cbMUniqueOutputFileNames->isChecked();
  m_cfg.m_autoClearOutputFileName            = ui->cbMAutoClearOutputFileName->isChecked();

  m_cfg.m_enableMuxingTracksByLanguage       = ui->cbMEnableMuxingTracksByLanguage->isChecked();
  m_cfg.m_enableMuxingAllVideoTracks         = ui->cbMEnableMuxingAllVideoTracks->isChecked();
  m_cfg.m_enableMuxingAllAudioTracks         = ui->cbMEnableMuxingAllAudioTracks->isChecked();
  m_cfg.m_enableMuxingAllSubtitleTracks      = ui->cbMEnableMuxingAllSubtitleTracks->isChecked();
  m_cfg.m_enableMuxingTracksByTheseLanguages = ui->tbMEnableMuxingTracksByLanguage->selectedItemValues();

  // Often used selections page:
  m_cfg.m_oftenUsedLanguages                 = ui->tbOftenUsedLanguages->selectedItemValues();
  m_cfg.m_oftenUsedCountries                 = ui->tbOftenUsedCountries->selectedItemValues();
  m_cfg.m_oftenUsedCharacterSets             = ui->tbOftenUsedCharacterSets->selectedItemValues();

  m_cfg.m_oftenUsedLanguagesOnly             = ui->cbOftenUsedLanguagesOnly->isChecked() && !m_cfg.m_oftenUsedLanguages.isEmpty();
  m_cfg.m_oftenUsedCountriesOnly             = ui->cbOftenUsedCountriesOnly->isChecked() && !m_cfg.m_oftenUsedCountries.isEmpty();
  m_cfg.m_oftenUsedCharacterSetsOnly         = ui->cbOftenUsedCharacterSetsOnly->isChecked() && !m_cfg.m_oftenUsedCharacterSets.isEmpty();

  // Header editor page
  m_cfg.m_headerEditorDroppedFilesPolicy     = static_cast<Util::Settings::HeaderEditorDroppedFilesPolicy>(ui->cbHEDroppedFilesPolicy->currentData().toInt());

  // Run programs page:
  m_cfg.m_runProgramConfigurations.clear();
  for (auto idx = 0, numTabs = ui->twJobsPrograms->count(); idx < numTabs; ++idx) {
    auto widget = static_cast<PrefsRunProgramWidget *>(ui->twJobsPrograms->widget(idx));
    auto cfg    = widget->config();

    if (!cfg->m_active || cfg->isValid())
      m_cfg.m_runProgramConfigurations << cfg;
  }

  m_cfg.save();
}

void
PreferencesDialog::enableOftendUsedLanguagesOnly() {
  ui->cbOftenUsedLanguagesOnly->setEnabled(!ui->tbOftenUsedLanguages->selectedItemValues().isEmpty());
}

void
PreferencesDialog::enableOftendUsedCountriesOnly() {
  ui->cbOftenUsedCountriesOnly->setEnabled(!ui->tbOftenUsedCountries->selectedItemValues().isEmpty());
}

void
PreferencesDialog::enableOftendUsedCharacterSetsOnly() {
  ui->cbOftenUsedCharacterSetsOnly->setEnabled(!ui->tbOftenUsedCharacterSets->selectedItemValues().isEmpty());
}

void
PreferencesDialog::enableOutputFileNameControls() {
  bool isChecked        = ui->cbMAutoSetOutputFileName->isChecked();
  bool relativeSelected = ui->rbMAutoSetRelativeDirectory->isChecked();
  bool fixedSelected    = ui->rbMAutoSetFixedDirectory->isChecked();

  Util::enableWidgets(QList<QWidget *>{} << ui->gbDestinationDirectory   << ui->cbMUniqueOutputFileNames << ui->cbMAutoDestinationOnlyForVideoFiles, isChecked);
  Util::enableWidgets(QList<QWidget *>{} << ui->leMAutoSetFixedDirectory << ui->pbMBrowseAutoSetFixedDirectory, isChecked && fixedSelected);
  ui->leMAutoSetRelativeDirectory->setEnabled(isChecked && relativeSelected);
}

void
PreferencesDialog::browseFixedOutputDirectory() {
  auto dir = Util::getExistingDirectory(this, QY("Select destination directory"), ui->leMAutoSetFixedDirectory->text());
  if (!dir.isEmpty())
    ui->leMAutoSetFixedDirectory->setText(dir);
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
  auto selection            = QItemSelection{pageModelIndex, pageModelIndex};

  ui->pageSelector->selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Current);
  ui->pages->setCurrentIndex(pageIndex);
}

void
PreferencesDialog::accept() {
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

    return;
  }

  QDialog::accept();
}

}}
