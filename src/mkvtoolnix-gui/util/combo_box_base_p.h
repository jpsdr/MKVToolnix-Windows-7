#pragma once

#include "common/common_pch.h"

#include <QString>

namespace mtx::gui::Util {

class ComboBoxBase;
class ComboBoxBasePrivate {
  friend class ComboBoxBase;

  bool m_withEmpty{};
  QString m_emptyTitle;
  QStringList m_additionalItems;

public:
  virtual ~ComboBoxBasePrivate();
};

}
