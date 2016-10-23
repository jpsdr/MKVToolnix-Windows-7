#ifndef MTX_MKVTOOLNIX_GUI_UTIL_COMBO_BOX_BASE_P_H
#define MTX_MKVTOOLNIX_GUI_UTIL_COMBO_BOX_BASE_P_H

#include "common/common_pch.h"

#include <QString>

namespace mtx { namespace gui { namespace Util {

class ComboBoxBase;
class ComboBoxBasePrivate {
  friend class ComboBoxBase;

  bool m_withEmpty{};
  QString m_emptyTitle;
  QStringList m_additionalItems;

public:
  virtual ~ComboBoxBasePrivate();
};

}}}

#endif  // MTX_MKVTOOLNIX_GUI_UTIL_COMBO_BOX_BASE_P_H
