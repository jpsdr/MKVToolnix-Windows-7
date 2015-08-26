#ifndef MTX_MKVTOOLNIX_GUI_UTIL_COMBO_BOX_BASE_H
#define MTX_MKVTOOLNIX_GUI_UTIL_COMBO_BOX_BASE_H

#include "common/common_pch.h"

#include <QComboBox>

namespace mtx { namespace gui { namespace Util {

class ComboBoxBasePrivate;
class ComboBoxBase: public QComboBox {
  Q_OBJECT;

protected:
  Q_DECLARE_PRIVATE(ComboBoxBase);

protected:
  QScopedPointer<ComboBoxBasePrivate> d_ptr;

  explicit ComboBoxBase(ComboBoxBasePrivate &d, QWidget *parent);

public:
  explicit ComboBoxBase(QWidget *parent = nullptr);
  virtual ~ComboBoxBase();

  virtual ComboBoxBase &setup(bool withEmpty = false, QString const &emptyTitle = QString{}) = 0;

  virtual ComboBoxBase &setCurrentByData(QString const &value);
  virtual ComboBoxBase &setCurrentByData(QStringList const &values);

public slots:
  virtual void reInitialize();
};

}}}

#endif  // MTX_MKVTOOLNIX_GUI_UTIL_COMBO_BOX_BASE_H
