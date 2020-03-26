#pragma once

#include "common/common_pch.h"

#include "mkvtoolnix-gui/main_window/tool_base.h"
#include "mkvtoolnix-gui/util/files_drag_drop_handler.h"

class QDragEnterEvent;
class QDropEvent;
class QMenu;

namespace mtx::gui::Info {

namespace Ui {
class Tool;
}

class Tab;

class Tool : public ToolBase {
  Q_OBJECT

protected:
  // UI stuff:
  std::unique_ptr<Ui::Tool> ui;
  QMenu *m_infoMenu;
  mtx::gui::Util::FilesDragDropHandler m_filesDDHandler;

public:
  explicit Tool(QWidget *parent, QMenu *mergeMenu);
  ~Tool();

  virtual void setupUi() override;
  virtual void setupActions() override;
  virtual std::pair<QString, QString> nextPreviousWindowActionTexts() const override;
  virtual void openFile(QString const &fileName);

public slots:
  virtual void retranslateUi();

  virtual void selectAndOpenFile();

  virtual void saveCurrentTab();

  virtual bool closeTab(int index);
  virtual void closeCurrentTab();
  virtual void closeSendingTab();
  virtual bool closeAllTabs();

  virtual void toolShown() override;
  virtual void enableMenuActions();
  virtual void tabTitleChanged();

  virtual void filesDropped(QStringList const &fileNames, Qt::MouseButtons mouseButtons);
  virtual void openMultipleFilesFromCommandLine(QStringList const &fileNames);

protected:
  Tab *appendTab(Tab *tab);
  virtual Tab *currentTab();

  virtual void showInfoWidget();

  virtual void dragEnterEvent(QDragEnterEvent *event) override;
  virtual void dropEvent(QDropEvent *event) override;
};

}
