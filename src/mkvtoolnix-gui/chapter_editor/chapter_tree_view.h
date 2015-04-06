#ifndef MTX_MKVTOOLNIX_GUI_CHAPTER_EDITOR_CHAPTER_TREE_VIEW_H
#define MTX_MKVTOOLNIX_GUI_CHAPTER_EDITOR_CHAPTER_TREE_VIEW_H

#include "common/common_pch.h"

#include <QTreeView>

namespace mtx { namespace gui { namespace ChapterEditor {

class Tool;

class ChapterTreeView : public QTreeView {
  Q_OBJECT;

public:
  ChapterTreeView(QWidget *parent);
  virtual ~ChapterTreeView();
};

}}}

#endif  // MTX_MKVTOOLNIX_GUI_CHAPTER_EDITOR_CHAPTER_TREE_VIEW_H
