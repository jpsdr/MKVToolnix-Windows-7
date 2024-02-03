/**************************************************************************
**
** This file is adapted from stylehelper.cpp which is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU General Public License Usage
**
** This file may be used under the terms of the GNU General Public
** License version 2 as published by the Free Software Foundation and
** appearing in the file COPYING included in the packaging of this file.
** Please review the following information to ensure the GNU General
** Public License version 2 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/gpl-2.0.html.
**
**************************************************************************/

#include "mkvtoolnix-gui/util/style_helper.h"

#include <QtCore/QObject>
#include <QtCore/QRect>
#include <QtGui/QPainter>
#include <QtGui/QPalette>
#include <QtGui/QPixmapCache>
#include <QtWidgets/QApplication>
#include <QtWidgets/QStyleOption>
#include <QtWidgets/QWidget>

#include "mkvtoolnix-gui/util/font.h"
#include "mkvtoolnix-gui/util/settings.h"

// libintl.h #defines sprintf to libintl_sprintf on mingw.
#undef sprintf

// Note, this is exported but in a private header as qtopengl depends on it.
// We should consider adding this as a public helper function.
void qt_blurImage(QPainter *p, QImage &blurImage, qreal radius, bool quality, bool alphaOnly, int transposed = 0);

namespace mtx::gui::Util {

// Clamps float color values within (0, 255)
static
int clamp(double x) {
  const int val = x > 255 ? 255 : static_cast<int>(x);
  return val < 0 ? 0 : val;
}

QColor
StyleHelper::mergedColors(QColor const &colorA,
                          QColor const &colorB,
                          int factor) {
  const int maxFactor = 100;
  QColor tmp = colorA;
  tmp.setRed((tmp.red() * factor) / maxFactor + (colorB.red() * (maxFactor - factor)) / maxFactor);
  tmp.setGreen((tmp.green() * factor) / maxFactor + (colorB.green() * (maxFactor - factor)) / maxFactor);
  tmp.setBlue((tmp.blue() * factor) / maxFactor + (colorB.blue() * (maxFactor - factor)) / maxFactor);
  return tmp;
}

qreal
StyleHelper::sidebarFontSize() {
  static std::optional<qreal> s_fontSize;

  if (s_fontSize)
    return s_fontSize.value();

  auto defaultFont  = mtx::gui::Util::defaultUiFont();
  auto userFontSize = mtx::gui::Util::Settings::get().m_uiFontPointSize;

#if defined(Q_WS_MAC)
  s_fontSize = userFontSize * 10.0 / defaultFont.pointSizeF();
#else
  s_fontSize = userFontSize *  7.5 / defaultFont.pointSizeF();
#endif

  return s_fontSize.value();
}

QPalette
StyleHelper::sidebarFontPalette(QPalette const &original) {
  QPalette palette = original;
  palette.setColor(QPalette::Active, QPalette::Text, panelTextColor());
  palette.setColor(QPalette::Active, QPalette::WindowText, panelTextColor());
  palette.setColor(QPalette::Inactive, QPalette::Text, panelTextColor().darker());
  palette.setColor(QPalette::Inactive, QPalette::WindowText, panelTextColor().darker());
  return palette;
}

QColor
StyleHelper::panelTextColor(bool lightColored) {
  //qApp->palette().highlightedText().color();
  if (!lightColored)
    return Qt::white;
  else
    return Qt::black;
}

// Invalid by default, setBaseColor needs to be called at least once
QColor StyleHelper::m_baseColor;
QColor StyleHelper::m_requestedBaseColor;

QColor
StyleHelper::baseColor(bool lightColored) {
  if (!lightColored)
    return m_baseColor;
  else
    return m_baseColor.lighter(230);
}

QColor
StyleHelper::highlightColor(bool lightColored) {
  QColor result = baseColor(lightColored);
  if (!lightColored)
    result.setHsv(result.hue(), clamp(result.saturation()), clamp(result.value() * 1.16));
  else
    result.setHsv(result.hue(), clamp(result.saturation()), clamp(result.value() * 1.06));
  return result;
}

QColor
StyleHelper::shadowColor(bool lightColored) {
  QColor result = baseColor(lightColored);
  result.setHsv(result.hue(), clamp(result.saturation() * 1.1), clamp(result.value() * 0.70));
  return result;
}

QColor
StyleHelper::borderColor(bool lightColored) {
  QColor result = baseColor(lightColored);
  result.setHsv(result.hue(), result.saturation(), result.value() / 2);
  return result;
}

// We try to ensure that the actual color used are within
// reasonalbe bounds while generating the actual baseColor
// from the users request.
void
StyleHelper::setBaseColor(QColor const &newcolor) {
  m_requestedBaseColor = newcolor;

  QColor color;
  color.setHsv(newcolor.hue(), newcolor.saturation() * 0.7, 64 + newcolor.value() / 3);

  if (color.isValid() && color != m_baseColor) {
    m_baseColor = color;
    for (auto & w : QApplication::topLevelWidgets())
      w->update();
  }
}

static void
verticalGradientHelper(QPainter *p,
                       QRect const &spanRect,
                       QRect const &rect,
                       bool lightColored) {
  QColor highlight = StyleHelper::highlightColor(lightColored);
  QColor shadow = StyleHelper::shadowColor(lightColored);
  QLinearGradient grad(spanRect.topRight(), spanRect.topLeft());
  grad.setColorAt(0, highlight.lighter(117));
  grad.setColorAt(1, shadow.darker(109));
  p->fillRect(rect, grad);

  QColor light(255, 255, 255, 80);
  p->setPen(light);
  p->drawLine(rect.topRight() - QPoint(1, 0), rect.bottomRight() - QPoint(1, 0));
  QColor dark(0, 0, 0, 90);
  p->setPen(dark);
  p->drawLine(rect.topLeft(), rect.bottomLeft());
}

void
StyleHelper::verticalGradient(QPainter *painter,
                              QRect const &spanRect,
                              QRect const &clipRect,
                              bool lightColored) {
  if (StyleHelper::usePixmapCache()) {
    QColor keyColor = baseColor(lightColored);
    auto key = QString::fromUtf8("mh_vertical %1 %2 %3 %4 %5").arg(spanRect.width()).arg(spanRect.height()).arg(clipRect.width()).arg(clipRect.height()).arg(keyColor.rgb());

    QPixmap pixmap;
    if (!QPixmapCache::find(key, &pixmap)) {
      pixmap = QPixmap(clipRect.size());
      QPainter p(&pixmap);
      QRect rect(0, 0, clipRect.width(), clipRect.height());
      verticalGradientHelper(&p, spanRect, rect, lightColored);
      p.end();
      QPixmapCache::insert(key, pixmap);
    }

    painter->drawPixmap(clipRect.topLeft(), pixmap);
  } else {
    verticalGradientHelper(painter, spanRect, clipRect, lightColored);
  }
}

static void
horizontalGradientHelper(QPainter *p,
                         QRect const &spanRect,
                         const QRect &rect,
                         bool lightColored) {
  if (lightColored) {
    QLinearGradient shadowGradient(rect.topLeft(), rect.bottomLeft());
    shadowGradient.setColorAt(0, 0xf0f0f0);
    shadowGradient.setColorAt(1, 0xcfcfcf);
    p->fillRect(rect, shadowGradient);
    return;
  }

  QColor base = StyleHelper::baseColor(lightColored);
  QColor highlight = StyleHelper::highlightColor(lightColored);
  QColor shadow = StyleHelper::shadowColor(lightColored);
  QLinearGradient grad(rect.topLeft(), rect.bottomLeft());
  grad.setColorAt(0, highlight.lighter(120));
  if (rect.height() == StyleHelper::navigationWidgetHeight()) {
    grad.setColorAt(0.4, highlight);
    grad.setColorAt(0.401, base);
  }
  grad.setColorAt(1, shadow);
  p->fillRect(rect, grad);

  QLinearGradient shadowGradient(spanRect.topLeft(), spanRect.topRight());
  shadowGradient.setColorAt(0, QColor(0, 0, 0, 30));
  QColor lighterHighlight;
  lighterHighlight = highlight.lighter(130);
  lighterHighlight.setAlpha(100);
  shadowGradient.setColorAt(0.7, lighterHighlight);
  shadowGradient.setColorAt(1, QColor(0, 0, 0, 40));
  p->fillRect(rect, shadowGradient);
}

void
StyleHelper::horizontalGradient(QPainter *painter,
                                QRect const &spanRect,
                                QRect const &clipRect,
                                bool lightColored) {
  if (StyleHelper::usePixmapCache()) {
    QColor keyColor = baseColor(lightColored);
    auto key = QString::fromUtf8("mh_horizontal %1 %2 %3 %4 %5 %6").arg(spanRect.width()).arg(spanRect.height()).arg(clipRect.width()).arg(clipRect.height()).arg(keyColor.rgb()).arg(spanRect.x());

    QPixmap pixmap;
    if (!QPixmapCache::find(key, &pixmap)) {
      pixmap = QPixmap(clipRect.size());
      QPainter p(&pixmap);
      QRect rect = QRect(0, 0, clipRect.width(), clipRect.height());
      horizontalGradientHelper(&p, spanRect, rect, lightColored);
      p.end();
      QPixmapCache::insert(key, pixmap);
    }

    painter->drawPixmap(clipRect.topLeft(), pixmap);

  } else {
    horizontalGradientHelper(painter, spanRect, clipRect, lightColored);
  }
}

static void
menuGradientHelper(QPainter *p,
                   QRect const &spanRect,
                   QRect const &rect) {
  QLinearGradient grad(spanRect.topLeft(), spanRect.bottomLeft());
  QColor menuColor = StyleHelper::mergedColors(StyleHelper::baseColor(), QColor(244, 244, 244), 25);
  grad.setColorAt(0, menuColor.lighter(112));
  grad.setColorAt(1, menuColor);
  p->fillRect(rect, grad);
}

void
StyleHelper::drawArrow(QStyle::PrimitiveElement element,
                       QPainter *painter,
                       QStyleOption const *option) {
  // From windowsstyle but modified to enable AA
  if (option->rect.width() <= 1 || option->rect.height() <= 1)
    return;

  QRect r = option->rect;
  int size = qMin(r.height(), r.width());
  QPixmap pixmap;
  auto pixmapName = QString::fromUtf8("arrow-%1-%2-%3-%4-%5").arg("$qt_ia").arg(uint(option->state)).arg(element).arg(size).arg(option->palette.cacheKey());
  if (!QPixmapCache::find(pixmapName, &pixmap)) {
    int border = size/5;
    int sqsize = 2*(size/2);
    QImage image(sqsize, sqsize, QImage::Format_ARGB32);
    image.fill(Qt::transparent);
    QPainter imagePainter(&image);
    imagePainter.setRenderHint(QPainter::Antialiasing, true);
    imagePainter.translate(0.5, 0.5);
    QPolygon a;
    switch (element) {
      case QStyle::PE_IndicatorArrowUp:
        a.setPoints(3, border, sqsize/2,  sqsize/2, border,  sqsize - border, sqsize/2);
        break;
      case QStyle::PE_IndicatorArrowDown:
        a.setPoints(3, border, sqsize/2,  sqsize/2, sqsize - border,  sqsize - border, sqsize/2);
        break;
      case QStyle::PE_IndicatorArrowRight:
        a.setPoints(3, sqsize - border, sqsize/2,  sqsize/2, border,  sqsize/2, sqsize - border);
        break;
      case QStyle::PE_IndicatorArrowLeft:
        a.setPoints(3, border, sqsize/2,  sqsize/2, border,  sqsize/2, sqsize - border);
        break;
      default:
        break;
    }

    int bsx = 0;
    int bsy = 0;

    if (option->state & QStyle::State_Sunken) {
      bsx = qApp->style()->pixelMetric(QStyle::PM_ButtonShiftHorizontal);
      bsy = qApp->style()->pixelMetric(QStyle::PM_ButtonShiftVertical);
    }

    QRect bounds = a.boundingRect();
    int sx = sqsize / 2 - bounds.center().x() - 1;
    int sy = sqsize / 2 - bounds.center().y() - 1;
    imagePainter.translate(sx + bsx, sy + bsy);

    if (!(option->state & QStyle::State_Enabled)) {
      imagePainter.setBrush(option->palette.mid().color());
      imagePainter.setPen(option->palette.mid().color());
    } else {
      QColor shadow(0, 0, 0, 100);
      imagePainter.translate(0, 1);
      imagePainter.setPen(shadow);
      imagePainter.setBrush(shadow);
      QColor foreGround(255, 255, 255, 210);
      imagePainter.drawPolygon(a);
      imagePainter.translate(0, -1);
      imagePainter.setPen(foreGround);
      imagePainter.setBrush(foreGround);
    }
    imagePainter.drawPolygon(a);
    imagePainter.end();
    pixmap = QPixmap::fromImage(image);
    QPixmapCache::insert(pixmapName, pixmap);
  }
  int xOffset = r.x() + (r.width() - size)/2;
  int yOffset = r.y() + (r.height() - size)/2;
  painter->drawPixmap(xOffset, yOffset, pixmap);
}

void
StyleHelper::menuGradient(QPainter *painter,
                          QRect const &spanRect,
                          QRect const &clipRect) {
  if (StyleHelper::usePixmapCache()) {
    auto key = QString::fromUtf8("mh_menu %1 %2 %3 %4 %5").arg(spanRect.width()).arg(spanRect.height()).arg(clipRect.width()).arg(clipRect.height()).arg(StyleHelper::baseColor().rgb());
    QPixmap pixmap;
    if (!QPixmapCache::find(key, &pixmap)) {
      pixmap = QPixmap(clipRect.size());
      QPainter p(&pixmap);
      QRect rect = QRect(0, 0, clipRect.width(), clipRect.height());
      menuGradientHelper(&p, spanRect, rect);
      p.end();
      QPixmapCache::insert(key, pixmap);
    }

    painter->drawPixmap(clipRect.topLeft(), pixmap);
  } else {
    menuGradientHelper(painter, spanRect, clipRect);
  }
}

// Draws a cached pixmap with shadow
void
StyleHelper::drawIconWithShadow(QIcon const &icon,
                                QRect const &rect,
                                QPainter *p,
                                QIcon::Mode iconMode,
                                int dipRadius,
                                QColor const &color,
                                QPoint const &dipOffset) {
  QPixmap cache;
  auto const devicePixelRatio = p->device()->devicePixelRatioF();
  QString pixmapName = Q("icon %0 %1 %2 %3").arg(icon.cacheKey()).arg(iconMode).arg(rect.height()).arg(devicePixelRatio);

  if (!QPixmapCache::find(pixmapName, &cache)) {
    // High-dpi support: The in parameters (rect, radius, offset) are in
    // device-independent pixels. The call to QIcon::pixmap() below might
    // return a high-dpi pixmap, which will in that case have a devicePixelRatio
    // different than 1. The shadow drawing caluculations are done in device
    // pixels.
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QPixmap px = icon.pixmap(rect.size(), devicePixelRatio, iconMode);
#else
    QWindow *window = dynamic_cast<QWidget*>(p->device())->window()->windowHandle();
    QPixmap px = icon.pixmap(window, rect.size(), iconMode);
#endif
    int radius = dipRadius * devicePixelRatio;
    QPoint offset = dipOffset * devicePixelRatio;
    cache = QPixmap(px.size() + QSize(radius * 2, radius * 2));
    cache.fill(Qt::transparent);

    QPainter cachePainter(&cache);

    // Draw shadow
    QImage tmp(px.size() + QSize(radius * 2, radius * 2 + 1), QImage::Format_ARGB32_Premultiplied);
    tmp.fill(Qt::transparent);

    QPainter tmpPainter(&tmp);
    tmpPainter.setCompositionMode(QPainter::CompositionMode_Source);
    tmpPainter.drawPixmap(QRect(radius, radius, px.width(), px.height()), px);
    tmpPainter.end();

    // blur the alpha channel
    QImage blurred(tmp.size(), QImage::Format_ARGB32_Premultiplied);
    blurred.fill(Qt::transparent);
    QPainter blurPainter(&blurred);
    qt_blurImage(&blurPainter, tmp, radius, false, true);
    blurPainter.end();

    tmp = blurred;

    // blacken the image...
    tmpPainter.begin(&tmp);
    tmpPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    tmpPainter.fillRect(tmp.rect(), color);
    tmpPainter.end();

    tmpPainter.begin(&tmp);
    tmpPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    tmpPainter.fillRect(tmp.rect(), color);
    tmpPainter.end();

    // draw the blurred drop shadow...
    cachePainter.drawImage(QRect(0, 0, cache.rect().width(), cache.rect().height()), tmp);

    // Draw the actual pixmap...
    cachePainter.drawPixmap(QRect(QPoint(radius, radius) + offset, QSize(px.width(), px.height())), px);
    cachePainter.end();
    cache.setDevicePixelRatio(devicePixelRatio);
    QPixmapCache::insert(pixmapName, cache);
  }

  QRect targetRect = cache.rect();
  targetRect.setSize(targetRect.size() / cache.devicePixelRatio());
  targetRect.moveCenter(rect.center() - dipOffset);
  p->drawPixmap(targetRect, cache);
}

// Draws a CSS-like border image where the defined borders are not stretched
void
StyleHelper::drawCornerImage(QImage const &img,
                             QPainter *painter,
                             QRect rect,
                             int left,
                             int top,
                             int right,
                             int bottom) {
  QSize size = img.size();
  if (top > 0) { //top
    painter->drawImage(QRect(rect.left() + left, rect.top(), rect.width() -right - left, top), img,
                       QRect(left, 0, size.width() -right - left, top));
    if (left > 0) //top-left
      painter->drawImage(QRect(rect.left(), rect.top(), left, top), img,
                         QRect(0, 0, left, top));
    if (right > 0) //top-right
      painter->drawImage(QRect(rect.left() + rect.width() - right, rect.top(), right, top), img,
                         QRect(size.width() - right, 0, right, top));
  }
  //left
  if (left > 0)
    painter->drawImage(QRect(rect.left(), rect.top()+top, left, rect.height() - top - bottom), img,
                       QRect(0, top, left, size.height() - bottom - top));
  //center
  painter->drawImage(QRect(rect.left() + left, rect.top()+top, rect.width() -right - left,
                           rect.height() - bottom - top), img,
                     QRect(left, top, size.width() -right -left,
                           size.height() - bottom - top));
  if (right > 0) //right
    painter->drawImage(QRect(rect.left() +rect.width() - right, rect.top()+top, right, rect.height() - top - bottom), img,
                       QRect(size.width() - right, top, right, size.height() - bottom - top));
  if (bottom > 0) { //bottom
    painter->drawImage(QRect(rect.left() +left, rect.top() + rect.height() - bottom,
                             rect.width() - right - left, bottom), img,
                       QRect(left, size.height() - bottom,
                             size.width() - right - left, bottom));
    if (left > 0) //bottom-left
      painter->drawImage(QRect(rect.left(), rect.top() + rect.height() - bottom, left, bottom), img,
                         QRect(0, size.height() - bottom, left, bottom));
    if (right > 0) //bottom-right
      painter->drawImage(QRect(rect.left() + rect.width() - right, rect.top() + rect.height() - bottom, right, bottom), img,
                         QRect(size.width() - right, size.height() - bottom, right, bottom));
  }
}

// Tints an image with tintColor, while preserving alpha and lightness
void
StyleHelper::tintImage(QImage &img,
                       QColor const &tintColor) {
  QPainter p(&img);
  p.setCompositionMode(QPainter::CompositionMode_Screen);

  for (int x = 0; x < img.width(); ++x) {
    for (int y = 0; y < img.height(); ++y) {
      QRgb rgbColor = img.pixel(x, y);
      int alpha = qAlpha(rgbColor);
      QColor c = QColor(rgbColor);

      if (alpha > 0) {
        c.toHsl();
        qreal l = c.lightnessF();
        QColor newColor = QColor::fromHslF(tintColor.hslHueF(), tintColor.hslSaturationF(), l);
        newColor.setAlpha(alpha);
        img.setPixel(x, y, newColor.rgba());
      }
    }
  }
}

int
StyleHelper::navigationWidgetHeight() {
  return 24;
}

QColor
StyleHelper::requestedBaseColor() {
  return m_requestedBaseColor;
}

QColor
StyleHelper::buttonTextColor() {
  return QColor(0x4c4c4c);
}

QColor
StyleHelper::sidebarHighlight() {
  return QColor(255, 255, 255, 40);
}

QColor
StyleHelper::sidebarShadow() {
  return QColor(0, 0, 0, 40);
}

bool
StyleHelper::usePixmapCache() {
  return true;
}

}
