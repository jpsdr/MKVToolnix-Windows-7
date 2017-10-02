#pragma once

#include "common/common_pch.h"

#include <Qt>
#include <QTabWidget>

#include "mkvtoolnix-gui/util/files_drag_drop_handler.h"

class QEvent;

namespace mtx { namespace gui { namespace Util {

class BasicTabWidgetPrivate;
class BasicTabWidget : public QTabWidget {
  Q_OBJECT;
  Q_DECLARE_PRIVATE(BasicTabWidget);

  QScopedPointer<BasicTabWidgetPrivate> const d_ptr;

public:
  BasicTabWidget(QWidget *parent);
  virtual ~BasicTabWidget();

  virtual void setCloseTabOnMiddleClick(bool close);
  virtual bool closeTabOnMiddleClick() const;

protected:
  virtual bool eventFilter(QObject *o, QEvent *e) override;
};

}}}
