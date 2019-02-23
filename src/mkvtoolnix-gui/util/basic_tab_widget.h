#pragma once

#include "common/common_pch.h"

#include <Qt>
#include <QTabWidget>

#include "common/qt.h"
#include "mkvtoolnix-gui/util/files_drag_drop_handler.h"

class QEvent;

namespace mtx { namespace gui { namespace Util {

class BasicTabWidgetPrivate;
class BasicTabWidget : public QTabWidget {
  Q_OBJECT

protected:
  MTX_DECLARE_PRIVATE(BasicTabWidgetPrivate)

  std::unique_ptr<BasicTabWidgetPrivate> const p_ptr;

  explicit BasicTabWidget(BasicTabWidgetPrivate &p);

public:
  BasicTabWidget(QWidget *parent);
  virtual ~BasicTabWidget();

  virtual void setCloseTabOnMiddleClick(bool close);
  virtual bool closeTabOnMiddleClick() const;

protected:
  virtual bool eventFilter(QObject *o, QEvent *e) override;
};

}}}
