#pragma once

#include "common/common_pch.h"

#include <QComboBox>

namespace mtx { namespace gui { namespace Util {

class ComboBoxBasePrivate;
class ComboBoxBase: public QComboBox {
  Q_OBJECT;

  using StringPairVector = std::vector<std::pair<QString, QString>>;

protected:
  Q_DECLARE_PRIVATE(ComboBoxBase);

protected:
  QScopedPointer<ComboBoxBasePrivate> d_ptr;

  explicit ComboBoxBase(ComboBoxBasePrivate &d, QWidget *parent);

public:
  explicit ComboBoxBase(QWidget *parent = nullptr);
  virtual ~ComboBoxBase();

  virtual ComboBoxBase &setup(bool withEmpty = false, QString const &emptyTitle = QString{}) = 0;

  virtual ComboBoxBase &setCurrentByData(QString const &value);
  virtual ComboBoxBase &setCurrentByData(QStringList const &values);

  virtual ComboBoxBase &setAdditionalItems(QString const &item);
  virtual ComboBoxBase &setAdditionalItems(QStringList const &items);
  virtual QStringList const &additionalItems() const;

  virtual ComboBoxBase &reInitializeIfNecessary();
  virtual bool onlyShowOftenUsed() const;

public slots:
  virtual void reInitialize();

protected:
  static StringPairVector mergeCommonAndAdditionalItems(StringPairVector const &commonItems, StringPairVector const &allItems, QStringList const &additionalItems);
};

}}}
