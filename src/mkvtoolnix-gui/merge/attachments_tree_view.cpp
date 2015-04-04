#include "common/common_pch.h"

#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QFileInfo>
#include <QMimeData>
#include <QStringList>

#include "common/qt.h"
#include "mkvtoolnix-gui/merge/attachments_tree_view.h"

namespace mtx { namespace gui { namespace Merge {

using namespace mtx::gui;

AttachmentsTreeView::AttachmentsTreeView(QWidget *parent)
  : QTreeView{parent}
{
}

AttachmentsTreeView::~AttachmentsTreeView() {
}

void
AttachmentsTreeView::dragEnterEvent(QDragEnterEvent *event) {
  if (!event->mimeData()->hasUrls())
    return;

  for (auto const &url : event->mimeData()->urls())
    if (!url.isLocalFile() || !QFileInfo{url.toLocalFile()}.isFile())
      return;

  event->acceptProposedAction();
}

void
AttachmentsTreeView::dragMoveEvent(QDragMoveEvent *event) {
  event->accept();
}

void
AttachmentsTreeView::dropEvent(QDropEvent *event) {
  if (!event->mimeData()->hasUrls())
    return;

  event->acceptProposedAction();

  auto fileNames = QStringList{};
  for (auto const &url : event->mimeData()->urls())
    if (url.isLocalFile())
      fileNames << url.toLocalFile();

  emit filesDropped(fileNames);
}

}}}
