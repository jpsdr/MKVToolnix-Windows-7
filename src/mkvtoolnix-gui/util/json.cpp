#include "common/common_pch.h"

#include <QDebug>
#include <QVariant>

#include "common/list_utils.h"
#include "common/qt.h"
#include "common/qt6_compat/meta_type.h"
#include "mkvtoolnix-gui/util/json.h"

namespace mtx::gui::Util {

QVariant
nlohmannJsonToVariant(nlohmann::json const &json) {
  if (json.is_null())
    return {};

  if (json.is_object()) {
    auto values = QVariantMap{};
    for (auto kv = json.begin(), end = json.end(); kv != end; ++kv)
      values.insert(Q(kv.key()), nlohmannJsonToVariant(kv.value()));

    return values;
  }

  if (json.is_array()) {
    auto values = QVariantList{};
    for (auto const &val : json)
      values << nlohmannJsonToVariant(val);

    return values;
  }

  return json.is_number_integer() ? QVariant{static_cast<qulonglong>(json.get<uint64_t>())}
       : json.is_number_float()   ? QVariant{json.get<double>()}
       : json.is_boolean()        ? QVariant{json.get<bool>()}
       :                            QVariant{Q(json.get<std::string>())};
}

static nlohmann::json
logUnsupportedVariantType(QVariant const &variant) {
  qDebug() << "varianttonlohmannjson: unsupported variant type" << mtxMetaTypeIdForVariant(variant) << variant;
  throw std::domain_error{"unsupported QVariant type"};
  return {};
}

nlohmann::json
variantToNlohmannJson(QVariant const &variant) {
  if (variant.isNull() || !variant.isValid())
    return {};

  if (mtxDoesVariantContain(variant, QMetaType::QVariantMap)) {
    auto values = nlohmann::json::object();
    auto map    = variant.toMap();

    for (auto kv = map.begin(), end = map.end(); kv != end; ++kv)
      values[ to_utf8(kv.key()) ] = variantToNlohmannJson(kv.value());

    return values;
  }

  if (   mtxDoesVariantContain(variant, QMetaType::QStringList)
      || mtxDoesVariantContain(variant, QMetaType::QVariantList)) {
    auto values = nlohmann::json::array();

    for (auto const &value : variant.toList())
      values.push_back(variantToNlohmannJson(value));

    return values;
  }

  return mtxDoesVariantContain(variant, QMetaType::Bool)       ? nlohmann::json(variant.toBool())
       : mtxDoesVariantContain(variant, QMetaType::QString)    ? nlohmann::json(to_utf8(variant.toString()))
       : mtxDoesVariantContain(variant, QMetaType::Double)     ? nlohmann::json(variant.toDouble())
       : mtxDoesVariantContain(variant, QMetaType::Float)      ? nlohmann::json(variant.toDouble())
       : mtxCanConvertVariantTo(variant, QMetaType::ULongLong) ? nlohmann::json(variant.toULongLong())
       : mtxCanConvertVariantTo(variant, QMetaType::QString)   ? nlohmann::json(to_utf8(variant.toString()))
       :                                                         logUnsupportedVariantType(variant);
}

}
