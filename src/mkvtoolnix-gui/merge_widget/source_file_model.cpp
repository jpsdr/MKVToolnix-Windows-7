#include "common/common_pch.h"

#include "common/sorting.h"
#include "common/strings/formatting.h"
#include "mkvtoolnix-gui/merge_widget/source_file_model.h"
#include "mkvtoolnix-gui/merge_widget/track_model.h"
#include "mkvtoolnix-gui/util/util.h"

#include <QFileInfo>
#include <QItemSelectionModel>
#include <QMimeData>
#include <QByteArray>
#include <QDataStream>

SourceFileModel::SourceFileModel(QObject *parent)
  : QStandardItemModel{parent}
  , m_sourceFiles{}
  , m_tracksModel{}
  , m_nonAppendedSelected{}
  , m_appendedSelected{}
  , m_additionalPartSelected{}
{
  m_additionalPartIcon.addFile(":/icons/16x16/distribute-horizontal-margin.png");
  m_addedIcon.addFile(":/icons/16x16/distribute-horizontal-x.png");
  m_normalIcon.addFile(":/icons/16x16/distribute-vertical-page.png");

  auto labels = QStringList{};
  labels << QY("File name") << QY("Container") << QY("File size") << QY("Directory");
  setHorizontalHeaderLabels(labels);
  horizontalHeaderItem(2)->setTextAlignment(Qt::AlignRight);
}

SourceFileModel::~SourceFileModel() {
}

void
SourceFileModel::setTracksModel(TrackModel *tracksModel) {
  m_tracksModel = tracksModel;
}

void
SourceFileModel::createAndAppendRow(QStandardItem *item,
                                    SourceFilePtr const &file,
                                    int position) {
  m_sourceFileMap[reinterpret_cast<quint64>(file.get())] = file;
  auto row                                               = createRow(file.get());

  if (file->isAdditionalPart()) {
    auto fileToAddTo = m_sourceFileMap[item->data(Util::SourceFileRole).value<quint64>()];
    Q_ASSERT(fileToAddTo);
    item->insertRow(position, row);

  } else
    item->appendRow(row);
}

void
SourceFileModel::setSourceFiles(QList<SourceFilePtr> &sourceFiles) {
  removeRows(0, rowCount());
  m_sourceFileMap.clear();

  m_sourceFiles = &sourceFiles;
  auto row      = 0u;

  for (auto const &file : *m_sourceFiles) {
    createAndAppendRow(invisibleRootItem(), file);

    mxinfo(boost::format("file %1% #add %2% #app %3%\n") % file->m_fileName % file->m_additionalParts.size() % file->m_appendedFiles.size());

    auto rowItem  = item(row);
    auto position = 0;

    for (auto const &additionalPart : file->m_additionalParts)
      createAndAppendRow(rowItem, additionalPart, position++);

    for (auto const &appendedFile : file->m_appendedFiles)
      createAndAppendRow(rowItem, appendedFile);

    ++row;
  }
}

QList<QStandardItem *>
SourceFileModel::createRow(SourceFile *sourceFile)
  const {
  auto items = QList<QStandardItem *>{};
  auto info  = QFileInfo{sourceFile->m_fileName};

  items << new QStandardItem{info.fileName()};
  items << new QStandardItem{sourceFile->isAdditionalPart() ? QY("(additional part)") : sourceFile->m_container};
  items << new QStandardItem{to_qs(format_file_size(sourceFile->isPlaylist() ? sourceFile->m_playlistSize : info.size()))};
  items << new QStandardItem{info.path()};

  items[0]->setData(reinterpret_cast<quint64>(sourceFile), Util::SourceFileRole);
  items[0]->setIcon(  sourceFile->isAdditionalPart() ? m_additionalPartIcon
                    : sourceFile->isAppended()       ? m_addedIcon
                    :                                  m_normalIcon);

  items[2]->setTextAlignment(Qt::AlignRight);

  return items;
}

