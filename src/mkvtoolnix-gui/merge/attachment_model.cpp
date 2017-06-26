#include "common/common_pch.h"

#include <QDir>
#include <QFileInfo>
#include <QMimeData>

#include "common/strings/formatting.h"
#include "mkvtoolnix-gui/merge/attachment_model.h"
#include "mkvtoolnix-gui/util/model.h"

namespace mtx { namespace gui { namespace Merge {

AttachmentModel::AttachmentModel(QObject *parent)
  : QStandardItemModel{parent}
{
  retranslateUi();
}

AttachmentModel::~AttachmentModel() {
}

void
AttachmentModel::reset() {
  beginResetModel();
  removeRows(0, rowCount());
  m_attachmentMap.clear();
  endResetModel();
}

QList<QStandardItem *>
AttachmentModel::createRowItems(AttachmentPtr const &attachment)
  const {
  auto list = QList<QStandardItem *>{};
  for (auto column = 0; column < NumberOfColumns; ++column)
    list << new QStandardItem{};

  list[0]->setData(reinterpret_cast<quint64>(attachment.get()), Util::AttachmentRole);

  return list;
}

void
AttachmentModel::setRowData(QList<QStandardItem *> const &items,
                            Attachment const &attachment) {
  auto info = QFileInfo{attachment.m_fileName};
  auto size = QNY("%1 byte (%2)", "%1 bytes (%2)", info.size()).arg(info.size()).arg(Q(format_file_size(info.size())));

  items[NameColumn       ]->setText(attachment.m_name);
  items[MIMETypeColumn   ]->setText(attachment.m_MIMEType);
  items[DescriptionColumn]->setText(attachment.m_description);
  items[StyleColumn      ]->setText(attachment.m_style == Attachment::ToAllFiles ? QY("To all destination files") : QY("Only to the first destination file"));
  items[SourceFileColumn ]->setText(info.fileName());
  items[SourceDirColumn  ]->setText(QDir::toNativeSeparators(info.path()));
  items[SizeColumn       ]->setText(size);

  items[SizeColumn       ]->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
}

void
AttachmentModel::retranslateUi() {
  Util::setDisplayableAndSymbolicColumnNames(*this, {
    { QY("Name"),             Q("name")           },
    { QY("MIME type"),        Q("mimeType")       },
    { QY("Attach to"),        Q("attachTo")       },
    { QY("Description"),      Q("description")    },
    { QY("Source file name"), Q("sourceFileName") },
    { QY("Directory"),        Q("directory")      },
    { QY("Size"),             Q("size")           },
  });

  horizontalHeaderItem(SizeColumn)->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

  for (auto row = 0, numRows = rowCount(); row < numRows; ++row)
    setRowData(itemsForRow(row), *attachmentForRow(row));
}

void
AttachmentModel::attachmentUpdated(Attachment const &attachment) {
  auto row = rowForAttachment(attachment);
  if (-1 != row)
    setRowData(itemsForRow(row), attachment);
}

void
AttachmentModel::removeSelectedAttachments(QItemSelection const &selection) {
  if (selection.isEmpty())
    return;

  auto rowsToRemove = QSet<int>{};

  for (auto &range : selection)
    for (auto &index : range.indexes())
      rowsToRemove << index.row();

  auto sortedRowsToRemove = rowsToRemove.toList();
  std::sort(sortedRowsToRemove.begin(), sortedRowsToRemove.end(), std::greater<int>());

  for (auto row : sortedRowsToRemove)
    removeRow(row);
}

void
AttachmentModel::addAttachments(QList<AttachmentPtr> const &attachmentsToAdd) {
  if (attachmentsToAdd.isEmpty())
    return;

  for (auto const &attachment : attachmentsToAdd) {
    auto newRow = createRowItems(attachment);
    setRowData(newRow, *attachment);
    m_attachmentMap[reinterpret_cast<quint64>(attachment.get())] = attachment;
    appendRow(newRow);
  }
}

void
AttachmentModel::replaceAttachments(QList<AttachmentPtr> const &newAttachments) {
  reset();
  addAttachments(newAttachments);
}

AttachmentPtr
AttachmentModel::attachmentForRow(int row)
  const {
  return m_attachmentMap.value(item(row)->data(Util::AttachmentRole).value<quint64>(), AttachmentPtr{});
}

int
AttachmentModel::rowForAttachment(Attachment const &attachment)
  const {
  for (auto row = 0, numRows = rowCount(); row < numRows; ++row)
    if (attachmentForRow(row).get() == &attachment)
      return row;

  return -1;
}

QList<AttachmentPtr>
AttachmentModel::attachments() {
  auto attachments = QList<AttachmentPtr>{};
  for (auto row = 0, numRows = rowCount(); row < numRows; ++row)
    attachments << attachmentForRow(row);

  return attachments;
}

QList<QStandardItem *>
AttachmentModel::itemsForRow(int row) {
  auto list = QList<QStandardItem *>{};
  for (auto column = 0; column < NumberOfColumns; ++column)
    list << item(row, column);

  return list;
}

Qt::DropActions
AttachmentModel::supportedDropActions()
  const {
  return Qt::MoveAction;
}

static int rc = -1;

bool
AttachmentModel::canDropMimeData(QMimeData const *data,
                                 Qt::DropAction action,
                                 int row,
                                 int column,
                                 QModelIndex const &parent)
  const {
  if (-1 == rc)
    rc = rowCount();

  if (!QStandardItemModel::canDropMimeData(data, action, row, column, parent))
    return false;

  bool ok = (Qt::MoveAction == action)
         && !parent.isValid()
         && (   ((0  <= row) && ( 0 <= column))
             || ((-1 == row) && (-1 == column)));

  return ok;
}

bool
AttachmentModel::dropMimeData(QMimeData const *data,
                              Qt::DropAction action,
                              int row,
                              int column,
                              QModelIndex const &parent) {
  if (-1 == rc)
    rc = rowCount();

  bool ok = (Qt::MoveAction == action)
         && !parent.isValid()
         && (   ((0  <= row) && ( 0 <= column))
             || ((-1 == row) && (-1 == column)));

  if (!ok)
    return false;

  auto result = QStandardItemModel::dropMimeData(data, action, row, 0, parent);

  Util::requestAllItems(*this);

  return result;
}

Qt::ItemFlags
AttachmentModel::flags(QModelIndex const &index)
  const {
  auto defaultFlags = QStandardItemModel::flags(index) & ~Qt::ItemIsDropEnabled;
  return index.isValid() ? defaultFlags | Qt::ItemIsDragEnabled : defaultFlags | Qt::ItemIsDropEnabled;
}

void
AttachmentModel::moveAttachmentsUpOrDown(QList<Attachment *> attachments,
                                         bool up) {
  auto attachmentRowMap = QHash<Attachment *, int>{};
  for (auto const &attachment : attachments)
    attachmentRowMap[attachment] = rowForAttachment(*attachment);

  std::sort(attachments.begin(), attachments.end(), [&attachmentRowMap](Attachment *a, Attachment *b) { return attachmentRowMap[a] < attachmentRowMap[b]; });

  if (!up)
    std::reverse(attachments.begin(), attachments.end());

  auto couldNotBeMoved = QHash<Attachment *, bool>{};
  auto isSelected      = QHash<Attachment *, bool>{};
  auto const direction = up ? -1 : +1;
  auto const numRows   = rowCount();

  for (auto const &attachment : attachments) {
    isSelected[attachment] = true;

    auto currentRow = rowForAttachment(*attachment);
    Q_ASSERT(0 <= currentRow);

    auto targetRow = currentRow + direction;

    if (   (0       >  targetRow)
        || (numRows <= targetRow)
        || couldNotBeMoved[attachmentForRow(targetRow).get()])
      couldNotBeMoved[attachment] = true;

    else
      insertRow(targetRow, takeRow(currentRow));
  }
}

}}}
