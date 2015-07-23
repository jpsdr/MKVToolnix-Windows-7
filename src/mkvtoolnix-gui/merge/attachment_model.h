#ifndef MTX_MKVTOOLNIX_GUI_MERGE_ATTACHMENT_MODEL_H
#define MTX_MKVTOOLNIX_GUI_MERGE_ATTACHMENT_MODEL_H

#include "common/common_pch.h"

#include "mkvtoolnix-gui/merge/attachment.h"

#include <QItemSelection>
#include <QList>
#include <QStandardItemModel>

namespace mtx { namespace gui { namespace Merge {

class AttachmentModel;
using AttachmentModelPtr = std::shared_ptr<AttachmentModel>;

class AttachmentModel : public QStandardItemModel {
  Q_OBJECT;

protected:
  QHash<quint64, AttachmentPtr> m_attachmentMap;

protected:
  static int const NameColumn        = 0;
  static int const MIMETypeColumn    = 1;
  static int const DescriptionColumn = 2;
  static int const StyleColumn       = 3;
  static int const SourceFileColumn  = 4;
  static int const SourceDirColumn   = 5;
  static int const NumberOfColumns   = 6;

public:
  AttachmentModel(QObject *parent);
  virtual ~AttachmentModel();

  virtual void reset();
  virtual void removeSelectedAttachments(QItemSelection const &selection);

  virtual void addAttachments(QList<AttachmentPtr> const &attachmentsToAdd);
  virtual void replaceAttachments(QList<AttachmentPtr> const &newAttachments);

  virtual void attachmentUpdated(Attachment const &attachment);

  virtual AttachmentPtr attachmentForRow(int row) const;
  virtual int rowForAttachment(Attachment const &attachment) const;
  virtual QList<QStandardItem *> itemsForRow(int row);
  virtual QList<AttachmentPtr> attachments();

  virtual void retranslateUi();

  virtual Qt::DropActions supportedDropActions() const override;
  virtual Qt::ItemFlags flags(QModelIndex const &index) const override;
  virtual bool canDropMimeData(QMimeData const *data, Qt::DropAction action, int row, int column, QModelIndex const &parent) const override;

  virtual void moveAttachmentsUpOrDown(QList<Attachment *> attachments, bool up);

protected:
  QList<QStandardItem *> createRowItems(AttachmentPtr const &attachment) const;
  void setRowData(QList<QStandardItem *> const &items, Attachment const &attachment);

};

}}}

#endif  // MTX_MKVTOOLNIX_GUI_MERGE_ATTACHMENT_MODEL_H
