#include "common/common_pch.h"

#include <QDebug>
#include <QFont>

#include <windows.h>

#include "common/qt.h"
#include "mkvtoolnix-gui/app.h"
#include "mkvtoolnix-gui/util/font.h"

namespace mtx::gui::Util {

namespace {
bool s_logFontDebugLogged = false;
}

QFont
logFontToQFont(LOGFONTW const &logFont) {
  QFont font;

  font.setFamily(QString::fromWCharArray(logFont.lfFaceName));
  font.setItalic(logFont.lfItalic);
  font.setUnderline(logFont.lfUnderline);
  font.setStrikeOut(logFont.lfStrikeOut);

  if (logFont.lfWeight != FW_DONTCARE) {
    auto weight = logFont.lfWeight < 400 ? QFont::Light
                : logFont.lfWeight < 600 ? QFont::Normal
                : logFont.lfWeight < 700 ? QFont::DemiBold
                : logFont.lfWeight < 800 ? QFont::Bold
                :                          QFont::Black;
    font.setWeight(weight);
  }

  auto verticalDPI = GetDeviceCaps(GetDC(0), LOGPIXELSY);
  font.setPointSizeF(qAbs(logFont.lfHeight) * 72.0 / verticalDPI);

  if (!s_logFontDebugLogged) {
    qDebug() << Q("logFontToQFont family %1 weight %2 (%3) height %4 (%5 DPI %6) italic %7 underline %8 strickeout %9")
                .arg(font.family()).arg(logFont.lfWeight).arg(font.weight()).arg(logFont.lfHeight).arg(font.pointSizeF()).arg(verticalDPI).arg(font.italic()).arg(font.underline()).arg(font.strikeOut());
    s_logFontDebugLogged = true;
  }

  return font;
}

QFont
defaultUiFont() {
  static std::optional<QFont> s_font;
  if (s_font)
    return *s_font;

  try {
    NONCLIENTMETRICSW nonClientMetrics;

    auto size = sizeof(nonClientMetrics);
    memset(&nonClientMetrics, 0, size);
    nonClientMetrics.cbSize = size;

    if (!SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, size, &nonClientMetrics, 0))
      throw false;

    s_font.reset(logFontToQFont(nonClientMetrics.lfMessageFont));

  } catch (bool) {
    qDebug() << "Windows default font query failed; returning application font";
    s_font.reset(App::font());
  }

  return *s_font;
}

}
