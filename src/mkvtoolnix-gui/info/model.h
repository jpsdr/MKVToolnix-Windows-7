#pragma once

#include "common/common_pch.h"

#include "mkvtoolnix-gui/jobs/job.h"

#include <QStandardItemModel>

namespace mtx { namespace gui {

namespace Util {
class KaxInfo;
}

namespace Info {

namespace {
constexpr int ElementRole      = Qt::UserRole + 1;
constexpr int EbmlIdRole       = Qt::UserRole + 2;
constexpr int DeferredLoadRole = Qt::UserRole + 3;
constexpr int LoadedRole       = Qt::UserRole + 4;
}

class ModelPrivate;
class Model: public QStandardItemModel {
  Q_OBJECT;

protected:
  MTX_DECLARE_PRIVATE(ModelPrivate);

  std::unique_ptr<ModelPrivate> const p_ptr;

public:
  Model(QObject *parent);
  virtual ~Model();

  void retranslateUi();

  void setInfo(std::unique_ptr<Util::KaxInfo> info);
  Util::KaxInfo &info();

  EbmlElement *elementFromIndex(QModelIndex const &idx);
  EbmlElement *elementFromItem(QStandardItem &item) const;

  QList<QStandardItem *> itemsForRow(QModelIndex const &idx);
  QList<QStandardItem *> newItems() const;
  void setItemsFromElement(QList<QStandardItem *> &items, EbmlElement &element);

  void reset();

  bool hasChildren(const QModelIndex &parent) const override;

public slots:
  void addElement(int level, EbmlElement *element, bool readFully);
  void addElementStructure(QStandardItem &parent, EbmlElement &element);

  void addChildrenOfLevel1Element(QModelIndex const &idx);
  void forgetLevel1ElementChildren(QModelIndex const &idx);
};

}}}
