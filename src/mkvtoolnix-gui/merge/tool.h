#pragma once

#include "common/common_pch.h"

#include "mkvtoolnix-gui/main_window/tool_base.h"
#include "mkvtoolnix-gui/util/files_drag_drop_handler.h"

class QDragEnterEvent;
class QDropEvent;
class QMenu;

namespace mtx { namespace gui { namespace Merge {

namespace Ui {
class Tool;
}

class MuxConfig;
class Tab;

class Tool : public ToolBase {
  Q_OBJECT;

protected:
  // UI stuff:
  std::unique_ptr<Ui::Tool> ui;
  QMenu *m_mergeMenu;
  mtx::gui::Util::FilesDragDropHandler m_filesDDHandler;

public:
  explicit Tool(QWidget *parent, QMenu *mergeMenu);
  ~Tool();

  virtual void setupUi() override;
  virtual void setupActions() override;
  virtual std::pair<QString, QString> nextPreviousWindowActionTexts() const override;
  virtual void openConfigFile(QString const &fileName);
  virtual void openFromConfig(MuxConfig const &config);

  virtual void addMergeTabIfNoneOpen();

public slots:
  virtual void retranslateUi();

  virtual void newConfig();
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

  virtual void toolShown() override;
  virtual void tabTitleChanged();

  virtual void filesDropped(QStringList const &fileNames, Qt::MouseButtons mouseButtons);

  virtual void addMultipleFiles(QStringList const &fileNames, Qt::MouseButtons mouseButtons);
  virtual void addMultipleFilesFromCommandLine(QStringList const &fileNames);
  virtual void openMultipleConfigFilesFromCommandLine(QStringList const &fileNames);
  virtual void addMultipleFilesToNewSettings(QStringList const &fileNames, bool newSettingsForEachFile);

  virtual void setupTabPositions();

protected:
  Tab *appendTab(Tab *tab);
  virtual Tab *currentTab();
  virtual void forEachTab(std::function<void(Tab &)> const &worker);

  virtual void enableMenuActions();
  virtual void enableCopyMenuActions();
  virtual void showMergeWidget();

  virtual void dragEnterEvent(QDragEnterEvent *event) override;
  virtual void dropEvent(QDropEvent *event) override;
};

}}}
