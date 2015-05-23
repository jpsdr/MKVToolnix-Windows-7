#include "common/common_pch.h"

#include <QPainter>

#include "mkvtoolnix-gui/util/moving_pixmap_overlay.h"

namespace mtx { namespace gui { namespace Util {

MovingPixmapOverlay::MovingPixmap::MovingPixmap(QString const &pixmapName,
                                                QPointF const &from,
                                                QPointF const &to)
  : m_pixmap{pixmapName}
  , m_from{from}
  , m_to{to}
  , m_step{}
{
}

MovingPixmapOverlay::~MovingPixmapOverlay() {
}

MovingPixmapOverlay::MovingPixmapOverlay(QWidget *parent)
  : QWidget{parent}
{
  setAnimation(500, 20);

  connect(&m_timer, &QTimer::timeout, this, &MovingPixmapOverlay::stepAnimation);

  auto pal = palette();
  pal.setColor(QPalette::Background, Qt::transparent);
  setPalette(pal);

  setAttribute(Qt::WA_TransparentForMouseEvents);

  hide();
}

void
MovingPixmapOverlay::addMovingPixmap(QString const &pixmapName,
                                     QPoint const &from,
                                     QPoint const &to) {
  auto pixmap = std::make_shared<MovingPixmap>(pixmapName,
                                               QPointF{from.x() * 100.0 / width(), from.y() * 100.0 / height()},
                                               QPointF{to.x()   * 100.0 / width(), to.y()   * 100.0 / height()});
  m_pixmaps << pixmap;

  if (1 == m_pixmaps.count()) {
    show();
    m_timer.start();
  }
}

void
MovingPixmapOverlay::stepAnimation() {
  if (m_pixmaps.isEmpty())
    return;

  update();

  for (int idx = m_pixmaps.count() - 1; idx >= 0; --idx) {
    ++m_pixmaps[idx]->m_step;
    if (m_pixmaps[idx]->m_step > m_animationSteps)
      m_pixmaps.removeAt(idx);
  }

  if (m_pixmaps.isEmpty()) {
    m_timer.stop();
    hide();
  }
}

void
MovingPixmapOverlay::paintEvent(QPaintEvent *) {
  if (m_pixmaps.isEmpty())
    return;

  QPainter painter{};
  painter.begin(this);

  painter.fillRect(rect(), Qt::transparent);

  for (auto const &pixmap : m_pixmaps) {
    auto x = width()  * (pixmap->m_from.x() + (pixmap->m_to.x() - pixmap->m_from.x()) * pixmap->m_step / m_animationSteps) / 100;
    auto y = height() * (pixmap->m_from.y() + (pixmap->m_to.y() - pixmap->m_from.y()) * pixmap->m_step / m_animationSteps) / 100;

    painter.drawPixmap(QPointF{x, y}, pixmap->m_pixmap);
  }

  painter.end();
}

void
MovingPixmapOverlay::setAnimation(int animationDuration,
                                  int frameDuration) {
  m_animationDuration = animationDuration;
  m_frameDuration     = frameDuration;
  m_animationSteps    = m_animationDuration / m_frameDuration;

  m_timer.setInterval(m_frameDuration);
}

}}}
