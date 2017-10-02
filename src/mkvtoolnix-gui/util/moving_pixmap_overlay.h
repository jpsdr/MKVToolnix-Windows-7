#pragma once

#include "common/common_pch.h"

#include <QList>
#include <QPixmap>
#include <QTimer>
#include <QWidget>

class QResizeEvent;

namespace mtx { namespace gui { namespace Util {

class MovingPixmapOverlay: public QWidget {
  Q_OBJECT;
private:
  class MovingPixmap {
  public:
    QPixmap m_pixmap;
    QPointF m_from, m_to;
    int m_step;

    MovingPixmap(QString const &pixmapName, QPointF const &from, QPointF const &to);
  };
  using MovingPixmapPtr = std::shared_ptr<MovingPixmap>;

  QList<MovingPixmapPtr> m_pixmaps;
  QTimer m_timer;
  int m_animationDuration, m_frameDuration, m_animationSteps;

public:
  MovingPixmapOverlay(QWidget *parent);
  virtual ~MovingPixmapOverlay();

  void addMovingPixmap(QString const &pixmapName, QPoint const &from, QPoint const &to);
  void setAnimation(int animationDuration, int frameDuration);

  virtual void paintEvent(QPaintEvent *event) override;

public slots:
  void stepAnimation();
};

}}}
