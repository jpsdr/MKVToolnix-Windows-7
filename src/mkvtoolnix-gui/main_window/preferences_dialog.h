#ifndef MTX_MKVTOOLNIX_GUI_MAIN_WINDOW_PREFERENCES_DIALOG_H
#define MTX_MKVTOOLNIX_GUI_MAIN_WINDOW_PREFERENCES_DIALOG_H

#include "common/common_pch.h"

#include <QDialog>

#include "mkvtoolnix-gui/util/settings.h"

class QListWidget;
class QModelIndex;

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
  void editDefaultAdditionalCommandLineOptions();
  void enableOutputFileNameControls();
  void browseFixedOutputDirectory();
  void pageSelectionChanged(QModelIndex const &current);
  void addProgramToExecute();
  void removeProgramToExecute(int index);
  void setSendersTabTitleForRunProgramExecutable(QString const &executable);
  void adjustPlaylistControls();
  void adjustRemoveOldJobsControls();

protected:
  void setupPageSelector();
  void setupToolTips();
  void setupConnections();

  void setupInterfaceLanguage();
  void setupTabPositions();
  void setupWhenToSetDefaultLanguage();
  void setupOnlineCheck();
  void setupJobRemovalPolicy();
  void setupCommonLanguages();
  void setupCommonCountries();
  void setupCommonCharacterSets();
  void setupProcessPriority();
  void setupPlaylistScanningPolicy();
  void setupOutputFileNamePolicy();
  void setupTrackPropertiesLayout();
  void setupEnableMuxingTracksByLanguage();
  void setupMergeAddingAppendingFilesPolicy();
  void setupHeaderEditorDroppedFilesPolicy();
  void setupJobsRunPrograms();
  void setupFont();

  void setTabTitleForRunProgramExecutable(int tabIdx, QString const &executable);
};

}}

#endif // MTX_MKVTOOLNIX_GUI_MAIN_WINDOW_PREFERENCES_DIALOG_H
