#ifndef MTX_MKVTOOLNIX_GUI_UTIL_COUNTRY_COMBO_BOX_H
#define MTX_MKVTOOLNIX_GUI_UTIL_COUNTRY_COMBO_BOX_H

#include "common/common_pch.h"

#include "mkvtoolnix-gui/util/combo_box_base.h"

namespace mtx { namespace gui { namespace Util {

class CountryComboBox: public ComboBoxBase {
  Q_OBJECT;

protected:
  explicit CountryComboBox(ComboBoxBasePrivate &d, QWidget *parent);

public:
  explicit CountryComboBox(QWidget *parent = nullptr);
  virtual ~CountryComboBox();

  virtual ComboBoxBase &setup(bool withEmpty = false, QString const &emptyTitle = QString{}) override;
  virtual bool onlyShowOftenUsed() const override;
};

}}}

#endif  // MTX_MKVTOOLNIX_GUI_UTIL_COUNTRY_COMBO_BOX_H
