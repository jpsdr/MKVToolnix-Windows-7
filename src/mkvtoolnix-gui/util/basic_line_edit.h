#pragma once

#include "common/common_pch.h"

#include <QLineEdit>

#include "common/qt.h"
#include "mkvtoolnix-gui/util/files_drag_drop_handler.h"

class QDragEnterEvent;
class QDragMoveEvent;
class QDropEvent;

namespace mtx::gui::Util {

class BasicLineEditPrivate;
class BasicLineEdit : public QLineEdit {
  Q_OBJECT

protected:
  MTX_DECLARE_PRIVATE(BasicLineEditPrivate)

  std::unique_ptr<BasicLineEditPrivate> const p_ptr;

  explicit BasicLineEdit(BasicLineEditPrivate &p);

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

}
