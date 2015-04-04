#ifndef MTX_MKVTOOLNIX_GUI_APP_H
#define MTX_MKVTOOLNIX_GUI_APP_H

#include "common/common_pch.h"

#include <QApplication>
#include <QStringList>

namespace mtx { namespace gui {

class App : public QApplication {
  Q_OBJECT;

protected:
  static QStringList ms_iso639LanguageDescriptions, ms_iso639Language2Codes;

public:
  App(int &argc, char **argv);
  virtual ~App();

  void retranslateUi();

public slots:
  void saveSettings() const;

public:
  static App *instance();

  static QStringList const &getIso639LanguageDescriptions();
  static QStringList const &getIso639Language2Codes();
  static void initializeLanguageLists();
  static void reinitializeLanguageLists();
};

}}

#endif  // MTX_MKVTOOLNIX_GUI_APP_H
