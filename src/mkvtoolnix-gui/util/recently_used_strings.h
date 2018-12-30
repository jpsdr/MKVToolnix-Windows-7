#pragma once

#include "common/common_pch.h"

#include <QStringList>

namespace mtx { namespace gui { namespace Util {

class RecentlyUsedStringsPrivate;
class RecentlyUsedStrings {
protected:
  MTX_DECLARE_PRIVATE(RecentlyUsedStringsPrivate);

  std::unique_ptr<RecentlyUsedStringsPrivate> const p_ptr;

  explicit RecentlyUsedStrings(RecentlyUsedStringsPrivate &p);

public:
  RecentlyUsedStrings(int numEntries);
  virtual ~RecentlyUsedStrings();

  QStringList items() const;
  void setItems(QStringList const &newItems);

  void add(QString const &entry);
  void remove(QString const &entry);

protected:
  void clamp();
};

}}}
