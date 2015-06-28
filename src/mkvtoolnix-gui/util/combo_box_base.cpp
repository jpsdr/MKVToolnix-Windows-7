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

void
ComboBoxBase::reInitialize() {
  auto previousBlocked = blockSignals(true);
  auto data            = currentData();
  auto firstText       = itemText(0);
  auto firstData       = itemData(0);

  clear();
  setup(m_withEmpty, m_emptyTitle);

  if (!firstData.isValid())
    insertItem(0, firstText, firstData);

  if (!data.isValid())
    setCurrentIndex(0);
  else
    setCurrentByData(data.toString());

  blockSignals(previousBlocked);
}

}}}
