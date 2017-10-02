#pragma once

#include "common/common_pch.h"

#include <Qt>
#include <QTreeView>

#include "mkvtoolnix-gui/util/files_drag_drop_handler.h"

class QDragEnterEvent;
class QDragMoveEvent;
class QDropEvent;

namespace mtx { namespace gui { namespace Util {

class BasicTreeView : public QTreeView {
  Q_OBJECT;

protected:
  bool m_acceptDroppedFiles{}, m_enterActivatesAllSelected{};
  FilesDragDropHandler m_filesDDHandler;

public:
  BasicTreeView(QWidget *parent);
  virtual ~BasicTreeView();

  BasicTreeView &acceptDroppedFiles(bool enable);
  BasicTreeView &enterActivatesAllSelected(bool enable);

signals:
  void filesDropped(QStringList const &fileNames, Qt::MouseButtons mouseButtons, Qt::KeyboardModifiers keyboardModifiers);
  void allSelectedActivated();
  void deletePressed();
  void insertPressed();
  void ctrlUpPressed();
  void ctrlDownPressed();

public slots:
  void toggleSelectionOfCurrentItem();

protected:
  virtual void dragEnterEvent(QDragEnterEvent *event) override;
  virtual void dragMoveEvent(QDragMoveEvent *event) override;
  virtual void dropEvent(QDropEvent *event) override;
  virtual void keyPressEvent(QKeyEvent *event) override;
};

}}}
