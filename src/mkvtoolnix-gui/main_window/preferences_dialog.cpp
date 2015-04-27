#include "common/common_pch.h"

#include <QFileDialog>

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
  : QDialog{parent}
  , ui{new Ui::PreferencesDialog}
  , m_cfg{Util::Settings::get()}
  , m_previousUiLocale{m_cfg.m_uiLocale}
{
  ui->setupUi(this);

  // GUI page
  setupOnlineCheck();
  setupInterfaceLanguage();
  setupJobsJobOutput();
  setupCommonLanguages();
  setupCommonCountries();
  setupCommonCharacterSets();

  // Merge page
  ui->cbMAutoSetFileTitle->setChecked(m_cfg.m_autoSetFileTitle);
  ui->cbMSetAudioDelayFromFileName->setChecked(m_cfg.m_setAudioDelayFromFileName);
  Util::setupLanguageComboBox(*ui->cbMDefaultTrackLanguage, m_cfg.m_defaultTrackLanguage);
  Util::setupCharacterSetComboBox(*ui->cbMDefaultSubtitleCharset, m_cfg.m_defaultSubtitleCharset);
  ui->leMDefaultAdditionalCommandLineOptions->setText(m_cfg.m_defaultAdditionalMergeOptions);

  setupProcessPriority();
  setupPlaylistScanningPolicy();
  setupOutputFileNamePolicy();

  // Chapter editor page
  Util::setupLanguageComboBox(*ui->cbCEDefaultLanguage, m_cfg.m_defaultChapterLanguage);
  Util::setupCountryComboBox(*ui->cbCEDefaultCountry, m_cfg.m_defaultChapterCountry, true, QY("– no selection by default –"));

  // Force scroll bars on combo boxes with a high number of entries.
  ui->cbMDefaultSubtitleCharset->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  ui->cbCEDefaultCountry       ->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

  setupToolTips();
  setupConnections();
}

PreferencesDialog::~PreferencesDialog() {
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
                   .arg(QY("No information is trasmitted to the server.")));

  Util::setToolTip(ui->cbGuiRemoveJobs,
                   Q("%1 %2")
                   .arg(QY("Normally completed jobs stay in the queue even over restarts until the user clears them out manually."))
                   .arg(QY("You can opt for having them removed automatically under certain conditions.")));

  Util::setToolTip(ui->cbCEDefaultLanguage, QY("This is the language that newly added chapter names get assigned automatically."));
  Util::setToolTip(ui->cbCEDefaultCountry, QY("This is the country that newly added chapter names get assigned automatically."));

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

  Util::setToolTip(ui->cbMWarnBeforeOverwriting, QY("If enabled the program will ask for confirmation before overwriting files and jobs."));
  ui->cbMWarnBeforeOverwriting->setEnabled(true);

  Util::setToolTip(ui->cbMDefaultTrackLanguage,
                   Q("<p>%1 %2</p><p>%3</p>")
                   .arg(QY("Certain file formats have a 'language' property for their tracks."))
                   .arg(QY("When the user adds such a file the track's language input is set to the language property from the source file."))
                   .arg(QY("The language selected here is used for tracks for which their source file contains no such property.")));

  Util::setToolTip(ui->cbMDefaultSubtitleCharset, QY("If a character set is selected here then the program will automatically set the character set input to this value for newly added text subtitle tracks."));

  Util::setToolTip(ui->leMDefaultAdditionalCommandLineOptions, QY("The options entered here are set for all new merge jobs by default."));

  Util::setToolTip(ui->cbMScanPlaylistsPolicy,
                   Q("<p>%1 %2</p><p>%3</p>")
                   .arg(QY("Whenever the user adds a play list the program can automatically scan the directory for other play lists and present the user with a detailed list of the play lists found."))
                   .arg(QY("The user can then select which play list to actually add."))
                   .arg(QY("This is useful for situations like Blu-ray discs on which a multitude of play lists exists in the same directory and where it is not obvious which feature (e.g. main movie, extras etc.) "
                           "a play list belongs to.")));

  Util::setToolTip(ui->sbMMinPlaylistDuration, QY("Only play lists whose duration are at least this long are considered and offered to the user for selection."));

  Util::setToolTip(ui->cbMAutoSetOutputFileName,
                   Q("%1 %2")
                   .arg(QY("If this option is enabled and if there is currently no output file name set then the program will set one for you when you add an input file."))
                   .arg(QY("The generated output file name has the same base name as the input file name but with an extension based on the track types present in the file.")));

  Util::setToolTip(ui->cbMUniqueOutputFileNames,
                   Q("%1 %2")
                   .arg(QY("If checked the program makes sure the suggested output file name is unique by adding a number (e.g. ' (1)') to the end of the file name."))
                   .arg(QY("This is done only if there is already a file whose name matches the unmodified output file name.")));

  // Often used XYZ page
  Util::setToolTip(ui->lwGuiAvailableCommonLanguages,     QY("The languages selected here will be shown at the top of all the language drop-down boxes in the program."));
  Util::setToolTip(ui->lwGuiSelectedCommonLanguages,      QY("The languages selected here will be shown at the top of all the language drop-down boxes in the program."));

  Util::setToolTip(ui->lwGuiAvailableCommonCountries,     QY("The countries selected here will be shown at the top of all the country drop-down boxes in the program."));
  Util::setToolTip(ui->lwGuiSelectedCommonCountries,      QY("The countries selected here will be shown at the top of all the country drop-down boxes in the program."));

  Util::setToolTip(ui->lwGuiAvailableCommonCharacterSets, QY("The character sets selected here will be shown at the top of all the character set drop-down boxes in the program."));
  Util::setToolTip(ui->lwGuiSelectedCommonCharacterSets,  QY("The character sets selected here will be shown at the top of all the character set drop-down boxes in the program."));
}

