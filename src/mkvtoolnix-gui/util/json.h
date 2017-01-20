#ifndef MTX_MKVTOOLNIX_GUI_UTIL_JSON_H
#define MTX_MKVTOOLNIX_GUI_UTIL_JSON_H

#include "common/common_pch.h"

#include "common/json.h"

class QVariant;

namespace mtx { namespace gui { namespace Util {

QVariant nlohmannJsonToVariant(nlohmann::json const &json);
nlohmann::json variantToNlohmannJson(QVariant const &variant);

}}}

#endif  // MTX_MKVTOOLNIX_GUI_UTIL_JSON_H
