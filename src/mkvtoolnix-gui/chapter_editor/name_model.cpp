#include "common/common_pch.h"

#include "common/chapters/chapters.h"
#include "common/ebml.h"
#include "common/qt.h"
#include "mkvtoolnix-gui/app.h"
#include "mkvtoolnix-gui/chapter_editor/name_model.h"
#include "mkvtoolnix-gui/util/model.h"
#include "mkvtoolnix-gui/util/settings.h"

using namespace mtx::gui;

namespace mtx::gui::ChapterEditor {

NameModel::NameModel(QObject *parent)
  : QStandardItemModel{parent}
{
}

NameModel::~NameModel() {
}

void
NameModel::retranslateUi() {
  Util::setDisplayableAndSymbolicColumnNames(*this, {
    { QY("Name"),      Q("name")     },
    { QY("Languages"), Q("language") },
  });
}

QList<QStandardItem *>
NameModel::newRowItems() {
  return QList<QStandardItem *>{} << new QStandardItem{} << new QStandardItem{};
}

libmatroska::KaxChapterDisplay *
NameModel::displayFromItem(QStandardItem *item) {
  return m_displayRegistry[ registryIdFromItem(item) ];
}

libmatroska::KaxChapterDisplay *
NameModel::displayFromIndex(QModelIndex const &idx) {
  return displayFromItem(itemFromIndex(idx));
}

Languages
NameModel::effectiveLanguagesForDisplay(libmatroska::KaxChapterDisplay &display) {
  Languages lists;
  QList<mtx::bcp47::language_c> legacyLanguageCodes;
  QStringList legacyLanguageNames;

  for (auto const &child : display) {
    if (auto kIetfLanguage = dynamic_cast<libmatroska::KaxChapLanguageIETF const *>(child); kIetfLanguage) {
      auto language = mtx::bcp47::language_c::parse(kIetfLanguage->GetValue());
      lists.languageCodes << language;
      lists.languageNames << Q(language.format_long());

    } else if (auto kLegacyLanguage = dynamic_cast<libmatroska::KaxChapterLanguage const *>(child); kLegacyLanguage) {
      auto language = mtx::bcp47::language_c::parse(kLegacyLanguage->GetValue());
      legacyLanguageCodes << language;
      legacyLanguageNames << Q(language.format_long());

    }
  }

  if (lists.languageCodes.isEmpty()) {
    lists.languageCodes = std::move(legacyLanguageCodes);
    lists.languageNames = std::move(legacyLanguageNames);
  }

  if (lists.languageCodes.isEmpty()) {
    auto eng = mtx::bcp47::language_c::parse("eng");
    lists.languageCodes << eng;
    lists.languageNames << Q(eng.format_long());
  }

  return lists;
}

void
NameModel::setRowText(QList<QStandardItem *> const &rowItems) {
  auto &display = *displayFromItem(rowItems[0]);
  auto lists    = effectiveLanguagesForDisplay(display);

  lists.languageNames.sort();

  rowItems[0]->setText(Q(get_child_value<libmatroska::KaxChapterString>(display)));
  rowItems[1]->setText(lists.languageNames.join(Q("; ")));
}

QList<QStandardItem *>
NameModel::itemsForRow(int row) {
  auto rowItems = QList<QStandardItem *>{};
  for (auto column = 0; 2 > column; ++column)
    rowItems << item(row, column);

  return rowItems;
}

void
NameModel::updateRow(int row) {
  setRowText(itemsForRow(row));
}

void
NameModel::append(libmatroska::KaxChapterDisplay &display) {
  auto rowItems = newRowItems();
  rowItems[0]->setData(registerDisplay(display), Util::ChapterEditorChapterDisplayRole);

  setRowText(rowItems);
  appendRow(rowItems);
}

void
NameModel::addNew() {
  auto &cfg     = Util::Settings::get();
  auto display = new libmatroska::KaxChapterDisplay;

  get_child<libmatroska::KaxChapterString>(display).SetValueUTF8(Y("<Unnamed>"));
  mtx::chapters::set_languages_in_display(*display, cfg.m_defaultChapterLanguage);

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
  delete_child(*m_chapter, display);
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
NameModel::populate(libmatroska::KaxChapterAtom &chapter) {
  beginResetModel();

  removeRows(0, rowCount());
  m_displayRegistry.empty();
  m_nextDisplayRegistryIdx = 0;
  m_chapter                = &chapter;

  for (auto const &child : chapter) {
    auto display = dynamic_cast<libmatroska::KaxChapterDisplay *>(child);
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
NameModel::registerDisplay(libmatroska::KaxChapterDisplay &display) {
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

}
