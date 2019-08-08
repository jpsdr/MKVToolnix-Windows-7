#pragma once

#include "common/common_pch.h"

class QFontMetrics;
class QString;

namespace mtx { namespace gui { namespace Util {

int horizontalAdvance(QFontMetrics const &metrics, QString const &text);

}}}
