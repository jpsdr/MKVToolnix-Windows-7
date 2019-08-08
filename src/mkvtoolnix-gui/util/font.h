#pragma once

#include "common/common_pch.h"

class QFont;
class QFontMetrics;
class QString;

namespace mtx { namespace gui { namespace Util {

QFont defaultUiFont();

int horizontalAdvance(QFontMetrics const &metrics, QString const &text);

}}}
