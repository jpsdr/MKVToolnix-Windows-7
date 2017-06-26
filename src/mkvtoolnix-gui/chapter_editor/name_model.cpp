#include "common/common_pch.h"

#include "common/ebml.h"
#include "common/qt.h"
#include "mkvtoolnix-gui/app.h"
#include "mkvtoolnix-gui/chapter_editor/name_model.h"
#include "mkvtoolnix-gui/util/model.h"
#include "mkvtoolnix-gui/util/settings.h"

namespace mtx { namespace gui { namespace ChapterEditor {

using namespace mtx::gui;

NameModel::NameModel(QObject *parent)
  : QStandardItemModel{parent}
{
}

NameModel::~NameModel() {
}

void
NameModel::retranslateUi() {
  Util::setDisplayableAndSymbolicColumnNames(*this, {
    { QY("Name"),     Q("name")     },
    { QY("Language"), Q("language") },
    { QY("Country"),  Q("country")  },
  });
}

QList<QStandardItem *>
NameModel::newRowItems() {
  return QList<QStandardItem *>{} << new QStandardItem{} << new QStandardItem{} << new QStandardItem{};
}

KaxChapterDisplay *
NameModel::displayFromItem(QStandardItem *item) {
  return m_displayRegistry[ registryIdFromItem(item) ];
}

KaxChapterDisplay *
NameModel::displayFromIndex(QModelIndex const &idx) {
  return displayFromItem(itemFromIndex(idx));
}

void
NameModel::setRowText(QList<QStandardItem *> const &rowItems) {
  auto &display = *displayFromItem(rowItems[0]);

  rowItems[0]->setText(Q(GetChildValue<KaxChapterString>(display)));
  rowItems[1]->setText(App::descriptionFromIso639_2LanguageCode(Q(FindChildValue<KaxChapterLanguage>(display, std::string{"eng"}))));
  rowItems[2]->setText(App::descriptionFromTopLevelDomainCountryCode(Q(FindChildValue<KaxChapterCountry>(display, std::string{""}))));
}

QList<QStandardItem *>
NameModel::itemsForRow(int row) {
  auto rowItems = QList<QStandardItem *>{};
  for (auto column = 0; 3 > column; ++column)
    rowItems << item(row, column);

  return rowItems;
}

void
NameModel::updateRow(int row) {
  setRowText(itemsForRow(row));
}

void
NameModel::append(KaxChapterDisplay &display) {
  auto rowItems = newRowItems();
  rowItems[0]->setData(registerDisplay(display), Util::ChapterEditorChapterDisplayRole);

  setRowText(rowItems);
  appendRow(rowItems);
}

void
NameModel::addNew() {
  auto &cfg     = Util::Settings::get();
  auto display = new KaxChapterDisplay;

  GetChild<KaxChapterString>(display).SetValueUTF8(Y("<Unnamed>"));
  GetChild<KaxChapterLanguage>(display).SetValue(to_utf8(cfg.m_defaultChapterLanguage));
  GetChild<KaxChapterCountry>(display).SetValue(to_utf8(cfg.m_defaultChapterCountry));

  m_chapter->PushElement(*display);
  append(*display);
}

void
NameModel::remove(QModelIndex const &idx) {
  auto displayItem = itemFromIndex(idx);
  auto display     = displayFromItem(displayItem);
  if (!display)
    return;

  m_displayRegistry.remove(registryIdFromItem(displayItem));

  removeRow(idx.row());
  DeleteChild(*m_chapter, display);
}

void
NameModel::reset() {
  beginResetModel();

  removeRows(0, rowCount());

  m_displayRegistry.empty();
  m_nextDisplayRegistryIdx = 0;
  m_chapter                = nullptr;

  endResetModel();
}

void
NameModel::populate(KaxChapterAtom &chapter) {
  beginResetModel();

  removeRows(0, rowCount());
  m_displayRegistry.empty();
  m_nextDisplayRegistryIdx = 0;
  m_chapter                = &chapter;

  for (auto const &child : chapter) {
    auto display = dynamic_cast<KaxChapterDisplay *>(child);
    if (display)
      append(*display);
  }

  endResetModel();
}

Qt::DropActions
NameModel::supportedDropActions()
  const {
  return Qt::MoveAction;
}

Qt::ItemFlags
NameModel::flags(QModelIndex const &index)
  const {
  if (!index.isValid())
    return Qt::ItemIsDropEnabled;

  return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemNeverHasChildren;
}

qulonglong
NameModel::registerDisplay(KaxChapterDisplay &display) {
  m_displayRegistry[ ++m_nextDisplayRegistryIdx ] = &display;
  return m_nextDisplayRegistryIdx;
}

qulonglong
NameModel::registryIdFromItem(QStandardItem *item) {
  return item ? item->data(Util::ChapterEditorChapterDisplayRole).value<qulonglong>() : 0;
}

bool
NameModel::canDropMimeData(QMimeData const *data,
                           Qt::DropAction action,
                           int,
                           int,
                           QModelIndex const &parent)
  const {
  if (!data || (Qt::MoveAction != action))
    return false;

  return !parent.isValid();
}

bool
NameModel::dropMimeData(QMimeData const *data,
                        Qt::DropAction action,
                        int row,
                        int column,
                        QModelIndex const &parent) {
  if (!canDropMimeData(data, action, row, column, parent))
    return false;

  auto isInside = (-1 == row) && (-1 == column);
  auto result   = QStandardItemModel::dropMimeData(data, action, isInside ? -1 : row, isInside ? -1 : 0, parent.sibling(parent.row(), 0));

  Util::requestAllItems(*this);

  return result;
}

}}}
