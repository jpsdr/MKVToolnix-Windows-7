#pragma once

#include "common/common_pch.h"

#include <QLineEdit>

#include "mkvtoolnix-gui/util/files_drag_drop_handler.h"

class QDragEnterEvent;
class QDragMoveEvent;
class QDropEvent;

namespace mtx { namespace gui { namespace Util {

class BasicLineEditPrivate;
class BasicLineEdit : public QLineEdit {
  Q_OBJECT;
  Q_DECLARE_PRIVATE(BasicLineEdit);

  QScopedPointer<BasicLineEditPrivate> const d_ptr;

public:
  BasicLineEdit(QWidget *parent);
  virtual ~BasicLineEdit();

  BasicLineEdit &acceptDroppedFiles(bool enable);
  BasicLineEdit &setTextToDroppedFileName(bool enable);

signals:
  void filesDropped(QStringList const &fileNames);
  void shiftReturnPressed();

protected:
  virtual void dragEnterEvent(QDragEnterEvent *event) override;
  virtual void dragMoveEvent(QDragMoveEvent *event) override;
  virtual void dropEvent(QDropEvent *event) override;
  virtual void keyPressEvent(QKeyEvent *event) override;
};

}}}
