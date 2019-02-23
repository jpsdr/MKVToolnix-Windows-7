#pragma once

#include "common/common_pch.h"

#include "mkvtoolnix-gui/jobs/job.h"

#include <QStandardItemModel>

namespace libmatroska {
class DataBuffer;
class KaxBlock;
class KaxSimpleBlock;
}

namespace mtx { namespace gui {

namespace Util {
class KaxInfo;
}

namespace Info {

namespace Roles {
int constexpr Element      = Qt::UserRole + 1;
int constexpr EbmlId       = Qt::UserRole + 2;
int constexpr DeferredLoad = Qt::UserRole + 3;
int constexpr Loaded       = Qt::UserRole + 4;
int constexpr Position     = Qt::UserRole + 5;
int constexpr Size         = Qt::UserRole + 6;
int constexpr PseudoType   = Qt::UserRole + 7;
}

namespace PseudoTypes {
uint32_t constexpr Unknown = 0xff000000u;
uint32_t constexpr Frame   = 0xff000001u;
}

class ModelPrivate;
class Model: public QStandardItemModel {
  Q_OBJECT

protected:
  MTX_DECLARE_PRIVATE(ModelPrivate)

  std::unique_ptr<ModelPrivate> const p_ptr;

public:
  Model(QObject *parent);
  virtual ~Model();

  void retranslateUi();

  void setInfo(std::unique_ptr<Util::KaxInfo> info);
  Util::KaxInfo &info();

  libebml::EbmlElement *elementFromIndex(QModelIndex const &idx);
  libebml::EbmlElement *elementFromItem(QStandardItem &item) const;

  QList<QStandardItem *> itemsForRow(QModelIndex const &idx);
  QList<QStandardItem *> newItems() const;
  void setItemsFromElement(QList<QStandardItem *> &items, libebml::EbmlElement &element);

  void reset();

  bool hasChildren(const QModelIndex &parent) const override;
  std::pair<QString, bool> elementName(libebml::EbmlElement &element);

public slots:
  void addElement(int level, libebml::EbmlElement *element, bool readFully);
  void addElementInfo(int level, QString const &text, boost::optional<int64_t> position, boost::optional<int64_t> size);
  void addElementStructure(QStandardItem &parent, libebml::EbmlElement &element);

  void addChildrenOfLevel1Element(QModelIndex const &idx);
  void forgetLevel1ElementChildren(QModelIndex const &idx);

protected:
  template<typename T> void addFrameInfoFor(T &block);
  // void addFrameInfoFor(libmatroska::KaxSimpleBlock &block);
  void addFrameInfo(libmatroska::DataBuffer &buffer, int64_t position);
};

}}}
