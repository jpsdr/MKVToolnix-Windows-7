#include "common/common_pch.h"

#include <QStringList>

#include "mkvtoolnix-gui/util/combo_box_base.h"
#include "mkvtoolnix-gui/util/combo_box_base_p.h"
#include "mkvtoolnix-gui/util/widget.h"

namespace mtx { namespace gui { namespace Util {

ComboBoxBasePrivate::~ComboBoxBasePrivate() {
}

ComboBoxBase::ComboBoxBase(QWidget *parent)
  : QComboBox{parent}
  , d_ptr{new ComboBoxBasePrivate}
{
}

ComboBoxBase::ComboBoxBase(ComboBoxBasePrivate &d,
                           QWidget *parent)
  : QComboBox{parent}
  , d_ptr{&d}
{
}

ComboBoxBase::~ComboBoxBase() {
}

ComboBoxBase &
ComboBoxBase::setup(bool withEmpty,
                    QString const &emptyTitle) {
  Q_D(ComboBoxBase);

  d->m_withEmpty  = withEmpty;
  d->m_emptyTitle = emptyTitle;

  return *this;
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
  Q_D(ComboBoxBase);

  auto previousBlocked = blockSignals(true);
  auto data            = currentData();
  auto firstText       = itemText(0);
  auto firstData       = itemData(0);

  clear();
  setup(d->m_withEmpty, d->m_emptyTitle);

  if (!firstData.isValid())
    insertItem(0, firstText, firstData);

  if (!data.isValid())
    setCurrentIndex(0);
  else
    setCurrentByData(data.toString());

  blockSignals(previousBlocked);
}

}}}
