#pragma once

#include "common/common_pch.h"

#include <Qt>
#include <QTreeView>

#include "common/qt.h"

class QDragEnterEvent;
class QDragMoveEvent;
class QDropEvent;

namespace mtx { namespace gui { namespace Util {

class BasicTreeViewPrivate;
class BasicTreeView : public QTreeView {
  Q_OBJECT

protected:
  MTX_DECLARE_PRIVATE(BasicTreeViewPrivate)

  std::unique_ptr<BasicTreeViewPrivate> const p_ptr;

  explicit BasicTreeView(QWidget *parent, BasicTreeViewPrivate &p);

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
