#pragma once

#include "common/common_pch.h"

#include <QComboBox>

#include "common/qt.h"

namespace mtx::gui::Util {

class ComboBoxBasePrivate;
class ComboBoxBase: public QComboBox {
  Q_OBJECT

protected:
  MTX_DECLARE_PRIVATE(ComboBoxBasePrivate)

  std::unique_ptr<ComboBoxBasePrivate> const p_ptr;

  explicit ComboBoxBase(ComboBoxBasePrivate &p, QWidget *parent);

  using StringPairVector = std::vector<std::pair<QString, QString>>;

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

public Q_SLOTS:
  virtual void reInitialize();

protected:
  static StringPairVector mergeCommonAndAdditionalItems(StringPairVector const &commonItems, StringPairVector const &allItems, QStringList const &additionalItems);
};

}
