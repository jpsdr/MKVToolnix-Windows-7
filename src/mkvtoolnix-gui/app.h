#ifndef MTX_MKVTOOLNIX_GUI_APP_H
#define MTX_MKVTOOLNIX_GUI_APP_H

#include "common/common_pch.h"

#include <QApplication>
#include <QStringList>

namespace mtx { namespace gui {

using Iso639Language     = std::pair<QString, QString>;
using Iso639LanguageList = std::vector<Iso639Language>;

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

  static Iso639LanguageList const &getIso639Languages();
  static QStringList const &getIso3166_1Alpha2CountryCodes();
  static void initializeLanguageLists();
  static void reinitializeLanguageLists();
  static int indexOfLanguage(QString const &englishName);

  static bool isInstalled();
};

}}

#endif  // MTX_MKVTOOLNIX_GUI_APP_H
