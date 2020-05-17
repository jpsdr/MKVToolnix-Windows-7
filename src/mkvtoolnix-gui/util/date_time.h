#pragma once

#include "common/common_pch.h"

class QString;

namespace mtx::gui::Util {

QString displayableDate(QDateTime const &date);
QString timeZoneOffsetString(QDateTime const &date);

}
