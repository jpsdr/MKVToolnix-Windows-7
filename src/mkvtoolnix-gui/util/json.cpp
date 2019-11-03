#include "common/common_pch.h"

#include <QDebug>
#include <QVariant>

#include "common/list_utils.h"
#include "common/qt.h"
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
  qDebug() << "varianttonlohmannjson: unsupported variant type" << variant.type() << variant;
  throw std::domain_error{"unsupported QVariant type"};
  return {};
}

nlohmann::json
variantToNlohmannJson(QVariant const &variant) {
  if (variant.isNull() || !variant.isValid())
    return {};

  auto const type = static_cast<QMetaType::Type>(variant.type());

  if (type == QMetaType::QVariantMap) {
    auto values = nlohmann::json::object();
    auto map    = variant.toMap();

    for (auto kv = map.begin(), end = map.end(); kv != end; ++kv)
      values[ to_utf8(kv.key()) ] = variantToNlohmannJson(kv.value());

    return values;
  }

  if (mtx::included_in(type, QMetaType::QStringList, QMetaType::QVariantList)) {
    auto values = nlohmann::json::array();

    for (auto const &value : variant.toList())
      values.push_back(variantToNlohmannJson(value));

    return values;
  }

  return type == QMetaType::Bool                                     ? nlohmann::json(variant.toBool())
       : type == QMetaType::QString                                  ? nlohmann::json(to_utf8(variant.toString()))
       : mtx::included_in(type, QMetaType::Double, QMetaType::Float) ? nlohmann::json(variant.toDouble())
       : variant.canConvert(QMetaType::ULongLong)                    ? nlohmann::json(variant.toULongLong())
       : variant.canConvert(QMetaType::QString)                      ? nlohmann::json(to_utf8(variant.toString()))
       :                                                               logUnsupportedVariantType(variant);
}

}
