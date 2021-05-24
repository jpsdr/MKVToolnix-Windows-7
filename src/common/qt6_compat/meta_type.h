#pragma once

#include <Qt>

#include <QMetaType>
#include <QVariant>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)

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

#else  // Qt >= 6

inline bool
mtxCanConvertVariantTo(QVariant const &variant,
                       QMetaType::Type targetTypeId) {
  return variant.canConvert(targetTypeId);
}

inline bool
mtxDoesVariantContain(QVariant const &variant,
                      QMetaType::Type typeId) {
  return static_cast<QMetaType::Type>(variant.type()) == typeId;
}

inline int
mtxMetaTypeIdForVariant(QVariant const &variant) {
  return static_cast<int>(variant.type());
}

#endif // Qt >= 6
