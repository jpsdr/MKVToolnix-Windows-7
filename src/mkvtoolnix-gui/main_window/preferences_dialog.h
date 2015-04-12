#ifndef MTX_MKVTOOLNIX_GUI_MAIN_WINDOW_PREFERENCES_DIALOG_H
#define MTX_MKVTOOLNIX_GUI_MAIN_WINDOW_PREFERENCES_DIALOG_H

#include "common/common_pch.h"

#include <QDialog>

#include "mkvtoolnix-gui/util/settings.h"

class QListWidget;

namespace mtx { namespace gui {

namespace Ui {
class PreferencesDialog;
}

class PreferencesDialog : public QDialog {
  Q_OBJECT;

protected:
  // UI stuff:
  std::unique_ptr<Ui::PreferencesDialog> ui;
  Util::Settings &m_cfg;

public:
  explicit PreferencesDialog(QWidget *parent);
  ~PreferencesDialog();

  void save();

public slots:
  void enableOutputFileNameControls();
  void browseFixedOutputDirectory();

  void availableCommonLanguagesSelectionChanged();
  void selectedCommonLanguagesSelectionChanged();
  void availableCommonCountriesSelectionChanged();
  void selectedCommonCountriesSelectionChanged();

  void addCommonLanguages();
  void removeCommonLanguages();
  void addCommonCountries();
  void removeCommonCountries();

protected:
  void setupConnections();

  void setupInterfaceLanguage();
  void setupOnlineCheck();
  void setupJobsJobOutput();
  void setupCommonLanguages();
  void setupCommonCountries();
  void setupProcessPriority();
  void setupDefaultSubtitleCharset();
  void setupPlaylistScanningPolicy();
  void setupOutputFileNamePolicy();
  void setupDefaultChapterCountry();

  void moveSelectedListWidgetItems(QListWidget &from, QListWidget &to);
};

}}

#endif // MTX_MKVTOOLNIX_GUI_MAIN_WINDOW_PREFERENCES_DIALOG_H
