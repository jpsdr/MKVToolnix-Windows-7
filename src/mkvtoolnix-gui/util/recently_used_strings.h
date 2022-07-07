#pragma once

#include "common/common_pch.h"

#include <QStringList>

namespace mtx::gui::Util {

class RecentlyUsedStringsPrivate;
class RecentlyUsedStrings {
protected:
  MTX_DECLARE_PRIVATE(RecentlyUsedStringsPrivate)

  std::unique_ptr<RecentlyUsedStringsPrivate> const p_ptr;

  explicit RecentlyUsedStrings(RecentlyUsedStringsPrivate &p);

public:
  RecentlyUsedStrings(int numEntries);
  virtual ~RecentlyUsedStrings();

  bool isEmpty() const;

  int maximumNumItems() const;
  void setMaximumNumItems(int numItems);
  QStringList items() const;
  void setItems(QStringList const &newItems);

  void add(QString const &entry);
  void remove(QString const &entry);

protected:
  void clamp();
};

}