quint64
SourceFileModel::storageValueFromIndex(QModelIndex const &idx)
  const {
  return idx.sibling(idx.row(), 0)
    .data(Util::SourceFileRole)
    .value<quint64>();
}

SourceFilePtr
SourceFileModel::fromIndex(QModelIndex const &idx)
  const {
  if (!idx.isValid())
    return nullptr;
  return m_sourceFileMap[storageValueFromIndex(idx)];
}

QModelIndex
SourceFileModel::indexFromSourceFile(SourceFile *sourceFile)
  const {
  if (!sourceFile)
    return QModelIndex{};

  return indexFromSourceFile(reinterpret_cast<quint64>(sourceFile), QModelIndex{});
}

QModelIndex
SourceFileModel::indexFromSourceFile(quint64 value,
                                     QModelIndex const &parent)
  const {
  auto currentValue = storageValueFromIndex(parent);
  if (currentValue == value)
    return parent;

  auto invalidIdx = QModelIndex{};

  for (auto row = 0, numRows = rowCount(parent); row < numRows; ++row) {
    auto idx = indexFromSourceFile(value, index(row, 0, parent));
    if (idx != invalidIdx)
      return idx;
  }

  return invalidIdx;
}

void
SourceFileModel::addAdditionalParts(QModelIndex const &fileToAddToIdx,
                                    QStringList const &fileNames) {
  auto actualIdx = Util::toTopLevelIdx(fileToAddToIdx);
  if (fileNames.isEmpty() || !actualIdx.isValid())
    return;

  auto fileToAddTo = fromIndex(actualIdx);
  auto itemToAddTo = itemFromIndex(actualIdx);
  Q_ASSERT(fileToAddTo && itemToAddTo);

  auto actualFileNames = QStringList{};
  std::copy_if(fileNames.begin(), fileNames.end(), std::back_inserter(actualFileNames), [&fileToAddTo](QString const &fileName) -> bool {
      if (fileToAddTo->m_fileName == fileName)
        return false;
      for (auto additionalPart : fileToAddTo->m_additionalParts)
        if (additionalPart->m_fileName == fileName)
          return false;
      return true;
    });

  if (actualFileNames.isEmpty())
    return;

  mtx::sort::naturally(actualFileNames.begin(), actualFileNames.end());

  for (auto &fileName : actualFileNames) {
    auto additionalPart              = std::make_shared<SourceFile>(fileName);
    additionalPart->m_additionalPart = true;
    additionalPart->m_appendedTo     = fileToAddTo.get();

    createAndAppendRow(itemToAddTo, additionalPart, fileToAddTo->m_additionalParts.size());

    fileToAddTo->m_additionalParts << additionalPart;
  }
}

void
SourceFileModel::addOrAppendFilesAndTracks(QModelIndex const &fileToAddToIdx,
                                           QList<SourceFilePtr> const &files,
                                           bool append) {
  Q_ASSERT(m_tracksModel);

  if (files.isEmpty())
    return;

  if (append)
    appendFilesAndTracks(fileToAddToIdx, files);
  else
    addFilesAndTracks(files);
}

void
SourceFileModel::addFilesAndTracks(QList<SourceFilePtr> const &files) {
  for (auto const &file : files) {
    createAndAppendRow(invisibleRootItem(), file);
    *m_sourceFiles << file;
  }

  m_tracksModel->addTracks(std::accumulate(files.begin(), files.end(), QList<TrackPtr>{}, [](QList<TrackPtr> &accu, SourceFilePtr const &file) { return accu << file->m_tracks; }));
}

