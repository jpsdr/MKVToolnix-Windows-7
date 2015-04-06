#include "common/common_pch.h"

#include "common/ebml.h"
#include "common/qt.h"
#include "mkvtoolnix-gui/chapter_editor/name_model.h"
#include "mkvtoolnix-gui/util/util.h"

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
  auto labels = QStringList{} << QY("Name") << QY("Language") << QY("Country");
  setHorizontalHeaderLabels(labels);
}

QList<QStandardItem *>
NameModel::newRowItems() {
  return QList<QStandardItem *>{} << new QStandardItem{} << new QStandardItem{} << new QStandardItem{};
}

KaxChapterDisplay *
NameModel::displayFromItem(QStandardItem *item) {
  return item ? item->data(Util::ChapterEditorChapterDisplayRole).value<KaxChapterDisplay *>() : nullptr;
}

KaxChapterDisplay *
NameModel::displayFromIndex(QModelIndex const &idx) {
  return displayFromItem(itemFromIndex(idx));
}

void
NameModel::setRowText(QList<QStandardItem *> const &rowItems) {
  auto &display = *displayFromItem(rowItems[0]);

  rowItems[0]->setText(Q(GetChildValue<KaxChapterString>(display)));
  rowItems[1]->setText(Q(FindChildValue<KaxChapterLanguage>(display, std::string{"eng"})));
  rowItems[2]->setText(Q(FindChildValue<KaxChapterCountry>(display, std::string{""})));
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
  rowItems[0]->setData(QVariant::fromValue(&display), Util::ChapterEditorChapterDisplayRole);

  setRowText(rowItems);
  appendRow(rowItems);
}

void
NameModel::addNew() {
  auto display = new KaxChapterDisplay;
  GetChild<KaxChapterString>(display).SetValueUTF8(Y("<unnamed>"));
  GetChild<KaxChapterLanguage>(display).SetValue("eng");

  m_chapter->PushElement(*display);
  append(*display);
}

void
NameModel::remove(QModelIndex const &idx) {
  auto chapter = displayFromItem(itemFromIndex(idx));
  if (!chapter)
    return;

  removeRow(idx.row());
  DeleteChild(*m_chapter, chapter);
}

void
NameModel::reset() {
  beginResetModel();
  removeRows(0, rowCount());
  m_chapter = nullptr;
  endResetModel();
}

void
NameModel::populate(KaxChapterAtom &chapter) {
  beginResetModel();
  removeRows(0, rowCount());

  m_chapter = &chapter;

  for (auto const &child : chapter) {
    auto display = dynamic_cast<KaxChapterDisplay *>(child);
    if (display)
      append(*display);
  }

  endResetModel();
}

}}}
