#pragma once

#include "common/common_pch.h"

#include "mkvtoolnix-gui/main_window/tool_base.h"
#include "mkvtoolnix-gui/util/files_drag_drop_handler.h"

class QDragEnterEvent;
class QDropEvent;
class QMenu;

namespace mtx { namespace gui { namespace ChapterEditor {

namespace Ui {
class Tool;
}

class Tab;

class Tool : public ToolBase {
  Q_OBJECT;

protected:
  // UI stuff:
  std::unique_ptr<Ui::Tool> ui;
  QMenu *m_chapterEditorMenu;
  mtx::gui::Util::FilesDragDropHandler m_filesDDHandler;

public:
  explicit Tool(QWidget *parent, QMenu *chapterEditorMenu);
  ~Tool();

  virtual void setupUi() override;
  virtual void setupActions() override;
  virtual std::pair<QString, QString> nextPreviousWindowActionTexts() const override;

public slots:
  virtual void retranslateUi();
  virtual void toolShown() override;
  virtual void tabTitleChanged();
  virtual void enableMenuActions();
  virtual void newFile();
  virtual void selectFileToOpen();
  virtual void save();
  virtual void saveAsXml();
  virtual void saveToMatroska();
  virtual void saveAllTabs();
  virtual bool closeTab(int index);
  virtual void closeCurrentTab();
  virtual void closeSendingTab();
  virtual bool closeAllTabs();
  virtual void reload();
  virtual void openFiles(QStringList const &fileNames);
  virtual void openFilesFromCommandLine(QStringList const &fileNames);
  virtual void setupTabPositions();

  virtual void removeChaptersFromExistingMatroskaFile();

protected:
  virtual Tab *appendTab(Tab *tab);
  virtual Tab *currentTab();
  virtual void forEachTab(std::function<void(Tab &)> const &worker);

  virtual void openFile(QString const &fileName);
  virtual void showChapterEditorsWidget();

  virtual void dragEnterEvent(QDragEnterEvent *event) override;
  virtual void dropEvent(QDropEvent *event) override;

public:
  static QString chapterNameTemplateToolTip();
};

}}}
