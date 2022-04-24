#pragma once

#include "common/common_pch.h"

#include "mkvtoolnix-gui/merge/track.h"

#include <QList>
#include <QStandardItemModel>

namespace mtx::gui::Merge {

class AttachedFileModel;
using AttachedFileModelPtr = std::shared_ptr<AttachedFileModel>;

class AttachedFileModel : public QStandardItemModel {
  Q_OBJECT

protected:
  QHash<quint64, TrackPtr> m_attachedFilesMap;
  QIcon m_yesIcon, m_noIcon;

protected:
  static int const NameColumn        = 0;
  static int const MIMETypeColumn    = 1;
  static int const MuxThisColumn     = 2;
  static int const DescriptionColumn = 3;
  static int const SourceFileColumn  = 4;
  static int const SourceDirColumn   = 5;
  static int const SizeColumn        = 6;
  static int const NumberOfColumns   = 7;

public:
  AttachedFileModel(QObject *parent);
  virtual ~AttachedFileModel();

  virtual void reset();

  virtual void addAttachedFiles(QList<TrackPtr> const &attachedFilesToAdd);
  virtual void removeAttachedFiles(QList<TrackPtr> const &attachedFilesToRemove);
  virtual void setAttachedFiles(QList<TrackPtr> const &attachedFilesToAdd);

  virtual void attachedFileUpdated(Track const &attachedFile);

  virtual TrackPtr attachedFileForRow(int row) const;
  virtual std::optional<int> rowForAttachedFile(Track const &attachedFile) const;
  virtual QList<QStandardItem *> itemsForRow(int row);

  virtual void retranslateUi();

protected:
  QList<QStandardItem *> createRowItems(Track const &attachedFile) const;
  void setRowData(QList<QStandardItem *> const &items, Track const &attachedFiles);
};

}
