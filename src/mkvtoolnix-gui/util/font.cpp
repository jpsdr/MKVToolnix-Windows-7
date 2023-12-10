#include "common/common_pch.h"

#include <QFontMetrics>
#include <QString>

#include "mkvtoolnix-gui/util/font.h"

namespace mtx::gui::Util {

int
horizontalAdvance(QFontMetrics const &metrics,
                  QString const &text) {
  return metrics.horizontalAdvance(text);
}

}
