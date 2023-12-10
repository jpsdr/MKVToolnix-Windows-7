#pragma once

#include <Qt>

#include <QMetaType>
#include <QVariant>

inline bool
mtxCanConvertVariantTo(QVariant const &variant,
                       QMetaType::Type targetTypeId) {
  return QMetaType::canConvert(variant.metaType(), QMetaType(targetTypeId));
}

inline bool
mtxDoesVariantContain(QVariant const &variant,
                      QMetaType::Type typeId) {
  return variant.metaType().id() == typeId;
}

inline int
mtxMetaTypeIdForVariant(QVariant const &variant) {
  return variant.metaType().id();
}
