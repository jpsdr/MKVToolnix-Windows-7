#include "common/common_pch.h"

#include <boost/optional.hpp>

#include "common/extern_data.h"
#include "common/fs_sys_helpers.h"
#include "common/iso639.h"
#include "common/qt.h"
#include "common/unique_numbers.h"
#include "common/version.h"
#include "mkvtoolnix-gui/app.h"
#include "mkvtoolnix-gui/util/settings.h"

namespace mtx { namespace gui {

static Iso639LanguageList s_iso639Languages;
static QStringList s_iso3166_1Alpha2CountryCodes;
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
  s_iso639Languages.clear();
  s_iso3166_1Alpha2CountryCodes.clear();

  initializeLanguageLists();
}

void
App::initializeLanguageLists() {
  if (!s_iso639Languages.empty())
    return;

  s_iso639Languages.reserve(g_iso639_languages.size());
  for (auto const &language : g_iso639_languages)
    s_iso639Languages.emplace_back(Q("%1 (%2)").arg(Q(language.english_name)).arg(Q(language.iso639_2_code)), Q(language.iso639_2_code));

  brng::sort(s_iso639Languages);

  s_iso3166_1Alpha2CountryCodes.reserve(cctlds.size());
  for (auto const &code : cctlds)
    s_iso3166_1Alpha2CountryCodes << Q(code);

  s_iso3166_1Alpha2CountryCodes.sort();
}

Iso639LanguageList const &
App::getIso639Languages() {
  initializeLanguageLists();
  return s_iso639Languages;
}

QStringList const &
App::getIso3166_1Alpha2CountryCodes() {
  initializeLanguageLists();
  return s_iso3166_1Alpha2CountryCodes;
}

int
App::indexOfLanguage(QString const &englishName) {
  initializeLanguageLists();

  auto itr = std::find_if(s_iso639Languages.begin(), s_iso639Languages.end(), [&englishName](Iso639Language const &l) { return l.second == englishName; });
  return itr == s_iso639Languages.end() ? -1 : std::distance(s_iso639Languages.begin(), itr);
}

bool
App::isInstalled() {
  if (!s_is_installed)
    s_is_installed.reset(mtx::sys::is_installed());
  return *s_is_installed;
}

}}
