#include "common/common_pch.h"

#include <QFileDialog>
#include <QVector>

#include "common/extern_data.h"
#include "common/qt.h"
#include "common/translation.h"
#include "mkvtoolnix-gui/app.h"
#include "mkvtoolnix-gui/forms/main_window/preferences_dialog.h"
#include "mkvtoolnix-gui/main_window/preferences_dialog.h"
#include "mkvtoolnix-gui/merge/additional_command_line_options_dialog.h"
#include "mkvtoolnix-gui/util/util.h"

namespace mtx { namespace gui {

PreferencesDialog::PreferencesDialog(QWidget *parent)
  : QDialog{parent, Qt::Dialog | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint}
  , ui{new Ui::PreferencesDialog}
  , m_cfg(Util::Settings::get())
  , m_previousUiLocale{m_cfg.m_uiLocale}
{
  ui->setupUi(this);

  ui->tabs->setTabPosition(m_cfg.m_tabPosition);
  ui->tbOftenUsedXYZ->setTabPosition(m_cfg.m_tabPosition);

  Util::restoreWidgetGeometry(this);
  Util::fixScrollAreaBackground(ui->mergeScrollArea);

  // GUI page
  ui->cbGuiDisableAnimations->setChecked(m_cfg.m_disableAnimations);
  ui->cbGuiShowToolSelector->setChecked(m_cfg.m_showToolSelector);
  ui->cbGuiWarnBeforeClosingModifiedTabs->setChecked(m_cfg.m_warnBeforeClosingModifiedTabs);
  ui->cbGuiWarnBeforeAbortingJobs->setChecked(m_cfg.m_warnBeforeAbortingJobs);
  ui->cbGuiShowMoveUpDownButtons->setChecked(m_cfg.m_showMoveUpDownButtons);
  setupOnlineCheck();
  setupInterfaceLanguage();
  setupTabPositions();

  ui->cbGuiUseDefaultJobDescription->setChecked(m_cfg.m_useDefaultJobDescription);
  ui->cbGuiShowOutputOfAllJobs->setChecked(m_cfg.m_showOutputOfAllJobs);
  ui->cbGuiSwitchToJobOutputAfterStarting->setChecked(m_cfg.m_switchToJobOutputAfterStarting);
  setupJobRemovalPolicy();

  setupCommonLanguages();
  setupCommonCountries();
  setupCommonCharacterSets();

  // Merge page
  ui->cbMAutoSetFileTitle->setChecked(m_cfg.m_autoSetFileTitle);
  ui->cbMSetAudioDelayFromFileName->setChecked(m_cfg.m_setAudioDelayFromFileName);
  ui->cbMDisableCompressionForAllTrackTypes->setChecked(m_cfg.m_disableCompressionForAllTrackTypes);
  ui->cbMDisableDefaultTrackForSubtitles->setChecked(m_cfg.m_disableDefaultTrackForSubtitles);
  ui->cbMAlwaysAddDroppedFiles->setChecked(m_cfg.m_mergeAlwaysAddDroppedFiles);
  ui->cbMAlwaysShowOutputFileControls->setChecked(m_cfg.m_mergeAlwaysShowOutputFileControls);
  ui->cbMClearMergeSettings->setCurrentIndex(static_cast<int>(m_cfg.m_clearMergeSettings));
  ui->cbMDefaultTrackLanguage->setup().setCurrentByData(m_cfg.m_defaultTrackLanguage);
  ui->cbMDefaultSubtitleCharset->setup(true, QY("– no selection by default –")).setCurrentByData(m_cfg.m_defaultSubtitleCharset);
  ui->leMDefaultAdditionalCommandLineOptions->setText(m_cfg.m_defaultAdditionalMergeOptions);

  setupProcessPriority();
  setupPlaylistScanningPolicy();
  setupOutputFileNamePolicy();
  setupEnableMuxingTracksByLanguage();

  // Chapter editor page
  ui->leCENameTemplate->setText(m_cfg.m_chapterNameTemplate);
  ui->cbCEDropLastFromBlurayPlaylist->setChecked(m_cfg.m_dropLastChapterFromBlurayPlaylist);
  ui->cbCEDefaultLanguage->setup().setCurrentByData(m_cfg.m_defaultChapterLanguage);
  ui->cbCEDefaultCountry->setup(true, QY("– no selection by default –")).setCurrentByData(m_cfg.m_defaultChapterCountry);

  setupToolTips();
  setupConnections();
}

PreferencesDialog::~PreferencesDialog() {
  Util::saveWidgetGeometry(this);
}

bool
PreferencesDialog::uiLocaleChanged()
  const {
  return m_previousUiLocale != m_cfg.m_uiLocale;
}

void
PreferencesDialog::setupToolTips() {
  // GUI page
  Util::setToolTip(ui->cbGuiCheckForUpdates,
                   Q("%1 %2 %3")
                   .arg(QY("If enabled the program will check online whether or not a new release of MKVToolNix is available on the home page."))
                   .arg(QY("This is done at startup and at most once within 24 hours."))
                   .arg(QY("No information is transmitted to the server.")));

  Util::setToolTip(ui->cbGuiDisableAnimations, QY("If checked several short animations used throughout the program as visual clues for the user will be disabled."));
  Util::setToolTip(ui->cbGuiWarnBeforeClosingModifiedTabs,
                   Q("%1 %2")
                   .arg(QY("If checked the program will ask for confirmation before closing or reloading tabs that have been modified."))
                   .arg(QY("This is also done when quitting the application.")));
  Util::setToolTip(ui->cbGuiWarnBeforeAbortingJobs,
                   Q("%1 %2")
                   .arg(QY("If checked the program will ask for confirmation before aborting a running job."))
                   .arg(QY("This happens when clicking the »abort« button in a »job output« tab and when quitting the application.")));

  Util::setToolTip(ui->cbGuiShowMoveUpDownButtons,
                   Q("%1 %2")
                   .arg(QY("Normally selected entries in list view can be moved around via drag & drop and with keyboard shortcuts (Ctrl+Up, Ctrl+Down)."))
                   .arg(QY("If checked additional buttons for moving selected entries up and down will be shown next to several list views.")));

  Util::setToolTip(ui->cbGuiWarnBeforeOverwriting, QY("If enabled the program will ask for confirmation before overwriting files and jobs."));
  ui->cbGuiWarnBeforeOverwriting->setEnabled(false);

  Util::setToolTip(ui->cbGuiUseDefaultJobDescription, QY("If disabled the GUI will let you enter a description for a job when adding it to the queue."));
  Util::setToolTip(ui->cbGuiShowOutputOfAllJobs,      QY("If enabled the first tab in the »job output« tool will not be cleared when a new job starts."));
  Util::setToolTip(ui->cbGuiSwitchToJobOutputAfterStarting, QY("If enabled the GUI will automatically switch to the job output tool whenever you start a job (e.g. by pressing »start muxing«)."));

  Util::setToolTip(ui->cbGuiRemoveJobs,
                   Q("%1 %2")
                   .arg(QY("Normally completed jobs stay in the queue even over restarts until the user clears them out manually."))
                   .arg(QY("You can opt for having them removed automatically under certain conditions.")));

  Util::setToolTip(ui->leCENameTemplate,
                   Q("<p>%1 %2</p><p>%3 %4</p>")
                   .arg(QY("This template will be used for new chapter entries."))
                   .arg(QY("The string '<NUM>' will be replaced by the chapter number.").toHtmlEscaped())
                   .arg(QY("You can specify a minimum number of places for the chapter number with '<NUM:places>', e.g. '<NUM:3>'.").toHtmlEscaped())
                   .arg(QY("The resulting number will be padded with leading zeroes if the number of places is less than specified.")));
  Util::setToolTip(ui->cbCEDefaultLanguage, QY("This is the language that newly added chapter names get assigned automatically."));
  Util::setToolTip(ui->cbCEDefaultCountry, QY("This is the country that newly added chapter names get assigned automatically."));
  Util::setToolTip(ui->cbCEDropLastFromBlurayPlaylist,
                   Q("%1 %2")
                   .arg(QY("Blu-ray discs often contain a chapter entry very close to the end of the movie."))
                   .arg(QY("If enabled the last entry will be skipped when loading chapters from such playlists in the chapter editor if it is located within five seconds of the end of the movie.")));

  // Merge page
  Util::setToolTip(ui->cbMAutoSetFileTitle,
                   Q("<p>%1 %2</p><p>%3</p>")
                   .arg(QY("Certain file formats have 'title' property."))
                   .arg(QY("When the user adds a file containing such a title then the program will copy the title into the »file title« input box if this option is enabled."))
                   .arg(QY("Note that even if the option is disabled mkvmerge will copy a source file's title property unless a title is manually set by the user.")));

  Util::setToolTip(ui->cbMSetAudioDelayFromFileName,
                   Q("%1 %2")
                   .arg(QY("When a file is added its name is scanned."))
                   .arg(QY("If it contains the word 'DELAY' followed by a number then this number is automatically put into the 'delay' input field for any audio track found in the file.")));

  Util::setToolTip(ui->cbMDisableDefaultTrackForSubtitles, QY("If enabled all subtitle tracks will have their »default track« flag set to »no« when they're added."));

  Util::setToolTip(ui->cbMDisableCompressionForAllTrackTypes,
                   Q("%1 %2")
                   .arg(QY("Normally mkvmerge will apply additional lossless compression for subtitle tracks for certain codecs."))
                   .arg(QY("Checking this option causes the GUI to set that compression to »none« by default for all track types when adding files.")));

  Util::setToolTip(ui->cbMAlwaysAddDroppedFiles, QY("If disabled the GUI will ask whether you want to add the dropped files, append them or add them as additional parts."));
  Util::setToolTip(ui->cbMAlwaysShowOutputFileControls,
                   Q("%1 %2")
                   .arg(QY("If enabled the output file name controls will always be visible no matter which tab is currently shown."))
                   .arg(QY("Otherwise they're shown on the 'output' tab.")));

  Util::setToolTip(ui->cbMClearMergeSettings,
                   Q("<p>%1</p><ol><li>%2 %3</li><li>%4 %5</li></ol>")
                   .arg(QY("The GUI can help you start your next merge settings after having started a job or having added a one to the job queue."))
                   .arg(QY("With »create new settings« a new set of merge settings will be added."))
                   .arg(QY("The current merge settings will be closed."))
                   .arg(QY("With »remove input files« all input files will be removed."))
                   .arg(QY("Most of the other settings on the output tab will be kept intact, though.")));

  Util::setToolTip(ui->cbMDefaultTrackLanguage,
                   Q("<p>%1 %2</p><p>%3</p>")
                   .arg(QY("Certain file formats have a 'language' property for their tracks."))
                   .arg(QY("When the user adds such a file the track's language input is set to the language property from the source file."))
                   .arg(QY("The language selected here is used for tracks for which their source file contains no such property.")));

  Util::setToolTip(ui->cbMDefaultSubtitleCharset, QY("If a character set is selected here then the program will automatically set the character set input to this value for newly added text subtitle tracks."));

  Util::setToolTip(ui->leMDefaultAdditionalCommandLineOptions, QY("The options entered here are set for all new merge jobs by default."));

  Util::setToolTip(ui->cbMScanPlaylistsPolicy,
                   Q("<p>%1 %2</p><p>%3</p>")
                   .arg(QY("Whenever the user adds a playlist the program can automatically scan the directory for other playlists and present the user with a detailed list of the playlists found."))
                   .arg(QY("The user can then select which playlist to actually add."))
                   .arg(QY("This is useful for situations like Blu-ray discs on which a multitude of playlists exists in the same directory and where it is not obvious which feature (e.g. main movie, extras etc.) "
                           "a playlist belongs to.")));

  Util::setToolTip(ui->sbMMinPlaylistDuration, QY("Only playlists whose duration are at least this long are considered and offered to the user for selection."));

  Util::setToolTip(ui->cbMAutoSetOutputFileName,
                   Q("%1 %2")
                   .arg(QY("If this option is enabled and if there is currently no output file name set then the program will set one for you when you add an input file."))
                   .arg(QY("The generated output file name has the same base name as the input file name but with an extension based on the track types present in the file.")));

  Util::setToolTip(ui->cbMUniqueOutputFileNames,
                   Q("%1 %2")
                   .arg(QY("If checked the program makes sure the suggested output file name is unique by adding a number (e.g. ' (1)') to the end of the file name."))
                   .arg(QY("This is done only if there is already a file whose name matches the unmodified output file name.")));

  Util::setToolTip(ui->cbMEnableMuxingTracksByLanguage,
                   Q("<p>%1 %2 %3</p><p>%4</p>")
                   .arg(QY("When adding source files all tracks are normally set to be muxed into the output file."))
                   .arg(QY("If this option is enabled then only those tracks will be set to be muxed whose language is selected below."))
                   .arg(QY("You can exempt certain track types from this restriction by checking the corresponding check box below, e.g. for video tracks."))
                   .arg(QY("Note that the language »Undetermined (und)« is assumed for tracks for which no language is known (e.g. those read from SRT subtitle files).")));
  Util::setToolTip(ui->cbMEnableMuxingAllVideoTracks,    QY("If enabled then tracks of this type will always be set to be muxed regardless of their language."));
  Util::setToolTip(ui->cbMEnableMuxingAllAudioTracks,    QY("If enabled then tracks of this type will always be set to be muxed regardless of their language."));
  Util::setToolTip(ui->cbMEnableMuxingAllSubtitleTracks, QY("If enabled then tracks of this type will always be set to be muxed regardless of their language."));
  ui->tbMEnableMuxingTracksByLanguage->setToolTips(QY("Tracks with a language in this list will be set not to be muxed by default."),
                                                   QY("Only tracks with a language in this list will be set to be muxed by default."));

  // Often used XYZ page
  ui->tbOftenUsedLanguages->setToolTips(QY("The languages selected here will be shown at the top of all the language drop-down boxes in the program."),
                                        QY("The languages selected here will be shown at the top of all the language drop-down boxes in the program."));

  ui->tbOftenUsedCountries->setToolTips(QY("The countries selected here will be shown at the top of all the country drop-down boxes in the program."),
                                        QY("The countries selected here will be shown at the top of all the country drop-down boxes in the program."));

  ui->tbOftenUsedCharacterSets->setToolTips(QY("The character sets selected here will be shown at the top of all the character set drop-down boxes in the program."),
                                            QY("The character sets selected here will be shown at the top of all the character set drop-down boxes in the program."));
}

void
PreferencesDialog::setupConnections() {
  connect(ui->pbMEditDefaultAdditionalCommandLineOptions, &QPushButton::clicked,       this,                                 &PreferencesDialog::editDefaultAdditionalCommandLineOptions);

  connect(ui->cbGuiRemoveJobs,                            &QCheckBox::toggled,         ui->cbGuiJobRemovalPolicy,            &QComboBox::setEnabled);
  connect(ui->cbMAutoSetOutputFileName,                   &QCheckBox::toggled,         this,                                 &PreferencesDialog::enableOutputFileNameControls);
  connect(ui->rbMAutoSetSameDirectory,                    &QRadioButton::toggled,      this,                                 &PreferencesDialog::enableOutputFileNameControls);
  connect(ui->rbMAutoSetRelativeDirectory,                &QRadioButton::toggled,      this,                                 &PreferencesDialog::enableOutputFileNameControls);
  connect(ui->rbMAutoSetPreviousDirectory,                &QRadioButton::toggled,      this,                                 &PreferencesDialog::enableOutputFileNameControls);
  connect(ui->rbMAutoSetFixedDirectory,                   &QRadioButton::toggled,      this,                                 &PreferencesDialog::enableOutputFileNameControls);
  connect(ui->pbMBrowseAutoSetFixedDirectory,             &QPushButton::clicked,       this,                                 &PreferencesDialog::browseFixedOutputDirectory);

  connect(ui->cbMEnableMuxingTracksByLanguage,            &QCheckBox::toggled,         ui->lMEnableMuxingAllTracksOfType,    &QLabel::setEnabled);
  connect(ui->cbMEnableMuxingTracksByLanguage,            &QCheckBox::toggled,         ui->cbMEnableMuxingAllVideoTracks,    &QLabel::setEnabled);
  connect(ui->cbMEnableMuxingTracksByLanguage,            &QCheckBox::toggled,         ui->cbMEnableMuxingAllAudioTracks,    &QLabel::setEnabled);
  connect(ui->cbMEnableMuxingTracksByLanguage,            &QCheckBox::toggled,         ui->cbMEnableMuxingAllSubtitleTracks, &QLabel::setEnabled);
  connect(ui->cbMEnableMuxingTracksByLanguage,            &QCheckBox::toggled,         ui->tbMEnableMuxingTracksByLanguage,  &QLabel::setEnabled);

  connect(ui->buttons,                                    &QDialogButtonBox::accepted, this,                                 &PreferencesDialog::accept);
  connect(ui->buttons,                                    &QDialogButtonBox::rejected, this,                                 &PreferencesDialog::reject);
}

void
PreferencesDialog::setupOnlineCheck() {
  ui->cbGuiCheckForUpdates->setChecked(m_cfg.m_checkForUpdates);
#if !defined(HAVE_CURL_EASY_H)
  ui->cbGuiCheckForUpdates->setVisible(false);
#endif  // HAVE_CURL_EASY_H
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
}

void
PreferencesDialog::setupCommonCountries() {
  auto &allCountries = App::iso3166_1Alpha2Countries();

  ui->tbOftenUsedCountries->setItems(QList<Util::SideBySideMultiSelect::Item>::fromVector(QVector<Util::SideBySideMultiSelect::Item>::fromStdVector(allCountries)), m_cfg.m_oftenUsedCountries);
}

void
PreferencesDialog::setupCommonCharacterSets() {
  ui->tbOftenUsedCharacterSets->setItems(QList<QString>::fromVector(QVector<QString>::fromStdVector(App::characterSets())), m_cfg.m_oftenUsedCharacterSets);
}

void
PreferencesDialog::setupProcessPriority() {
#if defined(SYS_WINDOWS)
  ui->cbMProcessPriority->addItem(QY("highest"), static_cast<int>(Util::Settings::HighestPriority)); // value 4, index 0
  ui->cbMProcessPriority->addItem(QY("higher"),  static_cast<int>(Util::Settings::HighPriority));    // value 3, index 1
#endif
  ui->cbMProcessPriority->addItem(QY("normal"),  static_cast<int>(Util::Settings::NormalPriority));  // value 2, index 2/0
  ui->cbMProcessPriority->addItem(QY("lower"),   static_cast<int>(Util::Settings::LowPriority));     // value 1, index 3/1
  ui->cbMProcessPriority->addItem(QY("lowest"),  static_cast<int>(Util::Settings::LowestPriority));  // value 0, index 4/2

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
}

void
PreferencesDialog::setupOutputFileNamePolicy() {
  auto isChecked = Util::Settings::DontSetOutputFileName != m_cfg.m_outputFileNamePolicy;
  auto rbToCheck = Util::Settings::ToPreviousDirectory        == m_cfg.m_outputFileNamePolicy ? ui->rbMAutoSetPreviousDirectory
                 : Util::Settings::ToFixedDirectory           == m_cfg.m_outputFileNamePolicy ? ui->rbMAutoSetFixedDirectory
                 : Util::Settings::ToRelativeOfFirstInputFile == m_cfg.m_outputFileNamePolicy ? ui->rbMAutoSetRelativeDirectory
                 :                                                                              ui->rbMAutoSetSameDirectory;

  ui->cbMAutoSetOutputFileName->setChecked(isChecked);
  rbToCheck->setChecked(true);
  ui->leMAutoSetRelativeDirectory->setText(m_cfg.m_relativeOutputDir.path());
  ui->leMAutoSetFixedDirectory->setText(m_cfg.m_fixedOutputDir.path());
  ui->cbMUniqueOutputFileNames->setChecked(m_cfg.m_uniqueOutputFileNames);

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
PreferencesDialog::setupTabPositions() {
  ui->cbGuiTabPositions->clear();
  ui->cbGuiTabPositions->addItem(QY("top"),    static_cast<int>(QTabWidget::North));
  ui->cbGuiTabPositions->addItem(QY("bottom"), static_cast<int>(QTabWidget::South));
  ui->cbGuiTabPositions->addItem(QY("left"),   static_cast<int>(QTabWidget::West));
  ui->cbGuiTabPositions->addItem(QY("right"),  static_cast<int>(QTabWidget::East));

  Util::setComboBoxIndexIf(ui->cbGuiTabPositions, [this](QString const &, QVariant const &data) {
    return data.toInt() == static_cast<int>(m_cfg.m_tabPosition);
  });
}

void
PreferencesDialog::save() {
  // GUI page:
  m_cfg.m_uiLocale                           = ui->cbGuiInterfaceLanguage->currentData().toString();
  m_cfg.m_tabPosition                        = static_cast<QTabWidget::TabPosition>(ui->cbGuiTabPositions->currentData().toInt());
  m_cfg.m_checkForUpdates                    = ui->cbGuiCheckForUpdates->isChecked();
  m_cfg.m_disableAnimations                  = ui->cbGuiDisableAnimations->isChecked();
  m_cfg.m_showToolSelector                   = ui->cbGuiShowToolSelector->isChecked();
  m_cfg.m_warnBeforeClosingModifiedTabs      = ui->cbGuiWarnBeforeClosingModifiedTabs->isChecked();
  m_cfg.m_warnBeforeAbortingJobs             = ui->cbGuiWarnBeforeAbortingJobs->isChecked();
  m_cfg.m_showMoveUpDownButtons              = ui->cbGuiShowMoveUpDownButtons->isChecked();
  m_cfg.m_useDefaultJobDescription           = ui->cbGuiUseDefaultJobDescription->isChecked();
  m_cfg.m_showOutputOfAllJobs                = ui->cbGuiShowOutputOfAllJobs->isChecked();
  m_cfg.m_switchToJobOutputAfterStarting     = ui->cbGuiSwitchToJobOutputAfterStarting->isChecked();
  auto idx                                   = !ui->cbGuiRemoveJobs->isChecked() ? 0 : ui->cbGuiJobRemovalPolicy->currentIndex() + 1;
  m_cfg.m_jobRemovalPolicy                   = static_cast<Util::Settings::JobRemovalPolicy>(idx);

  m_cfg.m_chapterNameTemplate                = ui->leCENameTemplate->text();
  m_cfg.m_defaultChapterLanguage             = ui->cbCEDefaultLanguage->currentData().toString();
  m_cfg.m_defaultChapterCountry              = ui->cbCEDefaultCountry->currentData().toString();
  m_cfg.m_dropLastChapterFromBlurayPlaylist  = ui->cbCEDropLastFromBlurayPlaylist->isChecked();

  // Merge page:
  m_cfg.m_autoSetFileTitle                   = ui->cbMAutoSetFileTitle->isChecked();
  m_cfg.m_setAudioDelayFromFileName          = ui->cbMSetAudioDelayFromFileName->isChecked();
  m_cfg.m_disableCompressionForAllTrackTypes = ui->cbMDisableCompressionForAllTrackTypes->isChecked();
  m_cfg.m_disableDefaultTrackForSubtitles    = ui->cbMDisableDefaultTrackForSubtitles->isChecked();
  m_cfg.m_mergeAlwaysAddDroppedFiles         = ui->cbMAlwaysAddDroppedFiles->isChecked();
  m_cfg.m_mergeAlwaysShowOutputFileControls  = ui->cbMAlwaysShowOutputFileControls->isChecked();
  m_cfg.m_clearMergeSettings                 = static_cast<Util::Settings::ClearMergeSettingsAction>(ui->cbMClearMergeSettings->currentIndex());
  m_cfg.m_defaultTrackLanguage               = ui->cbMDefaultTrackLanguage->currentData().toString();
  m_cfg.m_defaultSubtitleCharset             = ui->cbMDefaultSubtitleCharset->currentData().toString();
  m_cfg.m_priority                           = static_cast<Util::Settings::ProcessPriority>(ui->cbMProcessPriority->currentData().toInt());
  m_cfg.m_defaultAdditionalMergeOptions      = ui->leMDefaultAdditionalCommandLineOptions->text();

  m_cfg.m_scanForPlaylistsPolicy             = static_cast<Util::Settings::ScanForPlaylistsPolicy>(ui->cbMScanPlaylistsPolicy->currentIndex());
  m_cfg.m_minimumPlaylistDuration            = ui->sbMMinPlaylistDuration->value();

  m_cfg.m_outputFileNamePolicy               = !ui->cbMAutoSetOutputFileName->isChecked()   ? Util::Settings::DontSetOutputFileName
                                             : ui->rbMAutoSetRelativeDirectory->isChecked() ? Util::Settings::ToRelativeOfFirstInputFile
                                             : ui->rbMAutoSetFixedDirectory->isChecked()    ? Util::Settings::ToFixedDirectory
                                             : ui->rbMAutoSetPreviousDirectory->isChecked() ? Util::Settings::ToPreviousDirectory
                                             :                                                Util::Settings::ToSameAsFirstInputFile;
  m_cfg.m_relativeOutputDir                  = ui->leMAutoSetRelativeDirectory->text();
  m_cfg.m_fixedOutputDir                     = ui->leMAutoSetFixedDirectory->text();
  m_cfg.m_uniqueOutputFileNames              = ui->cbMUniqueOutputFileNames->isChecked();

  m_cfg.m_enableMuxingTracksByLanguage       = ui->cbMEnableMuxingTracksByLanguage->isChecked();
  m_cfg.m_enableMuxingAllVideoTracks         = ui->cbMEnableMuxingAllVideoTracks->isChecked();
  m_cfg.m_enableMuxingAllAudioTracks         = ui->cbMEnableMuxingAllAudioTracks->isChecked();
  m_cfg.m_enableMuxingAllSubtitleTracks      = ui->cbMEnableMuxingAllSubtitleTracks->isChecked();
  m_cfg.m_enableMuxingTracksByTheseLanguages = ui->tbMEnableMuxingTracksByLanguage->selectedItemValues();

  // Often used selections page:
  m_cfg.m_oftenUsedLanguages                 = ui->tbOftenUsedLanguages->selectedItemValues();
  m_cfg.m_oftenUsedCountries                 = ui->tbOftenUsedCountries->selectedItemValues();
  m_cfg.m_oftenUsedCharacterSets             = ui->tbOftenUsedCharacterSets->selectedItemValues();

  m_cfg.save();
}

void
PreferencesDialog::enableOutputFileNameControls() {
  bool isChecked        = ui->cbMAutoSetOutputFileName->isChecked();
  bool relativeSelected = ui->rbMAutoSetRelativeDirectory->isChecked();
  bool fixedSelected    = ui->rbMAutoSetFixedDirectory->isChecked();

  Util::enableWidgets(QList<QWidget *>{} << ui->rbMAutoSetSameDirectory  << ui->rbMAutoSetPreviousDirectory << ui->rbMAutoSetRelativeDirectory << ui->rbMAutoSetFixedDirectory << ui->cbMUniqueOutputFileNames, isChecked);
  Util::enableWidgets(QList<QWidget *>{} << ui->leMAutoSetFixedDirectory << ui->pbMBrowseAutoSetFixedDirectory, isChecked && fixedSelected);
  ui->leMAutoSetRelativeDirectory->setEnabled(isChecked && relativeSelected);
}

void
PreferencesDialog::browseFixedOutputDirectory() {
  auto dir = QFileDialog::getExistingDirectory(this, QY("Select output directory"), ui->leMAutoSetFixedDirectory->text());
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

}}