void
PreferencesDialog::setupConnections() {
  connect(ui->pbMEditDefaultAdditionalCommandLineOptions,          &QPushButton::clicked,                  this,                      &PreferencesDialog::editDefaultAdditionalCommandLineOptions);

  connect(ui->lwGuiAvailableCommonLanguages->selectionModel(),     &QItemSelectionModel::selectionChanged, this,                      &PreferencesDialog::availableCommonLanguagesSelectionChanged);
  connect(ui->lwGuiSelectedCommonLanguages->selectionModel(),      &QItemSelectionModel::selectionChanged, this,                      &PreferencesDialog::selectedCommonLanguagesSelectionChanged);
  connect(ui->lwGuiAvailableCommonCountries->selectionModel(),     &QItemSelectionModel::selectionChanged, this,                      &PreferencesDialog::availableCommonCountriesSelectionChanged);
  connect(ui->lwGuiSelectedCommonCountries->selectionModel(),      &QItemSelectionModel::selectionChanged, this,                      &PreferencesDialog::selectedCommonCountriesSelectionChanged);
  connect(ui->lwGuiAvailableCommonCharacterSets->selectionModel(), &QItemSelectionModel::selectionChanged, this,                      &PreferencesDialog::availableCommonCharacterSetsSelectionChanged);
  connect(ui->lwGuiSelectedCommonCharacterSets->selectionModel(),  &QItemSelectionModel::selectionChanged, this,                      &PreferencesDialog::selectedCommonCharacterSetsSelectionChanged);

  connect(ui->pbGuiAddCommonLanguages,                             &QPushButton::clicked,                  this,                      &PreferencesDialog::addCommonLanguages);
  connect(ui->pbGuiRemoveCommonLanguages,                          &QPushButton::clicked,                  this,                      &PreferencesDialog::removeCommonLanguages);
  connect(ui->pbGuiAddCommonCountries,                             &QPushButton::clicked,                  this,                      &PreferencesDialog::addCommonCountries);
  connect(ui->pbGuiRemoveCommonCountries,                          &QPushButton::clicked,                  this,                      &PreferencesDialog::removeCommonCountries);
  connect(ui->pbGuiAddCommonLanguages,                             &QPushButton::clicked,                  this,                      &PreferencesDialog::addCommonLanguages);
  connect(ui->pbGuiRemoveCommonLanguages,                          &QPushButton::clicked,                  this,                      &PreferencesDialog::removeCommonLanguages);

  connect(ui->cbGuiRemoveJobs,                                     &QCheckBox::toggled,                    ui->cbGuiJobRemovalPolicy, &QComboBox::setEnabled);
  connect(ui->cbMAutoSetOutputFileName,                            &QCheckBox::toggled,                    this,                      &PreferencesDialog::enableOutputFileNameControls);
  connect(ui->rbMAutoSetSameDirectory,                             &QRadioButton::toggled,                 this,                      &PreferencesDialog::enableOutputFileNameControls);
  connect(ui->rbMAutoSetParentDirectory,                           &QRadioButton::toggled,                 this,                      &PreferencesDialog::enableOutputFileNameControls);
  connect(ui->rbMAutoSetPreviousDirectory,                         &QRadioButton::toggled,                 this,                      &PreferencesDialog::enableOutputFileNameControls);
  connect(ui->rbMAutoSetFixedDirectory,                            &QRadioButton::toggled,                 this,                      &PreferencesDialog::enableOutputFileNameControls);
  connect(ui->pbMBrowseAutoSetFixedDirectory,                      &QPushButton::clicked,                  this,                      &PreferencesDialog::browseFixedOutputDirectory);

  connect(ui->buttons,                                             &QDialogButtonBox::accepted,            this,                      &PreferencesDialog::accept);
  connect(ui->buttons,                                             &QDialogButtonBox::rejected,            this,                      &PreferencesDialog::reject);
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
PreferencesDialog::setupJobsJobOutput() {
  auto doRemove = Util::Settings::JobRemovalPolicy::Never != m_cfg.m_jobRemovalPolicy;
  auto idx      = std::max(static_cast<int>(m_cfg.m_jobRemovalPolicy), 1) - 1;

  ui->cbGuiRemoveJobs->setChecked(doRemove);
  ui->cbGuiJobRemovalPolicy->setEnabled(doRemove);
  ui->cbGuiJobRemovalPolicy->setCurrentIndex(idx);
}

void
PreferencesDialog::setupCommonLanguages() {
  auto &languages = App::iso639Languages();
  auto isCommon   = QHash<QString, bool>{};

  for (auto const &language : m_cfg.m_oftenUsedLanguages)
    isCommon[language] = true;

  for (auto const &language : languages) {
    auto item = new QListWidgetItem{language.first};

    item->setData(Qt::UserRole, language.second);
    if (isCommon[language.second])
      ui->lwGuiSelectedCommonLanguages->addItem(item);
    else
      ui->lwGuiAvailableCommonLanguages->addItem(item);
  }

  ui->pbGuiAddCommonLanguages->setEnabled(false);
  ui->pbGuiRemoveCommonLanguages->setEnabled(false);
}

void
PreferencesDialog::setupCommonCountries() {
  auto &countries = App::iso3166_1Alpha2Countries();
  auto isCommon   = QHash<QString, bool>{};

  for (auto const &country : m_cfg.m_oftenUsedCountries)
    isCommon[country] = true;

  for (auto const &country : countries) {
    auto item = new QListWidgetItem{country.first};

    item->setData(Qt::UserRole, country.second);
    if (isCommon[country.second])
      ui->lwGuiSelectedCommonCountries->addItem(item);
    else
      ui->lwGuiAvailableCommonCountries->addItem(item);
  }

  ui->pbGuiAddCommonCountries->setEnabled(false);
  ui->pbGuiRemoveCommonCountries->setEnabled(false);
}

void
PreferencesDialog::setupCommonCharacterSets() {
  auto &characterSets = App::characterSets();
  auto isCommon       = QHash<QString, bool>{};

  for (auto const &characterSet : m_cfg.m_oftenUsedCharacterSets)
    isCommon[characterSet] = true;

  for (auto const &characterSet : characterSets) {
    auto item = new QListWidgetItem{characterSet};

    item->setData(Qt::UserRole, characterSet);
    if (isCommon[characterSet])
      ui->lwGuiSelectedCommonCharacterSets->addItem(item);
    else
      ui->lwGuiAvailableCommonCharacterSets->addItem(item);
  }

  ui->pbGuiAddCommonCharacterSets->setEnabled(false);
  ui->pbGuiRemoveCommonCharacterSets->setEnabled(false);
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
  auto rbToCheck = Util::Settings::ToPreviousDirectory      == m_cfg.m_outputFileNamePolicy ? ui->rbMAutoSetPreviousDirectory
                 : Util::Settings::ToFixedDirectory         == m_cfg.m_outputFileNamePolicy ? ui->rbMAutoSetFixedDirectory
                 : Util::Settings::ToParentOfFirstInputFile == m_cfg.m_outputFileNamePolicy ? ui->rbMAutoSetParentDirectory
                 :                                                                            ui->rbMAutoSetSameDirectory;

  ui->cbMAutoSetOutputFileName->setChecked(isChecked);
  rbToCheck->setChecked(true);
  ui->leMAutoSetFixedDirectory->setText(m_cfg.m_fixedOutputDir.path());
  ui->cbMUniqueOutputFileNames->setChecked(m_cfg.m_uniqueOutputFileNames);

  enableOutputFileNameControls();
}

void
PreferencesDialog::save() {
  // GUI page:
  m_cfg.m_uiLocale                  = ui->cbGuiInterfaceLanguage->currentData().toString();
  m_cfg.m_checkForUpdates           = ui->cbGuiCheckForUpdates->isChecked();
  auto idx                          = !ui->cbGuiRemoveJobs->isChecked() ? 0 : ui->cbGuiJobRemovalPolicy->currentIndex() + 1;
  m_cfg.m_jobRemovalPolicy          = static_cast<Util::Settings::JobRemovalPolicy>(idx);

  saveCommonList(*ui->lwGuiSelectedCommonLanguages,     m_cfg.m_oftenUsedLanguages);
  saveCommonList(*ui->lwGuiSelectedCommonCountries,     m_cfg.m_oftenUsedCountries);
  saveCommonList(*ui->lwGuiSelectedCommonCharacterSets, m_cfg.m_oftenUsedCharacterSets);

  // Merge page:
  m_cfg.m_autoSetFileTitle              = ui->cbMAutoSetFileTitle->isChecked();
  m_cfg.m_setAudioDelayFromFileName     = ui->cbMSetAudioDelayFromFileName->isChecked();
  m_cfg.m_defaultTrackLanguage          = ui->cbMDefaultTrackLanguage->currentData().toString();
  m_cfg.m_defaultSubtitleCharset        = ui->cbMDefaultSubtitleCharset->currentData().toString();
  m_cfg.m_priority                      = static_cast<Util::Settings::ProcessPriority>(ui->cbMProcessPriority->currentData().toInt());
  m_cfg.m_defaultAdditionalMergeOptions = ui->leMDefaultAdditionalCommandLineOptions->text();

  m_cfg.m_scanForPlaylistsPolicy        = static_cast<Util::Settings::ScanForPlaylistsPolicy>(ui->cbMScanPlaylistsPolicy->currentIndex());
  m_cfg.m_minimumPlaylistDuration       = ui->sbMMinPlaylistDuration->value();

  m_cfg.m_outputFileNamePolicy          = !ui->cbMAutoSetOutputFileName->isChecked()   ? Util::Settings::DontSetOutputFileName
                                        : ui->rbMAutoSetParentDirectory->isChecked()   ? Util::Settings::ToParentOfFirstInputFile
                                        : ui->rbMAutoSetFixedDirectory->isChecked()    ? Util::Settings::ToFixedDirectory
                                        : ui->rbMAutoSetPreviousDirectory->isChecked() ? Util::Settings::ToPreviousDirectory
                                        :                                                Util::Settings::ToSameAsFirstInputFile;
  m_cfg.m_fixedOutputDir                = ui->leMAutoSetFixedDirectory->text();
  m_cfg.m_uniqueOutputFileNames         = ui->cbMUniqueOutputFileNames->isChecked();

  // Chapter editor page:
  m_cfg.m_defaultChapterLanguage        = ui->cbCEDefaultLanguage->currentData().toString();
  m_cfg.m_defaultChapterCountry         = ui->cbCEDefaultCountry->currentData().toString();

  m_cfg.save();
}

void
PreferencesDialog::saveCommonList(QListWidget &from,
                                  QStringList &to) {
  to.clear();

  for (auto row = 0, numRows = from.count(); row < numRows; ++row)
    to << from.item(row)->data(Qt::UserRole).toString();
}

void
PreferencesDialog::moveSelectedListWidgetItems(QListWidget &from,
                                               QListWidget &to) {
  for (auto const &item : from.selectedItems()) {
    auto actualItem = from.takeItem(from.row(item));
    if (actualItem)
      to.addItem(actualItem);
  }
}

void
PreferencesDialog::enableOutputFileNameControls() {
  bool isChecked     = ui->cbMAutoSetOutputFileName->isChecked();
  bool fixedSelected = ui->rbMAutoSetFixedDirectory->isChecked();

  Util::enableWidgets(QList<QWidget *>{} << ui->rbMAutoSetSameDirectory  << ui->rbMAutoSetParentDirectory << ui->rbMAutoSetPreviousDirectory << ui->rbMAutoSetFixedDirectory << ui->cbMUniqueOutputFileNames, isChecked);
  Util::enableWidgets(QList<QWidget *>{} << ui->leMAutoSetFixedDirectory << ui->pbMBrowseAutoSetFixedDirectory, isChecked && fixedSelected);
}

void
PreferencesDialog::browseFixedOutputDirectory() {
  auto dir = QFileDialog::getExistingDirectory(this, QY("Select output directory"), ui->leMAutoSetFixedDirectory->text());
  if (!dir.isEmpty())
    ui->leMAutoSetFixedDirectory->setText(dir);
}

void
PreferencesDialog::availableCommonLanguagesSelectionChanged() {
  auto hasSelected = !ui->lwGuiAvailableCommonLanguages->selectedItems().isEmpty();
  ui->pbGuiAddCommonLanguages->setEnabled(hasSelected);
}

void
PreferencesDialog::selectedCommonLanguagesSelectionChanged() {
  auto hasSelected = !ui->lwGuiSelectedCommonLanguages->selectedItems().isEmpty();
  ui->pbGuiRemoveCommonLanguages->setEnabled(hasSelected);
}

void
PreferencesDialog::availableCommonCountriesSelectionChanged() {
  auto hasSelected = !ui->lwGuiAvailableCommonCountries->selectedItems().isEmpty();
  ui->pbGuiAddCommonCountries->setEnabled(hasSelected);
}

void
PreferencesDialog::selectedCommonCountriesSelectionChanged() {
  auto hasSelected = !ui->lwGuiSelectedCommonCountries->selectedItems().isEmpty();
  ui->pbGuiRemoveCommonCountries->setEnabled(hasSelected);
}

void
PreferencesDialog::availableCommonCharacterSetsSelectionChanged() {
  auto hasSelected = !ui->lwGuiAvailableCommonCharacterSets->selectedItems().isEmpty();
  ui->pbGuiAddCommonCharacterSets->setEnabled(hasSelected);
}

void
PreferencesDialog::selectedCommonCharacterSetsSelectionChanged() {
  auto hasSelected = !ui->lwGuiSelectedCommonCharacterSets->selectedItems().isEmpty();
  ui->pbGuiRemoveCommonCharacterSets->setEnabled(hasSelected);
}

void
PreferencesDialog::addCommonLanguages() {
  moveSelectedListWidgetItems(*ui->lwGuiAvailableCommonLanguages, *ui->lwGuiSelectedCommonLanguages);
}

void
PreferencesDialog::removeCommonLanguages() {
  moveSelectedListWidgetItems(*ui->lwGuiSelectedCommonLanguages, *ui->lwGuiAvailableCommonLanguages);
}

void
PreferencesDialog::addCommonCountries() {
  moveSelectedListWidgetItems(*ui->lwGuiAvailableCommonCountries, *ui->lwGuiSelectedCommonCountries);
}

void
PreferencesDialog::removeCommonCountries() {
  moveSelectedListWidgetItems(*ui->lwGuiSelectedCommonCountries, *ui->lwGuiAvailableCommonCountries);
}

void
PreferencesDialog::addCommonCharacterSets() {
  moveSelectedListWidgetItems(*ui->lwGuiAvailableCommonCharacterSets, *ui->lwGuiSelectedCommonCharacterSets);
}

void
PreferencesDialog::removeCommonCharacterSets() {
  moveSelectedListWidgetItems(*ui->lwGuiSelectedCommonCharacterSets, *ui->lwGuiAvailableCommonCharacterSets);
}

void
PreferencesDialog::editDefaultAdditionalCommandLineOptions() {
  Merge::AdditionalCommandLineOptionsDialog dlg{this, ui->leMDefaultAdditionalCommandLineOptions->text()};
  dlg.hideSaveAsDefaultCheckbox();
  if (dlg.exec())
    ui->leMDefaultAdditionalCommandLineOptions->setText(dlg.additionalOptions());
}

}}
