#include "common/common_pch.h"

#include <boost/optional.hpp>

#include "common/extern_data.h"
#include "common/fs_sys_helpers.h"
#include "common/iso639.h"
#include "common/qt.h"
#include "common/version.h"
#include "mkvtoolnix-gui/app.h"
#include "mkvtoolnix-gui/util/settings.h"

namespace mtx { namespace gui {

static QStringList s_iso639LanguageDescriptions, s_iso639_2LanguageCodes, s_iso3166_1Alpha2CountryCodes;
static boost::optional<bool> s_is_installed;

App::App(int &argc,
         char **argv)
  : QApplication{argc, argv}
{
  mtx_common_init("mkvtoolnix-gui", argv[0]);

  QCoreApplication::setOrganizationName("bunkus.org");
  QCoreApplication::setOrganizationDomain("bunkus.org");
  QCoreApplication::setApplicationName("mkvtoolnix-gui");

#ifdef SYS_WINDOWS
  QApplication::setStyle(Q("windowsvista"));
#endif

  Util::Settings::get().load();

  QObject::connect(this, SIGNAL(aboutToQuit()), this, SLOT(saveSettings()));

  retranslateUi();
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
  setApplicationDisplayName(Q(get_version_info("MKVToolNix GUI")));
}

void
App::reinitializeLanguageLists() {
  s_iso639LanguageDescriptions.clear();
  s_iso639_2LanguageCodes.clear();
  s_iso3166_1Alpha2CountryCodes.clear();

  initializeLanguageLists();
}

void
App::initializeLanguageLists() {
  if (!s_iso639LanguageDescriptions.isEmpty())
    return;

  using LangStorage = std::pair<QString, QString>;
  auto languages    = std::vector<LangStorage>{};

  languages.reserve(iso639_languages.size());
  s_iso639LanguageDescriptions.reserve(iso639_languages.size());
  s_iso639_2LanguageCodes.reserve(iso639_languages.size());

  for (auto const &language : iso639_languages)
    languages.emplace_back(Q(language.iso639_2_code), Q(language.english_name));

  brng::sort(languages, [](LangStorage const &a, LangStorage const &b) { return a.first < b.first; });

  for (auto const &language : languages) {
    s_iso639LanguageDescriptions << Q("%1 (%2)").arg(language.second).arg(language.first);
    s_iso639_2LanguageCodes      << language.first;
  }

  s_iso3166_1Alpha2CountryCodes.reserve(cctlds.size());
  for (auto const &code : cctlds)
    s_iso3166_1Alpha2CountryCodes << Q(code);

  s_iso3166_1Alpha2CountryCodes.sort();
}

QStringList const &
App::getIso639LanguageDescriptions() {
  initializeLanguageLists();
  return s_iso639LanguageDescriptions;
}

QStringList const &
App::getIso639_2LanguageCodes() {
  initializeLanguageLists();
  return s_iso639_2LanguageCodes;
}

QStringList const &
App::getIso3166_1Alpha2CountryCodes() {
  initializeLanguageLists();
  return s_iso3166_1Alpha2CountryCodes;
}

bool
App::isInstalled() {
  if (!s_is_installed)
    s_is_installed.reset(mtx::sys::is_installed());
  return *s_is_installed;
}

}}