void
SourceFileModel::removeFile(SourceFile *fileToBeRemoved) {
  m_sourceFileMap.remove(reinterpret_cast<quint64>(fileToBeRemoved));

  if (fileToBeRemoved->isAdditionalPart()) {
    auto row           = Util::findPtr(fileToBeRemoved,               fileToBeRemoved->m_appendedTo->m_additionalParts);
    auto parentFileRow = Util::findPtr(fileToBeRemoved->m_appendedTo, *m_sourceFiles);

    Q_ASSERT((-1 != row) && (-1 != parentFileRow));

    item(parentFileRow)->removeRow(row);
    fileToBeRemoved->m_appendedTo->m_additionalParts.removeAt(row);

    return;
  }

  if (fileToBeRemoved->isAppended()) {
    auto row           = Util::findPtr(fileToBeRemoved,               fileToBeRemoved->m_appendedTo->m_appendedFiles);
    auto parentFileRow = Util::findPtr(fileToBeRemoved->m_appendedTo, *m_sourceFiles);

    Q_ASSERT((-1 != row) && (-1 != parentFileRow));

    row += fileToBeRemoved->m_appendedTo->m_additionalParts.size();

    item(parentFileRow)->removeRow(row);
    fileToBeRemoved->m_appendedTo->m_appendedFiles.removeAt(row);

    return;
  }

  auto row = Util::findPtr(fileToBeRemoved, *m_sourceFiles);
  Q_ASSERT(-1 != row);

  invisibleRootItem()->removeRow(row);
  m_sourceFiles->removeAt(row);
}

void
SourceFileModel::removeFiles(QList<SourceFile *> const &files) {
  auto filesToRemove  = files.toSet();
  auto tracksToRemove = QSet<Track *>{};

  for (auto const &file : files) {
    for (auto const &track : file->m_tracks)
      tracksToRemove << track.get();

    for (auto const &appendedFile : file->m_appendedFiles) {
      filesToRemove << appendedFile.get();
      for (auto const &track : appendedFile->m_tracks)
        tracksToRemove << track.get();
    }
  }

  m_tracksModel->reDistributeAppendedTracksForFileRemoval(filesToRemove);
  m_tracksModel->removeTracks(tracksToRemove);

  auto filesToRemoveLast = QList<SourceFile *>{};
  for (auto &file : filesToRemove)
    if (!file->isRegular())
      removeFile(file);
    else
      filesToRemoveLast << file;

  for (auto &file : filesToRemoveLast)
    if (file->isRegular())
      removeFile(file);
}

void
SourceFileModel::appendFilesAndTracks(QModelIndex const &fileToAppendToIdx,
                                      QList<SourceFilePtr> const &files) {
  auto actualIdx = Util::toTopLevelIdx(fileToAppendToIdx);
  if (files.isEmpty() || !actualIdx.isValid())
    return;

  auto fileToAppendTo = fromIndex(actualIdx);
  auto itemToAppendTo = itemFromIndex(actualIdx);
  Q_ASSERT(fileToAppendTo && itemToAppendTo);

  for (auto const &file : files) {
    file->m_appended   = true;
    file->m_appendedTo = fileToAppendTo.get();

    createAndAppendRow(itemToAppendTo, file);

    fileToAppendTo->m_appendedFiles << file;
  }

  for (auto const &file : files)
    m_tracksModel->appendTracks(fileToAppendTo.get(), file->m_tracks);
}

void
SourceFileModel::updateSelectionStatus() {
  m_nonAppendedSelected    = false;
  m_appendedSelected       = false;
  m_additionalPartSelected = false;

  auto selectionModel      = qobject_cast<QItemSelectionModel *>(QObject::sender());
  Q_ASSERT(selectionModel);

  Util::withSelectedIndexes(selectionModel, [this](QModelIndex const &selectedIndex) {
    auto sourceFile = fromIndex(selectedIndex);
    Q_ASSERT(!!sourceFile);

    if (sourceFile->isRegular())
      m_nonAppendedSelected = true;

    else if (sourceFile->isAppended())
      m_appendedSelected = true;

    else if (sourceFile->isAdditionalPart())
      m_additionalPartSelected = true;
  });

  mxinfo(boost::format("file sel changed nonApp %1% app %2% addPart %3%\n") % m_nonAppendedSelected % m_appendedSelected % m_additionalPartSelected);
}

