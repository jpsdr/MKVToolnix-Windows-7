#include "common/common_pch.h"

#include <QLibraryInfo>

#include <boost/optional.hpp>

#include "common/extern_data.h"
#include "common/fs_sys_helpers.h"
#include "common/iso639.h"
#include "common/qt.h"
#include "common/translation.h"
#include "common/unique_numbers.h"
#include "common/version.h"
#include "mkvtoolnix-gui/app.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/util/settings.h"

namespace mtx { namespace gui {

static Iso639LanguageList s_iso639Languages;
static Iso3166CountryList s_iso3166_1Alpha2Countries;
static CharacterSetList s_characterSets;
static QHash<QString, QString> s_iso639_2LanguageCodeToDescription, s_iso3166_1Alpha2CountryCodeToDescription;

static boost::optional<bool> s_is_installed;

App::App(int &argc,
         char **argv)
  : QApplication{argc, argv}
{
  mtx_common_init("mkvtoolnix-gui", argv[0]);

  // The routines for handling unique numbers cannot cope with
  // multiple chapters being worked on at the same time as they safe
  // already-used numbers in one static container. So just disable them.
  ignore_unique_numbers(UNIQUE_CHAPTER_IDS);
  ignore_unique_numbers(UNIQUE_EDITION_IDS);

  QCoreApplication::setOrganizationName("bunkus.org");
  QCoreApplication::setOrganizationDomain("bunkus.org");
  QCoreApplication::setApplicationName("mkvtoolnix-gui");

#ifdef SYS_WINDOWS
  QApplication::setStyle(Q("windowsvista"));
#endif

  Util::Settings::get().load();

  QObject::connect(this, SIGNAL(aboutToQuit()), this, SLOT(saveSettings()));

  initializeLocale();
}

App::~App() {
}

void
App::saveSettings()
  const {
  Util::Settings::get().save();
}

App *
App::instance() {
  return static_cast<App *>(QApplication::instance());
}

void
App::retranslateUi() {
  if (MainWindow::get())
    MainWindow::get()->retranslateUi();
}

void
App::reinitializeLanguageLists() {
  s_iso639Languages.clear();
  s_iso3166_1Alpha2Countries.clear();
  s_iso639_2LanguageCodeToDescription.clear();
  s_iso3166_1Alpha2CountryCodeToDescription.clear();
  s_characterSets.clear();

  initializeLanguageLists();
}

void
App::initializeLanguageLists() {
  if (!s_iso639Languages.empty())
    return;

  initializeIso639Languages();
  initializeIso3166_1Alpha2Countries();
  initializeCharacterSets();
}

void
App::initializeIso639Languages() {
  using LanguageSorter = std::tuple<int, QString, QString>;
  auto toSort          = std::vector<LanguageSorter>{};
  auto &cfg            = Util::Settings::get();

  s_iso639Languages.reserve(g_iso639_languages.size());
  toSort.reserve(g_iso639_languages.size());

  for (auto const &language : g_iso639_languages) {
    auto languageCode = Q(language.iso639_2_code);
    auto description  = Q("%1 (%2)").arg(Q(language.english_name)).arg(languageCode);
    auto commonOrder  = cfg.m_oftenUsedLanguages.indexOf(languageCode) != -1 ? 0 : 1;

    toSort.emplace_back(commonOrder, description, languageCode);
    s_iso639_2LanguageCodeToDescription[languageCode] = description;
  }

  brng::sort(toSort);

  for (auto const &languageSorter : toSort) {
    s_iso639Languages.emplace_back(std::get<1>(languageSorter), std::get<2>(languageSorter));
  }
}

void
App::initializeIso3166_1Alpha2Countries() {
  using CountrySorter = std::tuple<int, QString, QString>;
  auto toSort         = std::vector<CountrySorter>{};
  auto &cfg           = Util::Settings::get();

  s_iso3166_1Alpha2Countries.reserve(g_cctlds.size());
  toSort.reserve(g_cctlds.size());

  for (auto const &country : g_cctlds) {
    auto countryCode = Q(country.code);
    auto description = Q("%1 (%2)").arg(Q(country.country)).arg(countryCode);
    auto commonOrder = cfg.m_oftenUsedCountries.indexOf(countryCode) != -1 ? 0 : 1;

    toSort.emplace_back(commonOrder, description, countryCode);
    s_iso3166_1Alpha2CountryCodeToDescription[countryCode] = description;
  }

  brng::sort(toSort);

  for (auto const &countrySorter : toSort)
    s_iso3166_1Alpha2Countries.emplace_back(std::get<1>(countrySorter), std::get<2>(countrySorter));
}

void
App::initializeCharacterSets() {
  using CharacterSetSorter = std::tuple<int, QString, QString>;
  auto toSort              = std::vector<CharacterSetSorter>{};
  auto &cfg                = Util::Settings::get();

  s_characterSets.reserve(sub_charsets.size());
  toSort.reserve(sub_charsets.size());

  for (auto const &characterSet : sub_charsets) {
    auto qCharacterSet = Q(characterSet);
    auto commonOrder   = cfg.m_oftenUsedCharacterSets.indexOf(qCharacterSet) != -1 ? 0 : 1;

    toSort.emplace_back(commonOrder, qCharacterSet.toLower(), qCharacterSet);
  }

  brng::sort(toSort);

  for (auto const &characterSetSorter : toSort)
    s_characterSets.emplace_back(std::get<2>(characterSetSorter));
}

Iso639LanguageList const &
App::iso639Languages() {
  initializeLanguageLists();
  return s_iso639Languages;
}

Iso3166CountryList const &
App::iso3166_1Alpha2Countries() {
  initializeLanguageLists();
  return s_iso3166_1Alpha2Countries;
}

CharacterSetList const &
App::characterSets() {
  initializeLanguageLists();
  return s_characterSets;
}

QString const &
App::descriptionFromIso639_2LanguageCode(QString const &code) {
  initializeLanguageLists();
  return s_iso639_2LanguageCodeToDescription.contains(code) ? s_iso639_2LanguageCodeToDescription[code] : code;
}

QString const &
App::descriptionFromIso3166_1Alpha2CountryCode(QString const &code) {
  initializeLanguageLists();
  return s_iso3166_1Alpha2CountryCodeToDescription.contains(code) ? s_iso3166_1Alpha2CountryCodeToDescription[code] : code;
}

bool
App::isInstalled() {
  if (!s_is_installed)
    s_is_installed.reset(mtx::sys::is_installed());
  return *s_is_installed;
}

void
App::initializeLocale(QString const &requestedLocale) {
  auto locale = to_utf8(requestedLocale);

#if defined(HAVE_LIBINTL_H)
  auto &cfg = Util::Settings::get();

  if (m_currentTranslator)
    removeTranslator(m_currentTranslator.get());
  m_currentTranslator.reset();

  translation_c::initialize_available_translations();

  if (locale.empty())
    locale = to_utf8(cfg.m_uiLocale);

  if (-1 == translation_c::look_up_translation(locale))
    locale = "";

  if (locale.empty()) {
    locale = boost::regex_replace(translation_c::get_default_ui_locale(), boost::regex{"\\..*", boost::regex::perl}, "");
    if (-1 == translation_c::look_up_translation(locale))
      locale = "";
  }

  if (!locale.empty()) {
    auto translator = std::make_unique<QTranslator>();
    auto paths      = QStringList{} << Q("%1/locale/%2/LC_MESSAGES").arg(applicationDirPath()).arg(Q(locale))
                                    << QLibraryInfo::location(QLibraryInfo::TranslationsPath);

    for (auto const &path : paths)
      if (translator->load(Q("qtbase_%1").arg(Q(locale)), path))
        break;

    installTranslator(translator.get());

    m_currentTranslator = std::move(translator);
    cfg.m_uiLocale      = Q(locale);
  }

#endif  // HAVE_LIBINTL_H

  init_locales(locale);

  retranslateUi();
}

}}
