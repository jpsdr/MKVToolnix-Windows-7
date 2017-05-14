#include "common/common_pch.h"

#include <QDragEnterEvent>
#include <QDragMoveEvent>

#include "common/list_utils.h"
#include "mkvtoolnix-gui/util/basic_tree_view.h"

namespace mtx { namespace gui { namespace Util {

using namespace mtx::gui;

BasicTreeView::BasicTreeView(QWidget *parent)
  : QTreeView{parent}
  , m_filesDDHandler{FilesDragDropHandler::Mode::Remember}
{
}

BasicTreeView::~BasicTreeView() {
}

BasicTreeView &
BasicTreeView::acceptDroppedFiles(bool enable) {
  m_acceptDroppedFiles = enable;
  return *this;
}

BasicTreeView &
BasicTreeView::enterActivatesAllSelected(bool enable) {
  m_enterActivatesAllSelected = enable;
  return *this;
}

void
BasicTreeView::dragEnterEvent(QDragEnterEvent *event) {
  if (m_acceptDroppedFiles && m_filesDDHandler.handle(event, false))
    return;

  QTreeView::dragEnterEvent(event);
}

void
BasicTreeView::dragMoveEvent(QDragMoveEvent *event) {
  if (m_acceptDroppedFiles && m_filesDDHandler.handle(event, false))
    return;

  QTreeView::dragMoveEvent(event);
}

void
BasicTreeView::dropEvent(QDropEvent *event) {
  if (m_acceptDroppedFiles && m_filesDDHandler.handle(event, true)) {
    emit filesDropped(m_filesDDHandler.fileNames(), event->mouseButtons(), event->keyboardModifiers());
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
  if (   m_enterActivatesAllSelected
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

}}}
