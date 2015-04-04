#ifndef MTX_MKVTOOLNIX_GUI_MERGE_ATTACHMENTS_TREE_VIEW_H
#define MTX_MKVTOOLNIX_GUI_MERGE_ATTACHMENTS_TREE_VIEW_H

#include "common/common_pch.h"

#include <QTreeView>

class QDragEnterEvent;
class QDragMoveEvent;
class QDropEvent;
class QStringList;

namespace mtx { namespace gui { namespace Merge {

class AttachmentsTreeView : public QTreeView {
  Q_OBJECT;

public:
  AttachmentsTreeView(QWidget *parent = nullptr);
  virtual ~AttachmentsTreeView();

  virtual void dragEnterEvent(QDragEnterEvent *event) override;
  virtual void dragMoveEvent(QDragMoveEvent *event) override;
  virtual void dropEvent(QDropEvent *event) override;

signals:
  void filesDropped(QStringList const &fileNames);
};

}}}

#endif  // MTX_MKVTOOLNIX_GUI_MERGE_ATTACHMENTS_TREE_VIEW_H
