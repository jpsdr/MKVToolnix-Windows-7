#pragma once

#include "common/common_pch.h"

#include "mkvtoolnix-gui/merge/source_file.h"

#include <QStandardItemModel>
#include <QIcon>
#include <QList>
#include <QSet>

namespace mtx { namespace gui { namespace Merge {

class SourceFileModel;
using SourceFileModelPtr = std::shared_ptr<SourceFileModel>;

class TrackModel;
class AttachedFileModel;

class SourceFileModel : public QStandardItemModel {
  Q_OBJECT;

protected:
  QList<SourceFilePtr> *m_sourceFiles;
  QHash<quint64, SourceFilePtr> m_sourceFileMap;
  QIcon m_additionalPartIcon, m_addedIcon, m_normalIcon;
  TrackModel *m_tracksModel;
  AttachedFileModel *m_attachedFilesModel;
  bool m_nonAppendedSelected, m_appendedSelected, m_additionalPartSelected;

public:
  SourceFileModel(QObject *parent);
  virtual ~SourceFileModel();

  virtual void retranslateUi();

  virtual void setSourceFiles(QList<SourceFilePtr> &sourceFiles);
  virtual void setOtherModels(TrackModel *tracksModel, AttachedFileModel *attachedFilesModel);
  virtual void addOrAppendFilesAndTracks(QModelIndex const &fileToAddToIdx, QList<SourceFilePtr> const &files, bool append);
  virtual void addAdditionalParts(QModelIndex const &fileToAddToIdx, QStringList const &fileNames);
  virtual void removeFiles(QList<SourceFile *> const &files);
  virtual void removeFile(SourceFile *fileToBeRemoved);

  virtual void moveSourceFilesUpOrDown(QList<SourceFile *> files, bool up);

  virtual SourceFilePtr fromIndex(QModelIndex const &idx) const;
  virtual QModelIndex indexFromSourceFile(SourceFile *sourceFile) const;

  virtual Qt::DropActions supportedDropActions() const;
  virtual Qt::ItemFlags flags(QModelIndex const &index) const;

  virtual QStringList mimeTypes() const;
  virtual QMimeData *mimeData(QModelIndexList const &indexes) const;

  virtual bool canDropMimeData(QMimeData const *data, Qt::DropAction action, int row, int column, QModelIndex const &parent) const override;
  virtual bool dropMimeData(QMimeData const *data, Qt::DropAction action, int row, int column, QModelIndex const &parent) override;

public slots:
  void updateSelectionStatus();
  void updateSourceFileLists();

protected:
  virtual void addFilesAndTracks(QList<SourceFilePtr> const &files);
  virtual void appendFilesAndTracks(QModelIndex const &fileToAddToIdx, QList<SourceFilePtr> const &files);

  void setItemsFromSourceFile(QList<QStandardItem *> const &items, SourceFile *sourceFile) const;
  QList<QStandardItem *> createRow(SourceFile *sourceFile) const;
  void createAndAppendRow(QStandardItem *item, SourceFilePtr const &file, int position = -1);
  void dumpSourceFiles(QString const &label) const;

  quint64 storageValueFromIndex(QModelIndex const &idx) const;

  bool dropSourceFiles(QMimeData const *data, Qt::DropAction action, int row, const QModelIndex &parent);

  QModelIndex indexFromSourceFile(quint64 value, QModelIndex const &parent) const;

  void sourceFileUpdated(SourceFile *sourceFile);

  void sortSourceFiles(QList<SourceFile *> &files, bool reverse = false);

  std::pair<int, int> countAppendedAndAdditionalParts(QStandardItem *parentItem);
};

}}}
