#include "common/common_pch.h"

#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QStandardItemModel>

#include "common/list_utils.h"
#include "mkvtoolnix-gui/util/basic_tree_view.h"
#include "mkvtoolnix-gui/util/files_drag_drop_handler.h"
#include "mkvtoolnix-gui/util/model.h"

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
    Q_EMIT filesDropped(p->m_filesDDHandler.fileNames(), event->buttons(), event->modifiers());
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
    Q_EMIT allSelectedActivated();
    event->accept();

  } else if (   (event->modifiers() == Qt::NoModifier)
             && mtx::included_in(static_cast<Qt::Key>(event->key()), Qt::Key_Backspace, Qt::Key_Delete))
    Q_EMIT deletePressed();

  else if (   (event->modifiers() == Qt::NoModifier)
           && (event->key()       == Qt::Key_Insert))
    Q_EMIT insertPressed();

  else if (   (event->modifiers() == Qt::ControlModifier)
           && (event->key()       == Qt::Key_Up))
    Q_EMIT ctrlUpPressed();

  else if (   (event->modifiers() == Qt::ControlModifier)
           && (event->key()       == Qt::Key_Down))
    Q_EMIT ctrlDownPressed();

  else if (   (event->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier))
           && (event->key()       == Qt::Key_Space))
    toggleSelectionOfCurrentItem();

  else
    QTreeView::keyPressEvent(event);
}

void
BasicTreeView::scrollToFirstSelected() {
  QModelIndex firstSelected;
  std::pair<int, int> firstSelectedRows;

  Util::withSelectedIndexes(this, [&firstSelected, &firstSelectedRows](auto const &idx) {
    auto thisRows = idx.parent().isValid() ? std::make_pair(idx.parent().row(), idx.row()) : std::make_pair(idx.row(), 0);

    if (!firstSelected.isValid() || (thisRows < firstSelectedRows)) {
      firstSelected     = idx;
      firstSelectedRows = thisRows;
    }
  });

  if (!firstSelected.isValid())
    return;

  scrollTo(firstSelected);

  if (firstSelected.parent().isValid())
    firstSelected = firstSelected.row() > 1 ? firstSelected.sibling(firstSelected.row() - 1, 0) : firstSelected.parent();

  else if (firstSelected.row() > 0) {
    firstSelected = firstSelected.sibling(firstSelected.row() - 1, 0);
    auto item     = dynamic_cast<QStandardItemModel const &>(*firstSelected.model()).itemFromIndex(firstSelected);

    if (item && (item->rowCount() > 0))
      firstSelected = item->child(item->rowCount() - 1)->index();
  }

  if (firstSelected.isValid())
    scrollTo(firstSelected);
}

}
