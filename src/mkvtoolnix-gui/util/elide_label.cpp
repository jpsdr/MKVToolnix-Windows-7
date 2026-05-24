#include "common/common_pch.h"

#include <QEvent>
#include <QFrame>
#include <QLabel>
#include <QPaintEvent>
// #include <QPainter>
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
  QLabel *m_label{};
  Qt::TextElideMode m_elideMode{Qt::ElideMiddle};
  bool m_clickable{};
};

ElideLabel::ElideLabel(QWidget *parent,
                       Qt::WindowFlags flags)
  : QFrame{parent, flags}
  , p_ptr{new ElideLabelPrivate}
{
  setup();
}

ElideLabel::ElideLabel(QString const &text,
                       QWidget *parent,
                       Qt::WindowFlags flags)
  : QFrame{parent, flags}
  , p_ptr{new ElideLabelPrivate}
{
  setup();
  p_func()->m_text = text;
}

ElideLabel::ElideLabel(QWidget *parent,
                       ElideLabelPrivate &p)
  : QFrame{parent}
  , p_ptr{&p}
{
  setup();
}

ElideLabel::~ElideLabel() {
}

void
ElideLabel::setup() {
  auto &p = *p_func();

  if (!p.m_label)
    p.m_label = new QLabel{this};

  p.m_label->setTextInteractionFlags(Qt::LinksAccessibleByMouse);

  connect(p.m_label, &QLabel::linkActivated, this, &ElideLabel::emitClickedSignal);
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
  Q_EMIT textChanged(p->m_text);
}

void
ElideLabel::setClickable(bool clickable) {
  p_func()->m_clickable = clickable;
  updateLabel();
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
  return sizeHint();
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
  auto &p = *p_func();

  QFrame::paintEvent(event);

  auto r          = contentsRect();
  auto elidedText = fontMetrics().elidedText(p.m_text, p.m_elideMode, r.width());

  p.m_label->resize(r.width(), r.height());

  if (p.m_clickable && isEnabled()) {
    p.m_label->setTextFormat(Qt::RichText);
    // Explicitly set link color from palette to ensure it updates with color scheme changes
    auto linkColor = palette().color(QPalette::Link).name();
    p.m_label->setText(Q("<a href=\"click://\" style=\"color: %1;\">%2</a>").arg(linkColor).arg(elidedText.toHtmlEscaped()));

  } else {
    p.m_label->setTextFormat(Qt::PlainText);
    p.m_label->setText(elidedText);
  }

  p.m_label->setEnabled(isEnabled());
}

void
ElideLabel::emitClickedSignal() {
  Q_EMIT clicked();
}

void
ElideLabel::updateLabel() {
  updateGeometry();
  update();
}

}
