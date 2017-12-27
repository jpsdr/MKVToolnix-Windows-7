#pragma once

#include "common/common_pch.h"

#include "mkvtoolnix-gui/util/files_drag_drop_handler.h"

#include <QWidget>

namespace mtx { namespace gui { namespace Util {

class FilesDragDropWidgetPrivate;
class FilesDragDropWidget : public QWidget {
  Q_OBJECT;

protected:
  MTX_DECLARE_PRIVATE(FilesDragDropWidget);

  std::unique_ptr<FilesDragDropWidgetPrivate> const p_ptr;

  explicit FilesDragDropWidget(QWidget *parent, FilesDragDropWidgetPrivate &p);

public:
  explicit FilesDragDropWidget(QWidget *parent = nullptr);
  ~FilesDragDropWidget();

  virtual void dragEnterEvent(QDragEnterEvent *event) override;
  virtual void dropEvent(QDropEvent *event) override;

signals:
  void filesDropped(QStringList const &files);
};

}}}
