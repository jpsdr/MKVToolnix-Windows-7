#ifndef MTX_MKVTOOLNIX_GUI_CHAPTER_EDITOR_NAME_TREE_VIEW_H
#define MTX_MKVTOOLNIX_GUI_CHAPTER_EDITOR_NAME_TREE_VIEW_H

#include "common/common_pch.h"

#include <QTreeView>

class QDragMoveEvent;
class QDropEvent;

namespace mtx { namespace gui { namespace ChapterEditor {

class Tool;

class NameTreeView : public QTreeView {
  Q_OBJECT;

public:
  NameTreeView(QWidget *parent);
  virtual ~NameTreeView();

protected:
  virtual void dragMoveEvent(QDragMoveEvent *event) override;
  virtual void dropEvent(QDropEvent *event) override;
};

}}}

#endif  // MTX_MKVTOOLNIX_GUI_CHAPTER_EDITOR_NAME_TREE_VIEW_H
