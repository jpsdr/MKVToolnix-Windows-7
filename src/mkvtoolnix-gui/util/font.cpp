#include "common/common_pch.h"

#include <QFontMetrics>
#include <QString>

#include "mkvtoolnix-gui/util/font.h"

namespace mtx::gui::Util {

int
horizontalAdvance(QFontMetrics const &metrics,
                  QString const &text) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
  return metrics.horizontalAdvance(text);
#else
  return metrics.width(text);
#endif
}

}
