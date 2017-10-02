/**************************************************************************
**
** This file is adapted from stylehelper.h which is part of Qt Creator
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

#pragma once

#include <QtGui/QColor>
#include <QtWidgets/QStyle>

class QPalette;
class QPainter;
class QRect;

namespace mtx { namespace gui { namespace Util {

// Helper class holding all custom color values

class StyleHelper {
private:
  static QColor m_baseColor;
  static QColor m_requestedBaseColor;

public:
  static const unsigned int DEFAULT_BASE_COLOR = 0x666666;

  // Height of the project explorer navigation bar
  static int navigationWidgetHeight();
  static qreal sidebarFontSize();
  static QPalette sidebarFontPalette(QPalette const &original);

  // This is our color table, all colors derive from baseColor
  static QColor requestedBaseColor();
  static QColor baseColor(bool lightColored = false);
  static QColor panelTextColor(bool lightColored = false);
  static QColor highlightColor(bool lightColored = false);
  static QColor shadowColor(bool lightColored = false);
  static QColor borderColor(bool lightColored = false);
  static QColor buttonTextColor();
  static QColor mergedColors(QColor const &colorA, QColor const &colorB, int factor = 50);

  static QColor sidebarHighlight();
  static QColor sidebarShadow();

  // Sets the base color and makes sure all top level widgets are updated
  static void setBaseColor(QColor const &color);

  // Draws a shaded anti-aliased arrow
  static void drawArrow(QStyle::PrimitiveElement element, QPainter *painter, QStyleOption const *option);

  // Gradients used for panels
  static void horizontalGradient(QPainter *painter, QRect const &spanRect, QRect const &clipRect, bool lightColored = false);
  static void verticalGradient(QPainter *painter, QRect const &spanRect, QRect const &clipRect, bool lightColored = false);
  static void menuGradient(QPainter *painter, QRect const &spanRect, QRect const &clipRect);
  static bool usePixmapCache();

  static void drawIconWithShadow(QIcon const &icon, QRect const &rect, QPainter *p, QIcon::Mode iconMode, int radius = 3, QColor const &color = QColor(0, 0, 0, 130), QPoint const &offset = QPoint(1, -2));
  static void drawCornerImage(QImage const &img, QPainter *painter, QRect rect, int left = 0, int top = 0, int right = 0, int bottom = 0);

  static void tintImage(QImage &img, QColor const &tintColor);
};

}}}
