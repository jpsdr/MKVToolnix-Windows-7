#include "common/common_pch.h"

#include "mkvtoolnix-gui/merge/attachment_model.h"
#include "mkvtoolnix-gui/util/util.h"

#include <boost/range/adaptor/uniqued.hpp>
#include <QFileInfo>

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
  endResetModel();
}

QList<QStandardItem *>
AttachmentModel::createRowItems(AttachmentPtr const &attachment)
  const {
  auto list = QList<QStandardItem *>{};
  for (auto column = 0; column < NumberOfColumns; ++column)
    list << new QStandardItem{};

  list[0]->setData(QVariant::fromValue(attachment), Util::AttachmentRole);

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

  for (auto row : rowsToRemove)
    removeRow(row);
}

void
AttachmentModel::addAttachments(QList<AttachmentPtr> const &attachmentsToAdd) {
  if (attachmentsToAdd.isEmpty())
    return;

  for (auto const &attachment : attachmentsToAdd) {
    auto newRow = createRowItems(attachment);
    setRowData(newRow, *attachment);
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
  return item(row)->data(Util::AttachmentRole).value<AttachmentPtr>();
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

}}}
