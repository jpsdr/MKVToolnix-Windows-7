#include "common/common_pch.h"

#include <QDragEnterEvent>
#include <QDragMoveEvent>

#include "common/list_utils.h"
#include "mkvtoolnix-gui/util/basic_tree_view.h"
#include "mkvtoolnix-gui/util/files_drag_drop_handler.h"

namespace mtx::gui::Util {

using namespace mtx::gui;

class BasicTreeViewPrivate {
private:
  friend class BasicTreeView;

  bool m_acceptDroppedFiles{}, m_enterActivatesAllSelected{};
  FilesDragDropHandler m_filesDDHandler{FilesDragDropHandler::Mode::Remember};
};

BasicTreeView::BasicTreeView(QWidget *parent)
  : QTreeView{parent}
  , p_ptr{new BasicTreeViewPrivate}
{
}

BasicTreeView::BasicTreeView(QWidget *parent,
                             BasicTreeViewPrivate &p)
  : QTreeView{parent}
  , p_ptr{&p}
{
}

BasicTreeView::~BasicTreeView() {
}

BasicTreeView &
BasicTreeView::acceptDroppedFiles(bool enable) {
  p_func()->m_acceptDroppedFiles = enable;
  return *this;
}

BasicTreeView &
BasicTreeView::enterActivatesAllSelected(bool enable) {
  p_func()->m_enterActivatesAllSelected = enable;
  return *this;
}

void
BasicTreeView::dragEnterEvent(QDragEnterEvent *event) {
  auto p = p_func();

  if (p->m_acceptDroppedFiles && p->m_filesDDHandler.handle(event, false))
    return;

  QTreeView::dragEnterEvent(event);
}

void
BasicTreeView::dragMoveEvent(QDragMoveEvent *event) {
  auto p = p_func();

  if (p->m_acceptDroppedFiles && p->m_filesDDHandler.handle(event, false))
    return;

  QTreeView::dragMoveEvent(event);
}

void
BasicTreeView::dropEvent(QDropEvent *event) {
  auto p = p_func();

  if (p->m_acceptDroppedFiles && p->m_filesDDHandler.handle(event, true)) {
    emit filesDropped(p->m_filesDDHandler.fileNames(), event->mouseButtons(), event->keyboardModifiers());
    return;
  }

  QTreeView::dropEvent(event);
}

void
BasicTreeView::toggleSelectionOfCurrentItem() {
  if (   mtx::included_in(selectionMode(), QAbstractItemView::ExtendedSelection, QAbstractItemView::MultiSelection)
      && currentIndex().isValid()
      && selectionModel())
    selectionModel()->select(currentIndex(), QItemSelectionModel::Toggle | QItemSelectionModel::Rows);
}

void
BasicTreeView::keyPressEvent(QKeyEvent *event) {
  auto p = p_func();

  if (   p->m_enterActivatesAllSelected
      && (event->modifiers() == Qt::NoModifier)
      && mtx::included_in(static_cast<Qt::Key>(event->key()), Qt::Key_Return, Qt::Key_Enter)) {
    emit allSelectedActivated();
    event->accept();

  } else if (   (event->modifiers() == Qt::NoModifier)
             && mtx::included_in(static_cast<Qt::Key>(event->key()), Qt::Key_Backspace, Qt::Key_Delete))
    emit deletePressed();

  else if (   (event->modifiers() == Qt::NoModifier)
           && (event->key()       == Qt::Key_Insert))
    emit insertPressed();

  else if (   (event->modifiers() == Qt::ControlModifier)
           && (event->key()       == Qt::Key_Up))
    emit ctrlUpPressed();

  else if (   (event->modifiers() == Qt::ControlModifier)
           && (event->key()       == Qt::Key_Down))
    emit ctrlDownPressed();

  else if (   (event->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier))
           && (event->key()       == Qt::Key_Space))
    toggleSelectionOfCurrentItem();

  else
    QTreeView::keyPressEvent(event);
}

}
