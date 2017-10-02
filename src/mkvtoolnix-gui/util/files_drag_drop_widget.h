#pragma once

#include "common/common_pch.h"

#include "mkvtoolnix-gui/util/files_drag_drop_handler.h"

#include <QWidget>

namespace mtx { namespace gui { namespace Util {

class FilesDragDropWidget : public QWidget {
  Q_OBJECT;

protected:
  mtx::gui::Util::FilesDragDropHandler m_filesDDHandler;

public:
  explicit FilesDragDropWidget(QWidget *parent = nullptr);
  ~FilesDragDropWidget();

  virtual void dragEnterEvent(QDragEnterEvent *event) override;
  virtual void dropEvent(QDropEvent *event) override;

signals:
  void filesDropped(QStringList const &files);
};

}}}
