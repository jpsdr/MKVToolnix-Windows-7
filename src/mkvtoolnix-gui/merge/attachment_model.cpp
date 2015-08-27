#include "common/common_pch.h"

#include <QDebug>
#include <QFileInfo>
#include <QMimeData>

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

  items[NameColumn       ]->setText(attachment.m_name);
  items[MIMETypeColumn   ]->setText(attachment.m_MIMEType);
  items[DescriptionColumn]->setText(attachment.m_description);
  items[StyleColumn      ]->setText(attachment.m_style == Attachment::ToAllFiles ? QY("to all output files") : QY("only to the first output file"));
  items[SourceFileColumn ]->setText(info.fileName());
  items[SourceDirColumn  ]->setText(info.path());
}

void
AttachmentModel::retranslateUi() {
  auto labels = QStringList{} << QY("Name") << QY("MIME type") << QY("Description") << QY("Attach to") << QY("Source file name") << QY("Directory");
  setHorizontalHeaderLabels(labels);

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
  if (-1 == rc) {
    rc = rowCount();
    qDebug() << "mtx attachments canDropMimeData initital rowCount" << rc;
  }

  if (!QStandardItemModel::canDropMimeData(data, action, row, column, parent))
    return false;

  if (rowCount() != rc)
    qDebug() << "mtx attachments canDropMimeData ROW COUNT MISMATCH initial" << rc << "current" << rowCount() << "row" << row << "column" << column << "parent" << parent << "action" << action;

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
  if (-1 == rc) {
    rc = rowCount();
    qDebug() << "mtx attachments dropMimeData initital rowCount" << rc;
  }

  bool ok = (Qt::MoveAction == action)
         && !parent.isValid()
         && (   ((0  <= row) && ( 0 <= column))
             || ((-1 == row) && (-1 == column)));

  if (!ok) {
    qDebug() << "mtx attachments DROP NOT OK; row" << row << "column" << column << "parent" << parent << "action" << action;
    return false;
  }

  auto rcBefore = rowCount();
  auto result   = QStandardItemModel::dropMimeData(data, action, row, 0, parent);
  auto rcAfter  = rowCount();

  qDebug() << "mtx attachments dropMimeData row count initial" << rc << "before" << rcBefore << "after" << rcAfter << "row" << row << "column" << column << "parent" << parent << "action" << action;

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
