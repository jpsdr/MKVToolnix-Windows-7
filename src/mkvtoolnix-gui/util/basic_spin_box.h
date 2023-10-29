#pragma once

#include "common/common_pch.h"

#include <QSpinBox>

#include "common/qt.h"

namespace mtx::gui::Util {

class BasicSpinBox : public QSpinBox {
  Q_OBJECT

public:
  BasicSpinBox(QWidget *parent);
  virtual ~BasicSpinBox();

protected:
  virtual QString textFromValue(int value) const override;
};

}
