#include "common/common_pch.h"

#include <QMouseEvent>
#include <QTabBar>
#include <QTabWidget>

#include "common/list_utils.h"
#include "mkvtoolnix-gui/util/basic_tab_widget.h"

namespace mtx { namespace gui { namespace Util {

using namespace mtx::gui;

class BasicTabWidgetPrivate {
  friend class BasicTabWidget;

  bool m_closeTabOnMiddleClick{true};
};

BasicTabWidget::BasicTabWidget(QWidget *parent)
  : QTabWidget{parent}
  , d_ptr{new BasicTabWidgetPrivate}
{
  tabBar()->installEventFilter(this);
}

BasicTabWidget::~BasicTabWidget() {
}

void
BasicTabWidget::setCloseTabOnMiddleClick(bool close) {
  Q_D(BasicTabWidget);

  d->m_closeTabOnMiddleClick = close;
}

bool
BasicTabWidget::closeTabOnMiddleClick()
  const {
  Q_D(const BasicTabWidget);

  return d->m_closeTabOnMiddleClick;
}

bool
BasicTabWidget::eventFilter(QObject *o,
                            QEvent *e) {
  Q_D(BasicTabWidget);

  if (   d->m_closeTabOnMiddleClick
      && (o == tabBar())
      && (e->type() == QEvent::MouseButtonPress)) {

    auto me = static_cast<QMouseEvent *>(e);
    if (me->buttons() == Qt::MiddleButton) {
      auto tabIdx = tabBar()->tabAt(me->pos());

      if (tabIdx >= 0)
        emit tabCloseRequested(tabIdx);

      return true;
    }
  }

  return QTabWidget::eventFilter(o, e);
}

}}}
