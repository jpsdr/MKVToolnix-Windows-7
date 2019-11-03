#pragma once

#include "common/common_pch.h"

class QFont;
class QFontMetrics;
class QString;

namespace mtx::gui::Util {

QFont defaultUiFont();

int horizontalAdvance(QFontMetrics const &metrics, QString const &text);

}
