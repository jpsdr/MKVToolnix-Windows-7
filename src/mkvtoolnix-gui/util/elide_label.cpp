#include "common/common_pch.h"

#include <QEvent>
#include <QFrame>
#include <QPaintEvent>
#include <QPainter>
#include <QString>

#include "common/common_pch.h"

#include "common/qt.h"
#include "mkvtoolnix-gui/util/elide_label.h"
#include "mkvtoolnix-gui/util/font.h"

namespace mtx::gui::Util {

class ElideLabelPrivate {
private:
  friend class ElideLabel;

  QString m_text;
  Qt::TextElideMode m_elideMode{Qt::ElideMiddle};
};

ElideLabel::ElideLabel(QWidget *parent,
                       Qt::WindowFlags flags)
  : QFrame{parent, flags}
  , p_ptr{new ElideLabelPrivate}
{
}

ElideLabel::ElideLabel(QString const &text,
                       QWidget *parent,
                       Qt::WindowFlags flags)
  : QFrame{parent, flags}
  , p_ptr{new ElideLabelPrivate}
{
  p_func()->m_text = text;
}

ElideLabel::ElideLabel(QWidget *parent,
                       ElideLabelPrivate &p)
  : QFrame{parent}
  , p_ptr{&p}
{
}

ElideLabel::~ElideLabel() {
}

QString
ElideLabel::text()
  const {
  return p_func()->m_text;
}

void
ElideLabel::setText(QString const &text) {
  auto p = p_func();

  if (text == p->m_text)
    return;

  p->m_text = text;
  updateLabel();
  emit textChanged(p->m_text);
}

Qt::TextElideMode
ElideLabel::elideMode()
  const {
  return p_func()->m_elideMode;
}

void
ElideLabel::setElideMode(Qt::TextElideMode mode) {
  auto p = p_func();

  if (mode == p->m_elideMode)
    return;

  p->m_elideMode = mode;
  updateLabel();
}

QSize
ElideLabel::sizeHint()
  const {
  auto metrics = fontMetrics();
  return QSize{ Util::horizontalAdvance(metrics, Q("…")), metrics.height() };
}

QSize
ElideLabel::minimumSizeHint()
  const {
  if (Qt::ElideNone == p_func()->m_elideMode)
    return sizeHint();

  auto metrics = fontMetrics();
  return QSize{ Util::horizontalAdvance(metrics, Q("…")), metrics.height() };
}

void
ElideLabel::changeEvent(QEvent *event) {
  QFrame::changeEvent(event);

  auto type = event->type();
  if ((QEvent::FontChange == type) || (QEvent::ApplicationFontChange == type))
    updateLabel();
}

void
ElideLabel::paintEvent(QPaintEvent *event) {
  auto p = p_func();

  QFrame::paintEvent(event);

  auto r          = contentsRect();
  auto elidedText = fontMetrics().elidedText(p->m_text, p->m_elideMode, r.width());

  QPainter painter{this};
  painter.drawText(r, Qt::AlignLeft, elidedText);
}

void
ElideLabel::updateLabel() {
  updateGeometry();
  update();
}

}
