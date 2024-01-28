#include "common/common_pch.h"

#include <QDebug>
#include <QLocale>

#include <ebml/EbmlDummy.h>
#include <matroska/KaxSegment.h>

#include "common/checksums/base.h"
#include "common/ebml.h"
#include "common/kax_element_names.h"
#include "common/qt.h"
#include "mkvtoolnix-gui/info/model.h"
#include "mkvtoolnix-gui/util/kax_info.h"
#include "mkvtoolnix-gui/util/model.h"

namespace mtx::gui::Info {

class ModelPrivate {
public:
  std::unique_ptr<Util::KaxInfo> m_info;
  QVector<QStandardItem *> m_treeInsertionPosition;
};

Model::Model(QObject *parent)
  : QStandardItemModel{parent}
  , p_ptr{new ModelPrivate}
{
  setColumnCount(4);
  retranslateUi();
}

Model::~Model() {
}

void
Model::setInfo(std::unique_ptr<Util::KaxInfo> info) {
  p_func()->m_info = std::move(info);
}

Util::KaxInfo &
Model::info() {
  return *p_func()->m_info;
}

libebml::EbmlElement *
Model::elementFromIndex(QModelIndex const &idx) {
  if (!idx.isValid())
    return {};

  auto item = itemFromIndex(idx);
  if (item)
    return elementFromItem(*item);

  return {};
}

libebml::EbmlElement *
Model::elementFromItem(QStandardItem &item)
  const {
  return reinterpret_cast<libebml::EbmlElement *>(item.data(Roles::Element).toULongLong());
}

QList<QStandardItem *>
Model::newItems()
  const {
  return { new QStandardItem{}, new QStandardItem{}, new QStandardItem{}, new QStandardItem{}, new QStandardItem{} };
}

QList<QStandardItem *>
Model::itemsForRow(QModelIndex const &idx) {
  QList<QStandardItem *> items;

  for (int column = 0, numColumns = columnCount(), row = idx.row(); column < numColumns; ++column)
    items << itemFromIndex(idx.sibling(row, column));

  return items;
}

void
Model::retranslateUi() {
  auto p = p_func();

  Util::setDisplayableAndSymbolicColumnNames(*this, {
    { QY("Elements"),  Q("elements") },
    { QY("Content"),   Q("content")  },
    { QY("Position"),  Q("position") },
    { QY("Size"),      Q("size")     },
    { QY("Data size"), Q("dataSize") },
  });

  horizontalHeaderItem(2)->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
  horizontalHeaderItem(3)->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
  horizontalHeaderItem(4)->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

  if (!p->m_info)
    return;

  Util::walkTree(*this, QModelIndex{}, [this](QModelIndex const &idx) {
    auto element = elementFromIndex(idx);
    if (element) {
      auto items = itemsForRow(idx);
      setItemsFromElement(items, *element);
    }
  });
}

void
Model::setItemsFromElement(QList<QStandardItem *> &items,
                           libebml::EbmlElement &element) {
  auto p             = p_func();
  auto nameAndStatus = elementName(element);
  auto locale        = QLocale::system();
  auto content       = nameAndStatus.second ? p->m_info->format_element_value(element) : std::string{};

  items[0]->setText(nameAndStatus.first);
  items[1]->setText(Q(content));
  items[2]->setText(locale.toString(static_cast<quint64>(element.GetElementPosition())));
  items[3]->setText(element.IsFiniteSize() ? locale.toString(static_cast<quint64>(element.HeadSize() + element.GetSize())) : QY("unknown"));
  items[4]->setText(element.IsFiniteSize() ? locale.toString(static_cast<quint64>(                     element.GetSize())) : QY("unknown"));

  items[2]->setTextAlignment(Qt::AlignRight);
  items[3]->setTextAlignment(Qt::AlignRight);
  items[4]->setTextAlignment(Qt::AlignRight);

  items[0]->setData(reinterpret_cast<qulonglong>(&element),          Roles::Element);
  items[0]->setData(static_cast<qint64>(get_ebml_id(element).GetValue()), Roles::EbmlId);
}

void
Model::reset() {
  auto p = p_func();

  beginResetModel();

  removeRows(0, rowCount());
  p->m_treeInsertionPosition.clear();
  p->m_treeInsertionPosition << invisibleRootItem();

  endResetModel();
}

void
Model::addElement(int level,
                  libebml::EbmlElement *element,
                  bool readFully) {
  auto p = p_func();

  if (!element)
    return;

  auto items = newItems();
  setItemsFromElement(items, *element);

  if (!readFully && dynamic_cast<libebml::EbmlMaster *>(element)) {
    items[0]->setData(true,  Roles::DeferredLoad);
    items[0]->setData(false, Roles::Loaded);
  }

  while (p->m_treeInsertionPosition.size() > (level + 1))
    p->m_treeInsertionPosition.removeLast();

  p->m_treeInsertionPosition.last()->appendRow(items);

  p->m_treeInsertionPosition << items[0];
}

void
Model::addElementInfo(int level,
                      QString const &text,
                      std::optional<int64_t> position,
                      std::optional<int64_t> size,
                      std::optional<int64_t> dataSize) {
  auto p = p_func();

  if (p->m_treeInsertionPosition.isEmpty()) {
    qDebug() << "showElementInfo: tree insert position is empty for " << level << (position ? *position : -1) << (size ? *size : -1) << text;
    return;
  }

  auto items  = newItems();
  auto locale = QLocale::system();

  items[0]->setText(text);
  items[2]->setText(position                     ? locale.toString(static_cast<quint64>(*position)) : QY("unknown"));
  items[3]->setText(size && (*size >= 0)         ? locale.toString(static_cast<quint64>(*size))     : QY("unknown"));
  items[4]->setText(dataSize && (*dataSize >= 0) ? locale.toString(static_cast<quint64>(*dataSize)) : QY("unknown"));

  items[2]->setTextAlignment(Qt::AlignRight);
  items[3]->setTextAlignment(Qt::AlignRight);
  items[4]->setTextAlignment(Qt::AlignRight);

  if (position)
    items[0]->setData(static_cast<qint64>(*position), Roles::Position);
  if (size && (*size >= 0))
    items[0]->setData(static_cast<qint64>(*size),     Roles::Size);

  items[0]->setData(static_cast<qint64>(PseudoTypes::Unknown), Roles::PseudoType);

  p->m_treeInsertionPosition.last()->appendRow(items);
}

void
Model::addFrameInfo(libmatroska::DataBuffer &buffer,
                    int64_t position) {
  auto p = p_func();

  if (p->m_treeInsertionPosition.isEmpty()) {
    qDebug() << "addFrameInfo: tree insert position is empty for " << position << buffer.Size();
    return;
  }

  auto items  = newItems();
  auto locale = QLocale::system();

  items[0]->setText(QY("Frame"));
  items[1]->setText(Q(fmt::format(Y("Adler-32: 0x{0:08x}"), mtx::checksum::calculate_as_uint(mtx::checksum::algorithm_e::adler32, buffer.Buffer(), buffer.Size()))));
  items[2]->setText(locale.toString(static_cast<quint64>(position)));
  items[3]->setText(locale.toString(static_cast<quint64>(buffer.Size())));

  items[2]->setTextAlignment(Qt::AlignRight);
  items[3]->setTextAlignment(Qt::AlignRight);

  items[0]->setData(static_cast<qint64>(position),           Roles::Position);
  items[0]->setData(static_cast<qint64>(buffer.Size()),      Roles::Size);
  items[0]->setData(static_cast<qint64>(PseudoTypes::Frame), Roles::PseudoType);

  p->m_treeInsertionPosition.last()->appendRow(items);
}

template<typename T>
void
Model::addFrameInfoFor(T &block) {
  auto framePos = block.GetElementPosition() + block.ElementSize();
  int numFrames = block.NumberFrames();

  for (int idx = 0; idx < numFrames; ++idx)
    framePos -= block.GetBuffer(idx).Size();

  for (int idx = 0; idx < numFrames; ++idx) {
    auto &buffer = block.GetBuffer(idx);

    addFrameInfo(buffer, framePos);

    framePos += buffer.Size();
  }
}

void
Model::addElementStructure(QStandardItem &parent,
                           libebml::EbmlElement &element) {
  auto p = p_func();

  p->m_info->run_generic_pre_processors(element);

  auto items = newItems();
  setItemsFromElement(items, element);

  parent.appendRow(items);

  p->m_treeInsertionPosition << items[0];

  auto master = dynamic_cast<libebml::EbmlMaster *>(&element);
  if (master) {
    for (auto child : *master)
      addElementStructure(*items[0], *child);

  } else if (dynamic_cast<libmatroska::KaxSimpleBlock *>(&element))
    addFrameInfoFor(static_cast<libmatroska::KaxSimpleBlock &>(element));

  else if (dynamic_cast<libmatroska::KaxBlock *>(&element))
    addFrameInfoFor(static_cast<libmatroska::KaxBlock &>(element));

  p->m_info->run_generic_post_processors(element);

  p->m_treeInsertionPosition.removeLast();
}

bool
Model::hasChildren(const QModelIndex &parent)
  const {
  if (!parent.isValid())
    return QStandardItemModel::hasChildren(parent);

  auto item = itemFromIndex(parent);
  if (!item->data(Roles::DeferredLoad).toBool())
    return QStandardItemModel::hasChildren(parent);

  if (!item->data(Roles::Loaded).toBool())
    return true;

  auto element = dynamic_cast<libebml::EbmlMaster *>(elementFromItem(*item));
  return element ? !!element->ListSize() : false;
}

void
Model::forgetLevel1ElementChildren(QModelIndex const &idx) {
  if (!idx.isValid())
    return;

  auto item = itemFromIndex(idx);
  if (!item->data(Roles::DeferredLoad).toBool() || !item->data(Roles::DeferredLoad).toBool())
    return;

  item->removeRows(0, item->rowCount());
  item->setData(false, Roles::Loaded);

  auto element = dynamic_cast<libebml::EbmlMaster *>(elementFromItem(*item));
  if (!element)
    return;

  for (auto child : *element)
    delete child;
  element->RemoveAll();
}

void
Model::addChildrenOfLevel1Element(QModelIndex const &idx) {
  if (!idx.isValid())
    return;

  auto p       = p_func();
  auto element = elementFromIndex(idx);
  auto master  = dynamic_cast<libebml::EbmlMaster *>(element);
  auto parent  = itemFromIndex(idx);
  auto items   = itemsForRow(idx);

  if (element)
    setItemsFromElement(items, *element);

  if (!master)
    return;

  p->m_info->run_generic_pre_processors(*element);

  for (auto child : *master)
    addElementStructure(*parent, *child);

  p->m_info->run_generic_post_processors(*element);
}

std::pair<QString, bool>
Model::elementName(libebml::EbmlElement &element) {
  auto name = kax_element_names_c::get(element);

  if (name.empty())
    return { Q(fmt::format(Y("Unknown element (ID: 0x{0})"), kax_info_c::format_ebml_id_as_hex(element))), false };

  if (dynamic_cast<libebml::EbmlDummy *>(&element))
    return { Q(fmt::format(Y("Known element, but invalid at this position: {0} (ID: 0x{1})"), name, kax_info_c::format_ebml_id_as_hex(element))), false };

  return { Q(name), true };
}

}
