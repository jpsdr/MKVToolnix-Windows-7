#ifndef MTX_MKVTOOLNIX_GUI_UTIL_BASIC_TREE_VIEW_H
#define MTX_MKVTOOLNIX_GUI_UTIL_BASIC_TREE_VIEW_H

#include "common/common_pch.h"

#include <QTreeView>

#include "mkvtoolnix-gui/util/files_drag_drop_handler.h"

class QDragEnterEvent;
class QDragMoveEvent;
class QDropEvent;

namespace mtx { namespace gui { namespace Util {

class BasicTreeView : public QTreeView {
  Q_OBJECT;

protected:
  bool m_dropInFirstColumnOnly{true}, m_acceptDroppedFiles{}, m_enterActivatesAllSelected{};
  FilesDragDropHandler m_filesDDHandler;

public:
  BasicTreeView(QWidget *parent);
  virtual ~BasicTreeView();

  BasicTreeView &dropInFirstColumnOnly(bool enable);
  BasicTreeView &acceptDroppedFiles(bool enable);
  BasicTreeView &enterActivatesAllSelected(bool enable);

signals:
  void filesDropped(QStringList const &fileNames);
  void allSelectedActivated();
  void deletePressed();
  void ctrlUpPressed();
  void ctrlDownPressed();

protected:
  virtual void dragEnterEvent(QDragEnterEvent *event) override;
  virtual void dragMoveEvent(QDragMoveEvent *event) override;
  virtual void dropEvent(QDropEvent *event) override;
  virtual void keyPressEvent(QKeyEvent *event) override;
};

}}}

#endif  // MTX_MKVTOOLNIX_GUI_UTIL_BASIC_TREE_VIEW_H
