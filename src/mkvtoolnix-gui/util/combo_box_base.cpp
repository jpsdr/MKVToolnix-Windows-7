#include "common/common_pch.h"

#include <QStringList>

#include "mkvtoolnix-gui/util/combo_box_base.h"
#include "mkvtoolnix-gui/util/combo_box_base_p.h"
#include "mkvtoolnix-gui/util/widget.h"

namespace mtx::gui::Util {

ComboBoxBasePrivate::~ComboBoxBasePrivate() {
}

ComboBoxBase::ComboBoxBase(QWidget *parent)
  : QComboBox{parent}
  , p_ptr{new ComboBoxBasePrivate}
{
}

ComboBoxBase::ComboBoxBase(ComboBoxBasePrivate &p,
                           QWidget *parent)
  : QComboBox{parent}
  , p_ptr{&p}
{
}

ComboBoxBase::~ComboBoxBase() {
}

ComboBoxBase &
ComboBoxBase::setup(bool withEmpty,
                    QString const &emptyTitle) {
  auto p          = p_func();
  p->m_withEmpty  = withEmpty;
  p->m_emptyTitle = emptyTitle;

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

ComboBoxBase &
ComboBoxBase::setAdditionalItems(QString const &item) {
  return setAdditionalItems(QStringList{} << item);
}

ComboBoxBase &
ComboBoxBase::setAdditionalItems(QStringList const &items) {
  auto p = p_func();

  p->m_additionalItems.clear();
  for (auto const &item : items)
    if (!item.isEmpty())
      p->m_additionalItems << item;

  return *this;
}

QStringList const &
ComboBoxBase::additionalItems()
  const {
  auto p = p_func();

  return p->m_additionalItems;
}

void
ComboBoxBase::reInitialize() {
  auto p                  = p_func();
  auto previousBlocked    = blockSignals(true);
  auto data               = currentData();
  auto firstText          = itemText(0);
  auto firstData          = itemData(0);
  auto additionalModified = false;

  if (data.isValid() && !p->m_additionalItems.contains(data.toString())) {
    p->m_additionalItems << data.toString();
    additionalModified = true;
  }

  clear();
  setup(p->m_withEmpty, p->m_emptyTitle);

  if (!firstData.isValid())
    insertItem(0, firstText, firstData);

  if (!data.isValid())
    setCurrentIndex(0);
  else
    setCurrentByData(data.toString());

  if (additionalModified)
    p->m_additionalItems.removeLast();

  blockSignals(previousBlocked);
}

ComboBoxBase &
ComboBoxBase::reInitializeIfNecessary() {
  if (onlyShowOftenUsed())
    reInitialize();
  return *this;
}

bool
ComboBoxBase::onlyShowOftenUsed()
  const {
  return false;
}

ComboBoxBase::StringPairVector
ComboBoxBase::mergeCommonAndAdditionalItems(StringPairVector const &commonItems,
                                            StringPairVector const &allItems,
                                            QStringList const &additionalItems) {
  auto items = commonItems;

  items.reserve(commonItems.size() + additionalItems.size());

  for (auto const &additionalItem : additionalItems) {
    if (additionalItems.isEmpty())
      continue;

    auto finder = [&additionalItem](std::pair<QString, QString> const &existingItem) {
      return existingItem.second == additionalItem;
    };

    auto itr = brng::find_if(commonItems, finder);
    if (itr != commonItems.end())
      continue;

    itr = brng::find_if(allItems, finder);
    if (itr != allItems.end())
      items.push_back(*itr);
  }

  brng::sort(items);

  return items;
}

}
