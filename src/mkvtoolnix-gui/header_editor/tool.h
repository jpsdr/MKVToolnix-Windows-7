#ifndef MTX_MKVTOOLNIX_GUI_HEADER_EDITOR_TOOL_H
#define MTX_MKVTOOLNIX_GUI_HEADER_EDITOR_TOOL_H

#include "common/common_pch.h"

#include "mkvtoolnix-gui/main_window/tool_base.h"

class QDragEnterEvent;
class QDropEvent;
class QMenu;

namespace mtx { namespace gui { namespace HeaderEditor {

namespace Ui {
class Tool;
}

class Tab;

class Tool : public ToolBase {
  Q_OBJECT;

protected:
  // UI stuff:
  std::unique_ptr<Ui::Tool> ui;
  QMenu *m_headerEditorMenu;
public:
  explicit Tool(QWidget *parent, QMenu *headerEditorMenu);
  ~Tool();

  virtual void retranslateUi() override;

  virtual void dragEnterEvent(QDragEnterEvent *event) override;
  virtual void dropEvent(QDropEvent *event) override;

public slots:
  virtual void toolShown() override;
  virtual void selectFileToOpen();
  virtual void save();
  virtual void closeTab(int index);
  virtual void closeCurrentTab();
  virtual void closeSendingTab();
  virtual void reload();

protected:
  virtual void openFile(QString const &fileName);
  virtual void setupMenu();
  virtual void showHeaderEditorsWidget();
  virtual Tab *currentTab();
};

}}}

#endif // MTX_MKVTOOLNIX_GUI_HEADER_EDITOR_TOOL_H
