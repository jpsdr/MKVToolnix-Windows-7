#include "common/common_pch.h"

#include <QEvent>
#include <QFrame>
#include <QPaintEvent>
#include <QPainter>
#include <QString>

#include "common/common_pch.h"

#include "common/qt.h"
#include "mkvtoolnix-gui/util/elide_label.h"

namespace mtx { namespace gui { namespace Util {

ElideLabel::ElideLabel(QWidget *parent,
                       Qt::WindowFlags flags)
  : QFrame{parent, flags}
{
  initVars();
}

ElideLabel::ElideLabel(QString const &text,
                       QWidget *parent,
                       Qt::WindowFlags flags)
  : QFrame{parent, flags}
  , m_text{text}
{
  initVars();
}

ElideLabel::~ElideLabel() {
}

void
ElideLabel::initVars() {
  m_elideMode = Qt::ElideMiddle;
}

QString
ElideLabel::text()
  const {
  return m_text;
}

void
ElideLabel::setText(QString const &text) {
  if (text == m_text)
    return;

  m_text = text;
  updateLabel();
  emit textChanged(m_text);
}

Qt::TextElideMode
ElideLabel::elideMode()
  const {
  return m_elideMode;
}

void
ElideLabel::setElideMode(Qt::TextElideMode mode) {
  if (mode == m_elideMode)
    return;

  m_elideMode = mode;
  updateLabel();
}

QSize
ElideLabel::sizeHint()
  const {
  auto metrics = fontMetrics();
  return QSize{ metrics.width(Q("…")), metrics.height() };
}

QSize
ElideLabel::minimumSizeHint()
  const {
  if (Qt::ElideNone == m_elideMode)
    return sizeHint();

  auto metrics = fontMetrics();
  return QSize{ metrics.width(Q("…")), metrics.height() };
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
  QFrame::paintEvent(event);

  QPainter p{this};
  auto r          = contentsRect();
  auto elidedText = fontMetrics().elidedText(m_text, m_elideMode, r.width());

  p.drawText(r, Qt::AlignLeft, elidedText);
}

void
ElideLabel::updateLabel() {
  updateGeometry();
  update();
}

}}}
