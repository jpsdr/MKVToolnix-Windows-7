#ifndef MTX_MKVTOOLNIX_GUI_MERGE_TOOL_H
#define MTX_MKVTOOLNIX_GUI_MERGE_TOOL_H

#include "common/common_pch.h"

#include "mkvtoolnix-gui/main_window/tool_base.h"

// class QDragEnterEvent;
// class QDropEvent;
class QMenu;

namespace mtx { namespace gui { namespace Merge {

namespace Ui {
class Tool;
}

class Tab;

class Tool : public ToolBase {
  Q_OBJECT;

protected:
  // UI stuff:
  std::unique_ptr<Ui::Tool> ui;
  QMenu *m_mergeMenu;

public:
  explicit Tool(QWidget *parent, QMenu *mergeMenu);
  ~Tool();

  virtual void retranslateUi() override;

public slots:
  virtual void newConfig();
  virtual void openConfig();

  virtual void closeTab(int index);
  virtual void closeCurrentTab();
  virtual void closeSendingTab();

  virtual void toolShown() override;
  virtual void tabTitleChanged();
  virtual void reconnectMenuActions();

protected:
  Tab *appendTab(Tab *tab);
  virtual Tab *currentTab();

  virtual void setupActions();
  virtual void enableMenuActions();
  virtual void showMergeWidget();
};

}}}

#endif // MTX_MKVTOOLNIX_GUI_MERGE_TOOL_H
