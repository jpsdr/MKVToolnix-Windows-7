#include "common/common_pch.h"

#include <QTreeWidgetItem>

#include "common/iso639.h"
#include "common/qt.h"
#include "mkvtoolnix-gui/merge/disc_library_information_widget.h"
#include "mkvtoolnix-gui/util/model.h"
#include "mkvtoolnix-gui/util/widget.h"

namespace mtx::gui::Merge {

namespace {

class DiscLibraryItem: public QTreeWidgetItem {
public:
  mtx::bluray::disc_library::info_t m_info;

public:
  DiscLibraryItem(mtx::bluray::disc_library::info_t const &info, QStringList const &texts);
  virtual bool operator <(QTreeWidgetItem const &cmp) const;

public:
  static DiscLibraryItem *create(std::string const &language, mtx::bluray::disc_library::info_t const &info);
};

DiscLibraryItem::DiscLibraryItem(mtx::bluray::disc_library::info_t const &info,
                                 QStringList const &texts)
  : QTreeWidgetItem{texts}
  , m_info{info}
{
}

bool
DiscLibraryItem::operator <(QTreeWidgetItem const &cmp)
  const {
  auto &otherItem = static_cast<DiscLibraryItem const &>(cmp);
  auto column     = treeWidget()->sortColumn();

  if (column != 3)
    return text(column).toLower() < otherItem.text(column).toLower();

  auto mySize    = std::make_pair(          data(3, Qt::UserRole).toUInt(),           data(3, Qt::UserRole + 1).toUInt());
  auto otherSize = std::make_pair(otherItem.data(3, Qt::UserRole).toUInt(), otherItem.data(3, Qt::UserRole + 1).toUInt());

  return mySize < otherSize;
}

DiscLibraryItem *
DiscLibraryItem::create(std::string const &language,
                        mtx::bluray::disc_library::info_t const &info) {
  auto biggestThumbnail = info.m_thumbnails.begin();
  auto end              = info.m_thumbnails.end();

  for (auto currentThumbnail = info.m_thumbnails.begin(); currentThumbnail != end; ++currentThumbnail) {
    auto size = currentThumbnail->m_width * currentThumbnail->m_height;

    if (size > (biggestThumbnail->m_width * biggestThumbnail->m_height))
      biggestThumbnail = currentThumbnail;
  }

  auto languageOpt  = mtx::iso639::look_up(language);
  auto languageName = languageOpt ? Q(languageOpt->english_name) : Q(language);

  auto item = new DiscLibraryItem{info, QStringList{
    languageName,
    Q(info.m_title),
    biggestThumbnail == end ? Q("") : Q(biggestThumbnail->m_file_name.filename().string()),
    biggestThumbnail == end ? Q("") : Q("%1x%2").arg(biggestThumbnail->m_width).arg(biggestThumbnail->m_height),
  }};

  item->setData(0, Qt::UserRole, Q(language));

  if (biggestThumbnail != end) {
    item->setTextAlignment(3, Qt::AlignRight | Qt::AlignVCenter);
    item->setData(3, Qt::UserRole,     biggestThumbnail->m_width);
    item->setData(3, Qt::UserRole + 1, biggestThumbnail->m_height);
  }

  return item;
}

} // anonymous namespace

// ----------------------------------------------------------------------

DiscLibraryInformationWidget::DiscLibraryInformationWidget(QWidget *parent)
  : QTreeWidget{parent}
{
}

DiscLibraryInformationWidget::~DiscLibraryInformationWidget() {
}

void
DiscLibraryInformationWidget::setDiscLibrary(mtx::bluray::disc_library::disc_library_t const &discLibrary) {
  m_discLibrary = discLibrary;
}

bool
DiscLibraryInformationWidget::isEmpty()
  const {
  return m_discLibrary.m_infos_by_language.empty();
}

std::optional<mtx::bluray::disc_library::info_t>
DiscLibraryInformationWidget::selectedInfo() {
  auto discLibraryIdx = Util::selectedRowIdx(this);

  if (!discLibraryIdx.isValid())
    return {};

  auto libraryItem = dynamic_cast<DiscLibraryItem *>(invisibleRootItem()->child(discLibraryIdx.row()));
  if (libraryItem)
    return libraryItem->m_info;

  return {};
}

void
DiscLibraryInformationWidget::setup() {
  if (isEmpty())
    return;

  auto item = headerItem();
  item->setText(0, QY("Language"));
  item->setText(1, QY("Title"));
  item->setText(2, QY("Cover image"));
  item->setText(3, QY("Size"));

  auto items = QList<QTreeWidgetItem *>{};
  for (auto const &infoItr : m_discLibrary.m_infos_by_language)
    items << DiscLibraryItem::create(infoItr.first, infoItr.second);

  insertTopLevelItems(0, items);
  setSortingEnabled(true);
  sortItems(0, Qt::AscendingOrder);

  setEditTriggers(QAbstractItemView::NoEditTriggers);
  setProperty("showDropIndicator", false);

  int rowToSelect = 0;

  for (auto row = 0, numRows = model()->rowCount(); row < numRows; ++row) {
    auto language = model()->data(model()->index(row, 0), Qt::UserRole).toString();
    if (language == Q("eng")) {
      rowToSelect = row;
      break;
    }
  }

  Util::selectRow(this, rowToSelect);

  Util::resizeViewColumnsToContents(this);
}

}
