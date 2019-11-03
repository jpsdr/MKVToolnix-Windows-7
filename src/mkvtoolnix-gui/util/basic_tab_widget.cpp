#include "common/common_pch.h"

#include <QMouseEvent>
#include <QTabBar>
#include <QTabWidget>

#include "common/list_utils.h"
#include "mkvtoolnix-gui/util/basic_tab_widget.h"

namespace mtx::gui::Util {

using namespace mtx::gui;

class BasicTabWidgetPrivate {
  friend class BasicTabWidget;

  bool m_closeTabOnMiddleClick{true};
};

BasicTabWidget::BasicTabWidget(QWidget *parent)
  : QTabWidget{parent}
  , p_ptr{new BasicTabWidgetPrivate}
{
  tabBar()->installEventFilter(this);
}

BasicTabWidget::~BasicTabWidget() {
}

void
BasicTabWidget::setCloseTabOnMiddleClick(bool close) {
  p_func()->m_closeTabOnMiddleClick = close;
}

bool
BasicTabWidget::closeTabOnMiddleClick()
  const {
  return p_func()->m_closeTabOnMiddleClick;
}

bool
BasicTabWidget::eventFilter(QObject *o,
                            QEvent *e) {
  auto p = p_func();

  if (   p->m_closeTabOnMiddleClick
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

}
