#pragma once

#include "common/common_pch.h"

#include "mkvtoolnix-gui/main_window/tool_base.h"
#include "mkvtoolnix-gui/merge/file_identification_thread.h"
#include "mkvtoolnix-gui/merge/source_file.h"
#include "mkvtoolnix-gui/util/settings.h"

class QDragEnterEvent;
class QDragMoveEvent;
class QDropEvent;
class QMenu;

namespace mtx::gui::Merge {

class MuxConfig;
class Tab;

class ToolPrivate;
class Tool : public ToolBase {
  Q_OBJECT

protected:
  MTX_DECLARE_PRIVATE(ToolPrivate)

  std::unique_ptr<ToolPrivate> const p_ptr;

public:
  explicit Tool(QWidget *parent, QMenu *mergeMenu);
  ~Tool();

  virtual void setupUi() override;
  virtual void setupActions() override;
  virtual std::pair<QString, QString> nextPreviousWindowActionTexts() const override;
  virtual void openConfigFile(QString const &fileName);
  virtual void openFromConfig(MuxConfig const &config);

  virtual void addMergeTabIfNoneOpen();

public Q_SLOTS:
  virtual void applyPreferences();
  virtual void retranslateUi();

  virtual Tab *appendNewTab();
  virtual void openConfig();

  virtual bool closeTab(int index);
  virtual void closeCurrentTab();
  virtual void closeSendingTab();
  virtual bool closeAllTabs();

  virtual void saveConfig();
  virtual void saveAllConfigs();
  virtual void saveConfigAs();
  virtual void saveOptionFile();
  virtual void startMuxing();
  virtual void startMuxingAll();
  virtual void addToJobQueue();
  virtual void addAllToJobQueue();
  virtual void showCommandLine();
  virtual void copyFirstFileNameToTitle();
  virtual void copyOutputFileNameToTitle();
  virtual void copyTitleToOutputFileName();
  virtual void addFilesFromClipboard();
  virtual void toggleTrackFlag();
  virtual void changeTrackLanguage(QString const &formattedLanguage, QString const &trackName);

  virtual void toolShown() override;
  virtual void tabTitleChanged();

  virtual void identifyMultipleFiles(QStringList const &fileNamesToIdentify, Qt::MouseButtons mouseButtons);
  virtual void handleIdentifiedFiles(IdentificationPack pack);
  virtual void handleIdentifiedNonSourceFiles(IdentificationPack &pack);
  virtual void handleIdentifiedSourceFiles(IdentificationPack &sourceFiles);

  virtual void handleExternallyAddedFiles(QStringList const &fileNames, Qt::MouseButtons mouseButtons);
  virtual void handleFilesFromCommandLine(QStringList const &fileNames);

  virtual void openMultipleConfigFilesFromCommandLine(QStringList const &fileNames);

  virtual void fileIdentificationStarted();
  virtual void fileIdentificationFinished();
  virtual void handleIdentifiedXmlOrSimpleChapters(QString const &fileName);
  virtual void handleIdentifiedXmlSegmentInfo(QString const &fileName);
  virtual void handleIdentifiedXmlTags(QString const &fileName);
  virtual void showFileIdentificationError(QString const &errorTitle, QString const &errorText);
  virtual void showScanningPlaylistDialog(int numFilesToScan);
  virtual void selectScanPlaylistPolicy(SourceFilePtr const &sourceFile, QFileInfoList const &files);
  virtual void selectPlaylistToAdd(QVector<SourceFilePtr> const &identifiedPlaylists);
  virtual void hideScanningDirectoryDialog();

  virtual void setupHorizontalScrollAreaInputLayout();
  virtual void setupHorizontalTwoColumnsInputLayout();
  virtual void setupVerticalTabWidgetInputLayout();

protected:
  virtual void setupModifySelectedTracksMenu();
  virtual void setupFileIdentificationThread();

  virtual Tab *currentTab();
  virtual Tab *tabForAddingOrAppending(uint64_t wantedId);
  virtual void forEachTab(std::function<void(Tab &)> const &worker);

  virtual void enableMenuActions();
  virtual void enableCopyMenuActions();
  virtual void showMergeWidget();

  virtual std::optional<Util::Settings::MergeAddingDirectoriesPolicy> determineAddingDirectoriesPolicy();
  virtual void addFileIdentificationPack(QStringList const &fileNames, IdentificationPack::AddMode addMode, Qt::MouseButtons mouseButtons);
  virtual QStringList fileNamesFromClipboard() const;

  virtual void dragEnterEvent(QDragEnterEvent *event) override;
  virtual void dragMoveEvent(QDragMoveEvent *event) override;
  virtual void dropEvent(QDropEvent *event) override;

  virtual void retrieveDiscInformationForPlaylists(QVector<SourceFilePtr> &sourceFiles);

public:
  static FileIdentificationWorker &identifier();
};

}
