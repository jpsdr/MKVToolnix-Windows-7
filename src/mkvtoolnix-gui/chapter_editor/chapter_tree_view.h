#ifndef MTX_MKVTOOLNIX_GUI_CHAPTER_EDITOR_CHAPTER_TREE_VIEW_H
#define MTX_MKVTOOLNIX_GUI_CHAPTER_EDITOR_CHAPTER_TREE_VIEW_H

#include "common/common_pch.h"

#include <QTreeView>

#include "mkvtoolnix-gui/util/files_drag_drop_handler.h"

class QDragMoveEvent;

namespace mtx { namespace gui { namespace ChapterEditor {

class Tool;

class ChapterTreeView : public QTreeView {
  Q_OBJECT;

protected:
  mtx::gui::Util::FilesDragDropHandler m_fileDDHandler;

public:
  ChapterTreeView(QWidget *parent);
  virtual ~ChapterTreeView();

protected:
  virtual void dragMoveEvent(QDragMoveEvent *event) override;
};

}}}

#endif  // MTX_MKVTOOLNIX_GUI_CHAPTER_EDITOR_CHAPTER_TREE_VIEW_H
