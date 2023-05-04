// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

// The code in this file is adopted from Qt's source code file
// qtbase/src/plugins/platforms/windows/qwindowstheme.cpp

#include "common/common_pch.h"

#include <QColor>
#include <QPalette>

#include <windows.h>

#include "mkvtoolnix-gui/app.h"

namespace mtx::gui {

static inline QColor
mixColors(const QColor &c1,
          const QColor &c2) {
  return {
    (c1.red() + c2.red()) / 2,
    (c1.green() + c2.green()) / 2,
    (c1.blue() + c2.blue()) / 2,
  };
}

static inline QColor
getSysColor(int index) {
  COLORREF cr = GetSysColor(index);
  return QColor(GetRValue(cr), GetGValue(cr), GetBValue(cr));
}

// from QStyle::standardPalette
static inline QPalette
standardPalette() {
  QColor backgroundColor(0xd4, 0xd0, 0xc8); // win 2000 grey
  QColor lightColor(backgroundColor.lighter());
  QColor darkColor(backgroundColor.darker());
  const QBrush darkBrush(darkColor);
  QColor midColor(Qt::gray);
  QPalette palette(Qt::black, backgroundColor, lightColor, darkColor, midColor, Qt::black, Qt::white);
  palette.setBrush(QPalette::Disabled, QPalette::WindowText, darkBrush);
  palette.setBrush(QPalette::Disabled, QPalette::Text, darkBrush);
  palette.setBrush(QPalette::Disabled, QPalette::ButtonText, darkBrush);
  palette.setBrush(QPalette::Disabled, QPalette::Base, QBrush(backgroundColor));
  return palette;
}

static QColor
placeHolderColor(QColor textColor) {
  textColor.setAlpha(128);
  return textColor;
}

/*
  This is used when the theme is light mode, and when the theme is dark but the
  application doesn't support dark mode. In the latter case, we need to check.
*/
static void
populateLightSystemBasePalette(QPalette &result) {
  QColor background = getSysColor(COLOR_BTNFACE);
  QColor textColor  = getSysColor(COLOR_WINDOWTEXT);
  QColor accent     = getSysColor(COLOR_HIGHLIGHT);

  const QColor btnFace      = background;
  const QColor btnHighlight = getSysColor(COLOR_BTNHIGHLIGHT);

  result.setColor(QPalette::Highlight, accent);
  result.setColor(QPalette::WindowText, getSysColor(COLOR_WINDOWTEXT));
  result.setColor(QPalette::Button, btnFace);
  result.setColor(QPalette::Light, btnHighlight);
  result.setColor(QPalette::Dark, getSysColor(COLOR_BTNSHADOW));
  result.setColor(QPalette::Mid, result.button().color().darker(150));
  result.setColor(QPalette::Text, textColor);
  result.setColor(QPalette::PlaceholderText, placeHolderColor(textColor));
  result.setColor(QPalette::BrightText, btnHighlight);
  result.setColor(QPalette::Base, getSysColor(COLOR_WINDOW));
  result.setColor(QPalette::Window, btnFace);
  result.setColor(QPalette::ButtonText, getSysColor(COLOR_BTNTEXT));
  result.setColor(QPalette::Midlight, getSysColor(COLOR_3DLIGHT));
  result.setColor(QPalette::Shadow, getSysColor(COLOR_3DDKSHADOW));
  result.setColor(QPalette::HighlightedText, getSysColor(COLOR_HIGHLIGHTTEXT));

  result.setColor(QPalette::Link, Qt::blue);
  result.setColor(QPalette::LinkVisited, Qt::magenta);
  result.setColor(QPalette::Inactive, QPalette::Button, result.button().color());
  result.setColor(QPalette::Inactive, QPalette::Window, result.window().color());
  result.setColor(QPalette::Inactive, QPalette::Light, result.light().color());
  result.setColor(QPalette::Inactive, QPalette::Dark, result.dark().color());

  if (result.midlight() == result.button())
    result.setColor(QPalette::Midlight, result.button().color().lighter(110));
}

static void
populateDarkSystemBasePalette(QPalette &result) {
  const QColor foreground     = Qt::white;
  const QColor background     = QColor(0x1E, 0x1E, 0x1E);
  const QColor accent         = QColor(0x00, 0x55, 0xff);
  const QColor accentDark     = accent.darker(120);
  const QColor accentDarker   = accentDark.darker(120);
  const QColor accentDarkest  = accentDarker.darker(120);
  const QColor accentLight    = accent.lighter(120);
  const QColor accentLighter  = accentLight.lighter(120);
  const QColor accentLightest = accentLighter.lighter(120);
  const QColor linkColor      = QColor(29, 153, 243);
  const QColor buttonColor    = background.lighter(200);

  result.setColor(QPalette::All, QPalette::WindowText, foreground);
  result.setColor(QPalette::All, QPalette::Text, foreground);
  result.setColor(QPalette::All, QPalette::BrightText, accentLightest);

  result.setColor(QPalette::All, QPalette::Button, buttonColor);
  result.setColor(QPalette::All, QPalette::ButtonText, foreground);
  result.setColor(QPalette::All, QPalette::Light, buttonColor.lighter(200));
  result.setColor(QPalette::All, QPalette::Midlight, buttonColor.lighter(150));
  result.setColor(QPalette::All, QPalette::Dark, buttonColor.darker(200));
  result.setColor(QPalette::All, QPalette::Mid, buttonColor.darker(150));
  result.setColor(QPalette::All, QPalette::Shadow, Qt::black);

  result.setColor(QPalette::All, QPalette::Base, background.lighter(150));
  result.setColor(QPalette::All, QPalette::Window, background);

  result.setColor(QPalette::All, QPalette::Highlight, accent);
  result.setColor(QPalette::All, QPalette::HighlightedText, accent.lightness() > 128 ? Qt::black : Qt::white);
  result.setColor(QPalette::All, QPalette::Link, linkColor);
  result.setColor(QPalette::All, QPalette::LinkVisited, accentDarkest);
  result.setColor(QPalette::All, QPalette::AlternateBase, accentDarkest);
  result.setColor(QPalette::All, QPalette::ToolTipBase, buttonColor);
  result.setColor(QPalette::All, QPalette::ToolTipText, foreground.darker(120));
  result.setColor(QPalette::All, QPalette::PlaceholderText, placeHolderColor(foreground));
}

QPalette
App::systemPalette(bool light) {
  QPalette result = standardPalette();
  if (light)
    populateLightSystemBasePalette(result);
  else
    populateDarkSystemBasePalette(result);

  if (result.window() != result.base()) {
    result.setColor(QPalette::Inactive, QPalette::Highlight,       result.color(QPalette::Inactive, QPalette::Window));
    result.setColor(QPalette::Inactive, QPalette::HighlightedText, result.color(QPalette::Inactive, QPalette::Text));
  }

  const QColor disabled = mixColors(result.windowText().color(), result.button().color());

  result.setColorGroup(QPalette::Disabled, result.windowText(), result.button(),
                       result.light(), result.dark(), result.mid(),
                       result.text(), result.brightText(), result.base(),
                       result.window());
  result.setColor(QPalette::Disabled, QPalette::WindowText, disabled);
  result.setColor(QPalette::Disabled, QPalette::Text, disabled);
  result.setColor(QPalette::Disabled, QPalette::ButtonText, disabled);
  result.setColor(QPalette::Disabled, QPalette::Highlight, result.color(QPalette::Highlight));
  result.setColor(QPalette::Disabled, QPalette::HighlightedText, result.color(QPalette::HighlightedText));
  result.setColor(QPalette::Disabled, QPalette::Base, result.window().color());
  return result;
}

}
