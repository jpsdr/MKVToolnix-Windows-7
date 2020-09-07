#pragma once

#include "common/common_pch.h"

#include "mkvtoolnix-gui/util/combo_box_base.h"

namespace mtx::gui::Util {

class CountryComboBox: public ComboBoxBase {
  Q_OBJECT

protected:
  bool m_withAlphaCodesOnly{};

  explicit CountryComboBox(ComboBoxBasePrivate &d, QWidget *parent);

public:
  explicit CountryComboBox(QWidget *parent = nullptr);
  virtual ~CountryComboBox();

  virtual ComboBoxBase &setWithAlphaCodesOnly(bool enable);

  virtual ComboBoxBase &setup(bool withEmpty = false, QString const &emptyTitle = QString{}) override;
  virtual bool onlyShowOftenUsed() const override;
};

}
