#ifndef MTX_MKVTOOLNIX_GUI_UTIL_COMBO_BOX_BASE_H
#define MTX_MKVTOOLNIX_GUI_UTIL_COMBO_BOX_BASE_H

#include "common/common_pch.h"

#include <QComboBox>

namespace mtx { namespace gui { namespace Util {

class ComboBoxBase: public QComboBox {
  Q_OBJECT;

protected:
  bool m_withEmpty{};
  QString m_emptyTitle;

public:
  explicit ComboBoxBase(QWidget *parent = nullptr);
  virtual ~ComboBoxBase();

  virtual ComboBoxBase &setup(bool withEmpty = false, QString const &emptyTitle = QString{}) = 0;

  virtual ComboBoxBase &setCurrentByData(QString const &value);
  virtual ComboBoxBase &setCurrentByData(QStringList const &values);
};

}}}

#endif  // MTX_MKVTOOLNIX_GUI_UTIL_COMBO_BOX_BASE_H
