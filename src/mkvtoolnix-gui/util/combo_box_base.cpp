#include "common/common_pch.h"

#include <QStringList>

#include "mkvtoolnix-gui/util/combo_box_base.h"
#include "mkvtoolnix-gui/util/util.h"

namespace mtx { namespace gui { namespace Util {

ComboBoxBase::ComboBoxBase(QWidget *parent)
  : QComboBox{parent}
{
}

ComboBoxBase::~ComboBoxBase() {
}

ComboBoxBase &
ComboBoxBase::setCurrentByData(QString const &value) {
  setCurrentByData(QStringList{} << value);

  return *this;
}

ComboBoxBase &
ComboBoxBase::setCurrentByData(QStringList const &values) {
  if (!count())
    setup();

  for (auto const &value : values)
    if (setComboBoxTextByData(this, value))
      return *this;

  return *this;
}

}}}
