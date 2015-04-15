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
  QString const m_previousUiLocale;

public:
  explicit PreferencesDialog(QWidget *parent);
  ~PreferencesDialog();

  void save();
  bool uiLocaleChanged() const;

public slots:
  void enableOutputFileNameControls();
  void browseFixedOutputDirectory();

  void availableCommonLanguagesSelectionChanged();
  void selectedCommonLanguagesSelectionChanged();
  void availableCommonCountriesSelectionChanged();
  void selectedCommonCountriesSelectionChanged();
  void availableCommonCharacterSetsSelectionChanged();
  void selectedCommonCharacterSetsSelectionChanged();

  void addCommonLanguages();
  void removeCommonLanguages();
  void addCommonCountries();
  void removeCommonCountries();
  void addCommonCharacterSets();
  void removeCommonCharacterSets();

protected:
  void setupConnections();

  void setupInterfaceLanguage();
  void setupOnlineCheck();
  void setupJobsJobOutput();
  void setupCommonLanguages();
  void setupCommonCountries();
  void setupCommonCharacterSets();
  void setupProcessPriority();
  void setupPlaylistScanningPolicy();
  void setupOutputFileNamePolicy();

  void moveSelectedListWidgetItems(QListWidget &from, QListWidget &to);

  void saveCommonList(QListWidget &from, QStringList &to);
};

}}

#endif // MTX_MKVTOOLNIX_GUI_MAIN_WINDOW_PREFERENCES_DIALOG_H
