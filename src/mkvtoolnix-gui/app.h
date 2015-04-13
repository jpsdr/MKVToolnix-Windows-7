#ifndef MTX_MKVTOOLNIX_GUI_APP_H
#define MTX_MKVTOOLNIX_GUI_APP_H

#include "common/common_pch.h"

#include <QApplication>
#include <QStringList>

namespace mtx { namespace gui {

using Iso639Language     = std::pair<QString, QString>;
using Iso639LanguageList = std::vector<Iso639Language>;
using Iso3166Country     = std::pair<QString, QString>;
using Iso3166CountryList = std::vector<Iso3166Country>;
using CharacterSetList   = std::vector<QString>;

class App : public QApplication {
  Q_OBJECT;

public:
  App(int &argc, char **argv);
  virtual ~App();

  void retranslateUi();

public slots:
  void saveSettings() const;

public:
  static App *instance();

  static Iso639LanguageList const &iso639Languages();
  static Iso3166CountryList const &iso3166_1Alpha2Countries();
  static QString const &descriptionFromIso639_2LanguageCode(QString const &code);
  static QString const &descriptionFromIso3166_1Alpha2CountryCode(QString const &code);
  static CharacterSetList const &characterSets();

  static void reinitializeLanguageLists();
  static void initializeLanguageLists();
  static void initializeIso3166_1Alpha2Countries();
  static void initializeIso639Languages();
  static void initializeCharacterSets();

  static bool isInstalled();
};

}}

#endif  // MTX_MKVTOOLNIX_GUI_APP_H
