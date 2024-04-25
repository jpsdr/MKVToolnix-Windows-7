#include "common/common_pch.h"

#include <QByteArray>
#include <QDataStream>
#include <QDir>
#include <QFileInfo>
#include <QItemSelectionModel>
#include <QMimeData>
#include <QPainter>
#include <QPixmap>
#include <QRegularExpression>

#include "common/logger.h"
#include "common/sequenced_file_names.h"
#include "common/sorting.h"
#include "common/strings/formatting.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/mime_types.h"
#include "mkvtoolnix-gui/merge/attached_file_model.h"
#include "mkvtoolnix-gui/merge/source_file_model.h"
#include "mkvtoolnix-gui/merge/track_model.h"
#include "mkvtoolnix-gui/util/container.h"
#include "mkvtoolnix-gui/util/model.h"
#include "mkvtoolnix-gui/util/settings.h"

namespace mtx::gui::Merge {

namespace {

QIcon
createSourceIndicatorIcon(SourceFile &sourceFile) {
  auto iconName = sourceFile.isAdditionalPart() ? Q("distribute-horizontal-margin")
                : sourceFile.isAppended()       ? Q("distribute-horizontal-x")
                :                                 Q("distribute-vertical-page");
  auto icon     = QIcon::fromTheme(iconName);

  if (!Util::Settings::get().m_mergeUseFileAndTrackColors)
    return Util::fixStandardItemIcon(icon);

  auto color = Util::Settings::get().nthFileColor(sourceFile.m_colorIndex);

  QPixmap combinedPixmap{28, 16};
  combinedPixmap.fill(Qt::transparent);

  QPainter painter{&combinedPixmap};

  painter.drawPixmap(0, 0, icon.pixmap(16, 16));

  painter.setPen(color);
  painter.setBrush(color);
  painter.drawRect(20, 2, 8, 12);

  QIcon combinedIcon;
  combinedIcon.addPixmap(combinedPixmap);

  return combinedIcon;
}

int
insertPriorityForSourceFile(SourceFile const &file) {
  return file.hasVideoTrack()     ? 0
       : file.hasAudioTrack()     ? 1
       : file.hasSubtitlesTrack() ? 2
       :                            3;
}

} // anonymous namespace

SourceFileModel::SourceFileModel(QObject *parent)
  : QStandardItemModel{parent}
  , m_sourceFiles{}
  , m_tracksModel{}
  , m_attachedFilesModel{}
  , m_nonAppendedSelected{}
  , m_appendedSelected{}
  , m_additionalPartSelected{}
{
  initializeColorIndexes();

  connect(MainWindow::get(), &MainWindow::preferencesChanged, this, &SourceFileModel::updateFileColors);
}

SourceFileModel::~SourceFileModel() {
}

void
SourceFileModel::retranslateUi() {
  Util::setDisplayableAndSymbolicColumnNames(*this, {
    { QY("File name"), Q("fileName")  },
    { QY("Container"), Q("container") },
    { QY("File size"), Q("fileSize")  },
    { QY("Directory"), Q("directory") },
  });

  horizontalHeaderItem(2)->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

  if (!m_sourceFiles)
    return;

  for (auto const &sourceFile : *m_sourceFiles) {
    sourceFileUpdated(sourceFile.get());

    for (auto const &additionalPart : sourceFile->m_additionalParts)
      sourceFileUpdated(additionalPart.get());

    for (auto const &appendedFile : sourceFile->m_appendedFiles)
      sourceFileUpdated(appendedFile.get());
  }
}

void
SourceFileModel::setOtherModels(TrackModel *tracksModel,
                                AttachedFileModel *attachedFilesModel) {
  m_tracksModel        = tracksModel;
  m_attachedFilesModel = attachedFilesModel;
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

  initializeColorIndexes();

  m_sourceFiles = &sourceFiles;
  auto row      = 0u;

  for (auto const &file : *m_sourceFiles) {
    assignColorIndex(*file);

    createAndAppendRow(invisibleRootItem(), file);

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
  for (int idx = 0; idx < columnCount(); ++idx)
    items << new QStandardItem{};

  setItemsFromSourceFile(items, sourceFile);

  return items;
}

void
SourceFileModel::setItemsFromSourceFile(QList<QStandardItem *> const &items,
                                        SourceFile *sourceFile)
  const {
  auto info = QFileInfo{sourceFile->m_fileName};

  items[0]->setText(info.fileName());
  items[1]->setText(sourceFile->isAdditionalPart() ? QY("(additional part)") : sourceFile->container());
  items[2]->setText(to_qs(mtx::string::format_file_size(sourceFile->isPlaylist() ? sourceFile->m_playlistSize : info.size())));
  items[3]->setText(QDir::toNativeSeparators(info.path()));

  items[0]->setData(reinterpret_cast<quint64>(sourceFile), Util::SourceFileRole);
  items[0]->setIcon(createSourceIndicatorIcon(*sourceFile));

  items[2]->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
}

void
SourceFileModel::sourceFileUpdated(SourceFile *sourceFile) {
  auto idx = indexFromSourceFile(sourceFile);
  if (!idx.isValid())
    return;

  auto items = QList<QStandardItem *>{};

  for (auto column = 0, numColumns = columnCount(); column < numColumns; ++column)
    items << itemFromIndex(idx.sibling(idx.row(), column));

  setItemsFromSourceFile(items, sourceFile);
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
SourceFileModel::addAdditionalParts(QStringList const &fileNames,
                                    QModelIndex const &fileToAddToIdx) {
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

    createAndAppendRow(itemToAddTo, additionalPart, fileToAddTo->m_additionalParts.size());

    fileToAddTo->m_additionalParts << additionalPart;
  }
}

void
SourceFileModel::addOrAppendFilesAndTracks(QVector<SourceFilePtr> const &files,
                                           QModelIndex const &fileToAddToIdx,
                                           bool append) {
  Q_ASSERT(m_tracksModel);

  if (files.isEmpty())
    return;

  for (auto const &file : files)
    assignColorIndex(*file);

  if (append)
    appendFilesAndTracks(files, fileToAddToIdx);
  else
    addFilesAndTracks(files);
}

QModelIndex
SourceFileModel::addFileSortedByType(SourceFilePtr const &file) {
  auto newFilePrio = insertPriorityForSourceFile(*file);

  for (int idx = 0, numFiles = m_sourceFiles->size(); idx < numFiles; ++idx) {
    auto existingFilePrio = insertPriorityForSourceFile(*m_sourceFiles->at(idx));

    if (existingFilePrio <= newFilePrio)
      continue;

    m_sourceFileMap[reinterpret_cast<quint64>(file.get())] = file;
    auto row                                               = createRow(file.get());

    invisibleRootItem()->insertRow(idx, row);
    m_sourceFiles->insert(idx, file);

    return index(idx, 0);
  }

  return {};
}

QModelIndex
SourceFileModel::addFileAtAppropriatePlace(SourceFilePtr const &file,
                                           bool sortByType) {
  QModelIndex insertPosIdx;

  if (sortByType) {
    insertPosIdx = addFileSortedByType(file);

    if (insertPosIdx.isValid())
      return insertPosIdx;
  }

  createAndAppendRow(invisibleRootItem(), file);
  *m_sourceFiles << file;

  return index(rowCount() - 1, 0);
}

void
SourceFileModel::addFilesAndTracks(QVector<SourceFilePtr> const &files) {
  std::optional<mtx::sequenced_file_names::SequencedFileNameData> previouslyAddedSequenceData;
  QModelIndex previouslyAddedPosition;

  auto &cfg                 = Util::Settings::get();
  auto sortByType           = cfg.m_mergeSortFilesTracksByTypeWhenAdding;
  auto reconstructSequences = cfg.m_mergeReconstructSequencesWhenAdding;
  auto filesToProcess       = files;

  if (reconstructSequences)
    mtx::sort::by(filesToProcess.begin(), filesToProcess.end(), [](auto const &file) {
      return mtx::sort::natural_string_c(file->m_fileName);
    });

  for (auto const &file : filesToProcess) {
    auto sequenceData = mtx::sequenced_file_names::analyzeFileNameForSequenceData(file->m_fileName);

    if (   reconstructSequences
        && previouslyAddedSequenceData
        && previouslyAddedPosition.isValid()
        && sequenceData
        && sequenceData->follows(*previouslyAddedSequenceData)) {
      appendFilesAndTracks({ file }, previouslyAddedPosition);
      previouslyAddedSequenceData = sequenceData;

      continue;
    }

    previouslyAddedPosition     = addFileAtAppropriatePlace(file, sortByType);
    previouslyAddedSequenceData = sequenceData;

    for (auto const &track : file->m_tracks)
      m_tracksModel->addTrackAtAppropriatePlace(track, sortByType);

    if (file->m_additionalParts.isEmpty())
      continue;

    auto itemToAddTo = item(rowCount() - 1, 0);
    auto row         = 0;
    for (auto const &additionalPart : file->m_additionalParts)
      createAndAppendRow(itemToAddTo, additionalPart, row++);
  }

  QList<TrackPtr> attachedFiles;
  for (auto const &file : files)
    attachedFiles << file->m_attachedFiles;

  m_attachedFilesModel->addAttachedFiles(attachedFiles);
}

void
SourceFileModel::removeFile(SourceFile *fileToBeRemoved) {
  m_availableColorIndexes.prepend(fileToBeRemoved->m_colorIndex);

  m_sourceFileMap.remove(reinterpret_cast<quint64>(fileToBeRemoved));

  if (fileToBeRemoved->isAdditionalPart()) {
    auto row = -1, parentFileRow = -1;
    auto numParentRows = m_sourceFiles->count();

    for (parentFileRow = 0; parentFileRow < numParentRows; ++parentFileRow) {
      row = Util::findPtr(fileToBeRemoved, (*m_sourceFiles)[parentFileRow]->m_additionalParts);
      if (row != -1)
        break;
    }

    Q_ASSERT((-1 != row) && (-1 != parentFileRow));

    item(parentFileRow)->removeRow(row);
    (*m_sourceFiles)[parentFileRow]->m_additionalParts.removeAt(row);

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
  auto filesToRemove  = Util::qListToSet(files);
  auto tracksToRemove = QSet<Track *>{};
  auto attachedFiles  = QList<TrackPtr>{};

  for (auto const &file : files) {
    for (auto const &track : file->m_tracks)
      tracksToRemove << track.get();

    attachedFiles += file->m_attachedFiles;

    for (auto const &appendedFile : file->m_appendedFiles) {
      filesToRemove << appendedFile.get();
      for (auto const &track : appendedFile->m_tracks)
        tracksToRemove << track.get();

      attachedFiles += appendedFile->m_attachedFiles;
    }
  }

  m_tracksModel->reDistributeAppendedTracksForFileRemoval(filesToRemove);
  m_tracksModel->removeTracks(tracksToRemove);
  m_attachedFilesModel->removeAttachedFiles(attachedFiles);

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
SourceFileModel::appendFilesAndTracks(QVector<SourceFilePtr> const &files,
                                      QModelIndex const &fileToAppendToIdx) {
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
    if (!sourceFile)
      return;

    if (sourceFile->isRegular())
      m_nonAppendedSelected = true;

    else if (sourceFile->isAppended())
      m_appendedSelected = true;

    else if (sourceFile->isAdditionalPart())
      m_additionalPartSelected = true;
  });
}

void
SourceFileModel::dumpSourceFiles(QString const &label)
  const {
  auto dumpIt = [](std::string const &prefix, SourceFilePtr const &sourceFile) {
    log_it(fmt::format("{0}{1}\n", prefix, sourceFile->m_fileName));
  };

  log_it(fmt::format("Dumping source files {0}\n", label));

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

QStringList
SourceFileModel::mimeTypes()
  const {
  return QStringList{} << mtx::gui::MimeTypes::MergeSourceFileModelItem;
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

  for (auto const &value : valuesToStore)
    stream << value;

  data->setData(mtx::gui::MimeTypes::MergeSourceFileModelItem, encoded);
  return data;
}

bool
SourceFileModel::canDropMimeData(QMimeData const *data,
                                 Qt::DropAction action,
                                 int,
                                 int,
                                 QModelIndex const &parent)
  const {
  if (   !data
      || !data->hasFormat(mtx::gui::MimeTypes::MergeSourceFileModelItem)
      || (Qt::MoveAction != action))
    return false;

  // If both appended files/additional parts and non-appended files
  // have been selected then those cannot be dragged & dropped at the
  // same time.
  if (m_nonAppendedSelected && (m_appendedSelected | m_additionalPartSelected))
    return false;

  // No drag & drop inside appended/additional parts, please.
  if (parent.isValid() && parent.parent().isValid())
    return false;

  // Appended files/additional parts can only be dropped onto
  // non-appended files (meaning on model indexes that are valid) –
  // but only on top level items (meaning the parent index is
  // invalid).
  if ((m_appendedSelected | m_additionalPartSelected) && !parent.isValid())
    return false;

  // Non-appended files can only be dropped onto the root note (whose
  // index isn't valid).
  if (m_nonAppendedSelected && parent.isValid())
    return false;

  return true;
}

bool
SourceFileModel::dropMimeData(QMimeData const *data,
                              Qt::DropAction action,
                              int row,
                              int column,
                              QModelIndex const &parent) {
  if (!canDropMimeData(data, action, row, column, parent))
    return false;

  if (row > rowCount(parent))
    row = rowCount(parent);
  if (row == -1)
    row = rowCount(parent);

  auto result = dropSourceFiles(data, action, row, parent.isValid() ? parent.sibling(parent.row(), 0) : parent);

  Util::requestAllItems(*this);

  return result;
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

  auto encoded = data->data(mtx::gui::MimeTypes::MergeSourceFileModelItem);
  QDataStream stream{&encoded, QIODevice::ReadOnly};

  while (!stream.atEnd()) {
    quint64 value;
    stream >> value;
    auto sourceFile = m_sourceFileMap[value];
    auto sourceIdx  = indexFromSourceFile(sourceFile.get());

    if (!sourceIdx.isValid())
      continue;

    auto sourceParent     = sourceIdx.parent();
    auto sourceParentItem = sourceParent.isValid() ? itemFromIndex(sourceParent) : invisibleRootItem();
    auto rowItems         = sourceParentItem->takeRow(sourceIdx.row());

    if (!parent.isValid()) {
      if ((sourceParent == parent) && (sourceIdx.row() < row))
        --row;

      invisibleRootItem()->insertRow(row, rowItems);
      ++row;

    } else {
      auto parentFile = fromIndex(parent);
      Q_ASSERT(parentFile);

      if (sourceFile->isAdditionalPart())
        row = std::min<int>(row, parentFile->m_additionalParts.size());
      else
        row = std::max<int>(row, parentFile->m_additionalParts.size());

      if ((sourceParent == parent) && (sourceIdx.row() < row))
        --row;

      itemFromIndex(parent)->insertRow(row, rowItems);
      ++row;
    }

    updateSourceFileLists();
  }

  return false;
}

void
SourceFileModel::sortSourceFiles(QList<SourceFile *> &files,
                                 bool reverse) {
  auto rows = QHash<SourceFile *, int>{};

  for (auto const &file : files)
    rows[file] = indexFromSourceFile(file).row();

  std::sort(files.begin(), files.end(), [&rows](SourceFile *a, SourceFile *b) -> bool {
    auto rowA = rows[a];
    auto rowB = rows[b];

    if ( a->isRegular() &&  b->isRegular())
      return rowA < rowB;

    if ( a->isRegular() && !b->isRegular())
      return true;

    if (!a->isRegular() &&  b->isRegular())
      return false;

    auto parentA = rows[a->m_appendedTo];
    auto parentB = rows[b->m_appendedTo];

    return (parentA < parentB)
        || ((parentA == parentB) && (rowA < rowB));
  });

  if (reverse) {
    std::reverse(files.begin(), files.end());
    std::stable_partition(files.begin(), files.end(), [](SourceFile *file) { return file->isRegular(); });
  }
}

std::pair<int, int>
SourceFileModel::countAppendedAndAdditionalParts(QStandardItem *parentItem) {
  auto numbers = std::make_pair(0, 0);

  for (auto row = 0, numRows = parentItem->rowCount(); row < numRows; ++row) {
    auto sourceFile = fromIndex(parentItem->child(row)->index());
    Q_ASSERT(!!sourceFile);

    if (sourceFile->isAdditionalPart())
      ++numbers.first;
    else
      ++numbers.second;
  }

  return numbers;
}

void
SourceFileModel::moveSourceFilesUpOrDown(QList<SourceFile *> files,
                                         bool up) {
  sortSourceFiles(files, !up);

  // qDebug() << "move up?" << up << "files" << files;

  auto couldNotBeMoved = QHash<SourceFile *, bool>{};
  auto isSelected      = QHash<SourceFile *, bool>{};
  auto const direction = up ? -1 : +1;
  auto const topRows   = rowCount();

  for (auto const &file : files) {
    isSelected[file] = true;

    if (!file->isRegular() && isSelected[file->m_appendedTo])
      continue;

    auto idx = indexFromSourceFile(file);
    Q_ASSERT(idx.isValid());

    auto targetRow = idx.row() + direction;
    if (couldNotBeMoved[fromIndex(idx.sibling(targetRow, 0)).get()]) {
      couldNotBeMoved[file] = true;
      continue;
    }

    if (file->isRegular()) {
      if (!((0 <= targetRow) && (targetRow < topRows))) {
        couldNotBeMoved[file] = true;
        continue;
      }

      // qDebug() << "top level: would like to move" << idx.row() << "to" << targetRow;

      insertRow(targetRow, takeRow(idx.row()));

      continue;
    }

    auto parentItem                   = itemFromIndex(idx.parent());
    auto const appendedAdditionalRows = countAppendedAndAdditionalParts(parentItem);
    auto const additionalPartsRows    = appendedAdditionalRows.first;
    auto const appendedRows           = appendedAdditionalRows.second;
    auto const lowerLimit             = (file->isAdditionalPart() ? 0 : additionalPartsRows);
    auto const upperLimit             = (file->isAdditionalPart() ? 0 : appendedRows) +  additionalPartsRows;

    if ((lowerLimit <= targetRow) && (targetRow < upperLimit)) {
      // qDebug() << "appended level normal: would like to move" << idx.row() << "to" << targetRow;

      parentItem->insertRow(targetRow, parentItem->takeRow(idx.row()));
      continue;
    }

    auto parentIdx = parentItem->index();
    Q_ASSERT(parentIdx.isValid());

    auto newParentRow = parentIdx.row() + direction;
    if ((0 > newParentRow) || (rowCount() <= newParentRow)) {
      // qDebug() << "appended, cannot move further";
      couldNotBeMoved[file] = true;
      continue;
    }

    auto newParent        = fromIndex(index(newParentRow, 0));
    auto newParentItem    = itemFromIndex(index(newParentRow, 0));
    auto rowItems         = parentItem->takeRow(idx.row());
    auto newParentNumbers = countAppendedAndAdditionalParts(newParentItem);
    targetRow             = up  && file->isAdditionalPart() ? newParentNumbers.first
                          : up                              ? newParentNumbers.first + newParentNumbers.second
                          : !up && file->isAdditionalPart() ? 0
                          :                                   newParentNumbers.first;

    Q_ASSERT(!!newParent);

    // qDebug() << "appended level cross: would like to move" << idx.row() << "from" << file->m_appendedTo << "to" << newParent.get() << "as" << targetRow;

    newParentItem->insertRow(targetRow, rowItems);
    file->m_appendedTo = newParent.get();
  }

  updateSourceFileLists();
}

void
SourceFileModel::initializeColorIndexes() {
  m_availableColorIndexes.clear();
  m_nextColorIndex = 0;
}

void
SourceFileModel::assignColorIndex(SourceFile &file) {
  if (m_availableColorIndexes.isEmpty())
    m_availableColorIndexes << m_nextColorIndex++;

  file.m_colorIndex = m_availableColorIndexes.front();

  for (auto const &track : file.m_tracks)
    track->m_colorIndex = file.m_colorIndex;

  m_availableColorIndexes.removeFirst();

  for (auto const &additionalPart : file.m_additionalParts)
    assignColorIndex(*additionalPart);

  for (auto const &appendedFile : file.m_appendedFiles)
    assignColorIndex(*appendedFile);
}

void
SourceFileModel::updateFileColors() {
  Util::walkTree(*this, {}, [this](auto const &idx) {
    auto item       = itemFromIndex(idx);
    auto sourceFile = fromIndex(idx);

    if (item && sourceFile)
      item->setIcon(createSourceIndicatorIcon(*sourceFile));
  });
}

}
