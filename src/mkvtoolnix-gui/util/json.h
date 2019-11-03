#pragma once

#include "common/common_pch.h"

#include "common/json.h"

class QVariant;

namespace mtx::gui::Util {

QVariant nlohmannJsonToVariant(nlohmann::json const &json);
nlohmann::json variantToNlohmannJson(QVariant const &variant);

}
