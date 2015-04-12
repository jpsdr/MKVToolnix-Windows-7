#include "common/common_pch.h"

#include <QFileDialog>

#include "common/extern_data.h"
#include "common/qt.h"
#include "common/translation.h"
#include "mkvtoolnix-gui/app.h"
#include "mkvtoolnix-gui/forms/main_window/preferences_dialog.h"
#include "mkvtoolnix-gui/main_window/preferences_dialog.h"
#include "mkvtoolnix-gui/util/util.h"

namespace mtx { namespace gui {

PreferencesDialog::PreferencesDialog(QWidget *parent)
  : QDialog{parent}
  , ui{new Ui::PreferencesDialog}
  , m_cfg{Util::Settings::get()}
{
  ui->setupUi(this);

  // GUI page
  setupOnlineCheck();
  setupInterfaceLanguage();
  setupJobsJobOutput();
  setupCommonLanguages();
  setupCommonCountries();

  // Merge page
  ui->cbMAutoSetFileTitle->setChecked(m_cfg.m_autoSetFileTitle);
  ui->cbMSetAudioDelayFromFileName->setChecked(m_cfg.m_setAudioDelayFromFileName);
  Util::setupLanguageComboBox(*ui->cbMDefaultTrackLanguage, m_cfg.m_defaultTrackLanguage);

  setupDefaultSubtitleCharset();
  setupProcessPriority();
  setupPlaylistScanningPolicy();
  setupOutputFileNamePolicy();

  // Chapter editor page
  Util::setupLanguageComboBox(*ui->cbCEDefaultLanguage, m_cfg.m_defaultChapterLanguage);
  Util::setupCountryComboBox(*ui->cbCEDefaultCountry, m_cfg.m_defaultChapterCountry, true, QY("– no selection by default –"));

  // Force scroll bars on combo boxes with a high number of entries.
  ui->cbMDefaultSubtitleCharset->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  ui->cbCEDefaultCountry       ->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

  setupConnections();
}

PreferencesDialog::~PreferencesDialog() {
}

void
PreferencesDialog::setupConnections() {
  connect(ui->lwGuiAvailableCommonLanguages->selectionModel(), &QItemSelectionModel::selectionChanged, this,                      &PreferencesDialog::availableCommonLanguagesSelectionChanged);
  connect(ui->lwGuiSelectedCommonLanguages->selectionModel(),  &QItemSelectionModel::selectionChanged, this,                      &PreferencesDialog::selectedCommonLanguagesSelectionChanged);
  connect(ui->lwGuiAvailableCommonCountries->selectionModel(), &QItemSelectionModel::selectionChanged, this,                      &PreferencesDialog::availableCommonCountriesSelectionChanged);
  connect(ui->lwGuiSelectedCommonCountries->selectionModel(),  &QItemSelectionModel::selectionChanged, this,                      &PreferencesDialog::selectedCommonCountriesSelectionChanged);

  connect(ui->pbGuiAddCommonLanguages,                         &QPushButton::clicked,                  this,                      &PreferencesDialog::addCommonLanguages);
  connect(ui->pbGuiRemoveCommonLanguages,                      &QPushButton::clicked,                  this,                      &PreferencesDialog::removeCommonLanguages);
  connect(ui->pbGuiAddCommonCountries,                         &QPushButton::clicked,                  this,                      &PreferencesDialog::addCommonCountries);
  connect(ui->pbGuiRemoveCommonCountries,                      &QPushButton::clicked,                  this,                      &PreferencesDialog::removeCommonCountries);

  connect(ui->cbGuiRemoveJobs,                                 &QCheckBox::toggled,                    ui->cbGuiJobRemovalPolicy, &QComboBox::setEnabled);
  connect(ui->cbMAutoSetOutputFileName,                        &QCheckBox::toggled,                    this,                      &PreferencesDialog::enableOutputFileNameControls);
  connect(ui->rbMAutoSetSameDirectory,                         &QRadioButton::toggled,                 this,                      &PreferencesDialog::enableOutputFileNameControls);
  connect(ui->rbMAutoSetParentDirectory,                       &QRadioButton::toggled,                 this,                      &PreferencesDialog::enableOutputFileNameControls);
  connect(ui->rbMAutoSetPreviousDirectory,                     &QRadioButton::toggled,                 this,                      &PreferencesDialog::enableOutputFileNameControls);
  connect(ui->rbMAutoSetFixedDirectory,                        &QRadioButton::toggled,                 this,                      &PreferencesDialog::enableOutputFileNameControls);
  connect(ui->pbMBrowseAutoSetFixedDirectory,                  &QPushButton::clicked,                  this,                      &PreferencesDialog::browseFixedOutputDirectory);

  connect(ui->buttons,                                         &QDialogButtonBox::accepted,            this,                      &PreferencesDialog::accept);
  connect(ui->buttons,                                         &QDialogButtonBox::rejected,            this,                      &PreferencesDialog::reject);
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

  brng::sort(translations, [](TranslationSorter const &a, TranslationSorter const &b) { return a.second < b.second; });

  for (auto const &translation : translations)
    ui->cbGuiInterfaceLanguage->addItem(translation.first, translation.second);

  // TODO: PreferencesDialog::setupInterfaceLanguage: change locale
#endif  // HAVE_LIBINTL_H
}

void
PreferencesDialog::setupJobsJobOutput() {
  // TODO: PreferencesDialog::setupJobsJobOutput
  ui->cbGuiJobRemovalPolicy->setEnabled(false);
}

void
PreferencesDialog::setupCommonLanguages() {
  auto &languages = App::getIso639Languages();
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
  auto &countries = App::getIso3166_1Alpha2Countries();
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
PreferencesDialog::setupProcessPriority() {
#if defined(SYS_WINDOWS)
  ui->cbMProcessPriority->addItem(QY("highest"), static_cast<int>(Util::Settings::HighestPriority)); // value 4, index 0
  ui->cbMProcessPriority->addItem(QY("higher"),  static_cast<int>(Util::Settings::HigherPriority));  // value 3, index 1
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
PreferencesDialog::setupDefaultSubtitleCharset() {
  ui->cbMDefaultSubtitleCharset->addItem(QY("– no selection by default –"));
  for (auto const &charset : sub_charsets)
    ui->cbMDefaultSubtitleCharset->addItem(Q(charset));

  // TODO: PreferencesDialog::setupDefaultSubtitleCharset: set current
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
  m_cfg.m_checkForUpdates           = ui->cbGuiCheckForUpdates->isChecked();
  saveCommonList(*ui->lwGuiSelectedCommonLanguages, m_cfg.m_oftenUsedLanguages);
  saveCommonList(*ui->lwGuiSelectedCommonCountries, m_cfg.m_oftenUsedCountries);

  // Merge page:
  m_cfg.m_autoSetFileTitle          = ui->cbMAutoSetFileTitle->isChecked();
  m_cfg.m_setAudioDelayFromFileName = ui->cbMSetAudioDelayFromFileName->isChecked();
  m_cfg.m_priority                  = static_cast<Util::Settings::ProcessPriority>(ui->cbMProcessPriority->currentData().toInt());
  m_cfg.m_defaultTrackLanguage      = ui->cbMDefaultTrackLanguage->currentData().toString();

  m_cfg.m_scanForPlaylistsPolicy    = static_cast<Util::Settings::ScanForPlaylistsPolicy>(ui->cbMScanPlaylistsPolicy->currentIndex());
  m_cfg.m_minimumPlaylistDuration   = ui->sbMMinPlaylistDuration->value();

  m_cfg.m_outputFileNamePolicy      = !ui->cbMAutoSetOutputFileName->isChecked()   ? Util::Settings::DontSetOutputFileName
                                    : ui->rbMAutoSetParentDirectory->isChecked()   ? Util::Settings::ToParentOfFirstInputFile
                                    : ui->rbMAutoSetFixedDirectory->isChecked()    ? Util::Settings::ToFixedDirectory
                                    : ui->rbMAutoSetPreviousDirectory->isChecked() ? Util::Settings::ToPreviousDirectory
                                    :                                                Util::Settings::ToSameAsFirstInputFile;
  m_cfg.m_fixedOutputDir            = ui->leMAutoSetFixedDirectory->text();
  m_cfg.m_uniqueOutputFileNames     = ui->cbMUniqueOutputFileNames->isChecked();

  // Chapter editor page:
  m_cfg.m_defaultChapterLanguage    = ui->cbCEDefaultLanguage->currentData().toString();
  m_cfg.m_defaultChapterCountry     = ui->cbCEDefaultCountry->currentData().toString();

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

}}
