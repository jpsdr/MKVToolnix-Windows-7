#include "common/common_pch.h"

#include "common/common.h"
#include "common/iso639.h"
#include "common/qt.h"
#include "common/version.h"
#include "mkvtoolnix-gui/app.h"
#include "mkvtoolnix-gui/util/settings.h"

namespace mtx { namespace gui {

QStringList App::ms_iso639LanguageDescriptions, App::ms_iso639Language2Codes;

App::App(int &argc,
         char **argv)
  : QApplication{argc, argv}
{
  mtx_common_init("mkvtoolnix-gui", argv[0]);

  QCoreApplication::setOrganizationName("bunkus.org");
  QCoreApplication::setOrganizationDomain("bunkus.org");
  QCoreApplication::setApplicationName("mkvtoolnix-gui");

#ifdef SYS_WINDOWS
  QApplication::setStyle(Q("windows"));
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
  ms_iso639LanguageDescriptions.clear();
  ms_iso639Language2Codes.clear();

  initializeLanguageLists();
}

void
App::initializeLanguageLists() {
  if (!ms_iso639LanguageDescriptions.isEmpty())
    return;

  using LangStorage = std::pair<QString, QString>;
  auto languages    = std::vector<LangStorage>{};

  languages.reserve(iso639_languages.size());
  ms_iso639LanguageDescriptions.reserve(iso639_languages.size());
  ms_iso639Language2Codes.reserve(iso639_languages.size());

  for (auto const &language : iso639_languages)
    languages.emplace_back(Q(language.iso639_2_code), Q(language.english_name));

  brng::sort(languages, [](LangStorage const &a, LangStorage const &b) { return a.first < b.first; });

  for (auto const &language : languages) {
    ms_iso639LanguageDescriptions << Q("%1 (%2)").arg(language.second).arg(language.first);
    ms_iso639Language2Codes       << language.first;
  }
}

QStringList const &
App::getIso639LanguageDescriptions() {
  initializeLanguageLists();
  return ms_iso639LanguageDescriptions;
}

QStringList const &
App::getIso639Language2Codes() {
  initializeLanguageLists();
  return ms_iso639Language2Codes;
}

}}
