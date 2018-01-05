#include "common/common_pch.h"

#include <QTreeView>

#include "mkvtoolnix-gui/util/tree.h"

namespace mtx { namespace gui { namespace Util {
void
expandCollapseAll(QTreeView *view,
                  bool expand,
                  QModelIndex const &parentIdx) {
  if (parentIdx.isValid())
    view->setExpanded(parentIdx, expand);

  for (auto row = 0, numRows = view->model()->rowCount(parentIdx); row < numRows; ++row)
    expandCollapseAll(view, expand, view->model()->index(row, 0, parentIdx));
}

}}}