void
SourceFileModel::dumpSourceFiles(QString const &label)
  const {
  auto dumpIt = [](std::string const &prefix, SourceFilePtr const &sourceFile) {
    mxinfo(boost::format("%1%%2%\n") % prefix % sourceFile->m_fileName);
  };

  mxinfo(boost::format("Dumping source files %1%\n") % label);

  for (auto const &sourceFile : *m_sourceFiles) {
    dumpIt("  ", sourceFile);
    for (auto const &additionalPart : sourceFile->m_additionalParts)
      dumpIt("    () ", additionalPart);
    for (auto const &appendedSourceFile : sourceFile->m_appendedFiles)
      dumpIt("    +  ", appendedSourceFile);
  }
}

void
SourceFileModel::updateSourceFileLists() {
  for (auto const &sourceFile : *m_sourceFiles) {
    sourceFile->m_additionalParts.clear();
    sourceFile->m_appendedFiles.clear();
  }

  m_sourceFiles->clear();

  for (auto row = 0, numRows = rowCount(); row < numRows; ++row) {
    auto idx        = index(row, 0, QModelIndex{});
    auto sourceFile = fromIndex(idx);

    Q_ASSERT(sourceFile);

    *m_sourceFiles << sourceFile;

    for (auto appendedRow = 0, numAppendedRows = rowCount(idx); appendedRow < numAppendedRows; ++appendedRow) {
      auto appendedSourceFile = fromIndex(index(appendedRow, 0, idx));
      Q_ASSERT(appendedSourceFile);

      appendedSourceFile->m_appendedTo = sourceFile.get();
      if (appendedSourceFile->isAppended())
        sourceFile->m_appendedFiles << appendedSourceFile;
      else
        sourceFile->m_additionalParts << appendedSourceFile;
    }
  }

  // TODO: SourceFileModel::updateSourceFileLists move dropped additional parts to end of additional parts sub-list

  dumpSourceFiles("updateSourceFileLists END");
}

Qt::DropActions
SourceFileModel::supportedDropActions()
  const {
  return Qt::MoveAction;
}

Qt::ItemFlags
SourceFileModel::flags(QModelIndex const &index)
  const {
  auto actualFlags = QStandardItemModel::flags(index) & ~Qt::ItemIsDropEnabled & ~Qt::ItemIsDragEnabled;

  // If both appended files/additional parts and non-appended files
  // have been selected then those cannot be dragged & dropped at the
  // same time.
  if (m_nonAppendedSelected && (m_appendedSelected | m_additionalPartSelected))
    return actualFlags;

  // Everyting else can be at least dragged.
  actualFlags |= Qt::ItemIsDragEnabled;

  auto indexSourceFile = fromIndex(index);

  // Appended files/additional parts can only be dropped onto
  // non-appended files (meaning on model indexes that are valid) –
  // but only on top level items (meaning the parent index is
  // invalid).
  if ((m_appendedSelected | m_additionalPartSelected) && index.isValid() && !index.parent().isValid())
    actualFlags |= Qt::ItemIsDropEnabled;

  // Non-appended files can only be dropped onto the root note (whose
  // index isn't valid).
  else if (m_nonAppendedSelected && !index.isValid())
    actualFlags |= Qt::ItemIsDropEnabled;

  return actualFlags;
}

#define MIME_TYPE "application/x-mkvtoolnixgui-sourcefilemodelitems"

QStringList
SourceFileModel::mimeTypes()
  const {
  return QStringList{} << Q(MIME_TYPE);
}

