#include "common/common_pch.h"

#include <QDebug>

#include "mkvtoolnix-gui/util/recently_used_strings.h"

namespace mtx::gui::Util {

class RecentlyUsedStringsPrivate {
  friend class RecentlyUsedStrings;

  int m_numItems{};
  QStringList m_items;

  explicit RecentlyUsedStringsPrivate(int numItems)
    : m_numItems{numItems}
  {
  }
};

RecentlyUsedStrings::RecentlyUsedStrings(int numItems)
  : p_ptr{new RecentlyUsedStringsPrivate{numItems}}
{
}

RecentlyUsedStrings::~RecentlyUsedStrings() {
}

QStringList
RecentlyUsedStrings::items()
  const {
  return p_func()->m_items;
}

void
RecentlyUsedStrings::setMaximumNumItems(int numItems) {
  p_func()->m_numItems = numItems;
  clamp();
}

void
RecentlyUsedStrings::setItems(QStringList const &newItems) {
  p_func()->m_items = newItems;
  clamp();
}

void
RecentlyUsedStrings::add(QString const &item) {
  remove(item);
  p_func()->m_items.prepend(item);
  clamp();
}

void
RecentlyUsedStrings::remove(QString const &item) {
  p_func()->m_items.removeAll(item);
}

void
RecentlyUsedStrings::clamp() {
  auto p    = p_func();
  auto size = p->m_items.size();

  if (size > p->m_numItems)
    p->m_items.erase(p->m_items.begin() + p->m_numItems, p->m_items.end());
}

bool
RecentlyUsedStrings::isEmpty()
  const {
  return p_func()->m_items.isEmpty();
}

int
RecentlyUsedStrings::maximumNumItems()
  const {
  return p_func()->m_numItems;
}

}
