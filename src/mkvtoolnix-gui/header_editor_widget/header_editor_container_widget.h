#ifndef MTX_MKVTOOLNIX_GUI_HEADER_EDITOR_WIDGET_HEADER_EDITOR_CONTAINER_WIDGET_H
#define MTX_MKVTOOLNIX_GUI_HEADER_EDITOR_WIDGET_HEADER_EDITOR_CONTAINER_WIDGET_H

#include "common/common_pch.h"

#include "common/qt_kax_analyzer.h"
#include "mkvtoolnix-gui/main_window/tool_base.h"

class QDragEnterEvent;
class QDropEvent;
class QMenu;

namespace Ui {
class HeaderEditorContainerWidget;
}

class HeaderEditorContainerWidget : public ToolBase {
  Q_OBJECT;

protected:
  // UI stuff:
  std::unique_ptr<Ui::HeaderEditorContainerWidget> ui;
  QMenu *m_headerEditorMenu;
public:
  explicit HeaderEditorContainerWidget(QWidget *parent, QMenu *headerEditorMenu);
  ~HeaderEditorContainerWidget();

  virtual void retranslateUi() override;

  virtual void dragEnterEvent(QDragEnterEvent *event) override;
  virtual void dropEvent(QDropEvent *event) override;

public slots:
  virtual void toolShown() override;
  virtual void selectFileToOpen();
  virtual void save();
  virtual void closeTab(int index);
  virtual void closeCurrentTab();
  virtual void reload();

protected:
  virtual void openFile(QString const &fileName);
  virtual void setupMenu();
  virtual void showHeaderEditorsWidget();
};

#endif // MTX_MKVTOOLNIX_GUI_HEADER_EDITOR_WIDGET_HEADER_EDITOR_CONTAINER_WIDGET_H