QMimeData *
SourceFileModel::mimeData(QModelIndexList const &indexes)
  const {
  auto valuesToStore = QSet<quint64>{};

  for (auto const &index : indexes)
    if (index.isValid())
      valuesToStore << storageValueFromIndex(index);

  if (valuesToStore.isEmpty())
    return nullptr;

  auto data    = new QMimeData{};
  auto encoded = QByteArray{};

  QDataStream stream{&encoded, QIODevice::WriteOnly};

  mxinfo(boost::format("mimeData %1% entries\n") % valuesToStore.size());
  for (auto const &value : valuesToStore) {
    mxinfo(boost::format("  store %|1$08x|\n") % value);
    stream << value;
  }

  data->setData(MIME_TYPE, encoded);
  return data;
}

bool
SourceFileModel::dropMimeData(QMimeData const *data,
                              Qt::DropAction action,
                              int row,
                              int,
                              QModelIndex const &parent) {
  if (!data)
    return false;

  if (row > rowCount(parent))
    row = rowCount(parent);
  if (row == -1)
    row = rowCount(parent);

  if (data->hasFormat(MIME_TYPE))
    return dropSourceFiles(data, action, row, parent);

  return false;
}

QString
dumpIdx(QModelIndex const &idx,
        QString dumped = QString{}) {
  if (!idx.isValid())
    return dumped.isEmpty() ? "<invalid>" : dumped;

  return Q("%1/%2%3").arg(idx.row()).arg(idx.column()).arg(dumped.isEmpty() ? Q("") : Q(">%1").arg(dumped));
}

bool
SourceFileModel::dropSourceFiles(QMimeData const *data,
                                 Qt::DropAction action,
                                 int row,
                                 QModelIndex const &parent) {
  if (action != Qt::MoveAction)
    return QAbstractItemModel::dropMimeData(data, action, row, 0, parent);

  mxinfo(boost::format("dropMimeData row %1% parent %2%/%3%\n") % row % parent.row() % parent.column());

  auto encoded = data->data(MIME_TYPE);
  QDataStream stream{&encoded, QIODevice::ReadOnly};

  while (!stream.atEnd()) {
    quint64 value;
    stream >> value;
    auto sourceFile = m_sourceFileMap[value];
    auto sourceIdx  = indexFromSourceFile(sourceFile.get());

    if (!sourceIdx.isValid())
      continue;

    mxinfo(boost::format("  val %|1$08x| ptr %2% idx %3%/%4%\n") % value % sourceFile.get() % sourceIdx.row() % sourceIdx.column());

    auto sourceParent     = sourceIdx.parent();
    auto sourceParentItem = sourceParent.isValid() ? itemFromIndex(sourceParent) : invisibleRootItem();
    auto priorRow         = row;
    auto rowItems         = sourceParentItem->takeRow(sourceIdx.row());

    if (!parent.isValid()) {
      if ((sourceParent == parent) && (sourceIdx.row() < row))
        --row;

      mxinfo(boost::format("tried moving case 1 from %1% row %2% to new parent %3% row %4% new row %5%\n") % dumpIdx(sourceIdx.parent()) % sourceIdx.row() % dumpIdx(parent) % priorRow % (row + 1));

      invisibleRootItem()->insertRow(row, rowItems);
      ++row;

    } else {
      auto parentFile = fromIndex(parent);
      Q_ASSERT(parentFile);

      if (sourceFile->isAdditionalPart())
        row = std::min(row, parentFile->m_additionalParts.size());
      else
        row = std::max(row, parentFile->m_additionalParts.size());

      if ((sourceParent == parent) && (sourceIdx.row() < row))
        --row;

      mxinfo(boost::format("tried moving case 2 from %1% row %2% to new parent %3% row %4% new row %5%\n") % dumpIdx(sourceIdx.parent()) % sourceIdx.row() % dumpIdx(parent) % priorRow % (row + 1));

      itemFromIndex(parent)->insertRow(row, rowItems);
      ++row;
    }

    updateSourceFileLists();
  }

  return false;
}
