#include "common/common_pch.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QItemSelectionModel>

#include "common/logger.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/merge/track_model.h"
#include "mkvtoolnix-gui/util/model.h"

namespace mtx { namespace gui { namespace Merge {

TrackModel::TrackModel(QObject *parent)
  : QStandardItemModel{parent}
  , m_tracks{}
  , m_audioIcon(":/icons/16x16/knotify.png")
  , m_videoIcon(":/icons/16x16/tool-animator.png")
  , m_subtitleIcon(":/icons/16x16/subtitles.png")
  , m_attachmentIcon(":/icons/16x16/mail-attachment.png")
  , m_chaptersIcon(":/icons/16x16/clock.png")
  , m_tagsIcon(":/icons/16x16/mail-tagged.png")
  , m_genericIcon(":/icons/16x16/application-octet-stream.png")
  , m_ignoreTrackRemovals{}
  , m_nonAppendedSelected{}
  , m_appendedSelected{}
  , m_nonRegularSelected{}
  , m_appendedMultiParentsSelected{}
  , m_appendedMultiTypeSelected{}
  , m_selectedTrackType{}
  , m_debug{"track_model"}
{
  connect(this, &TrackModel::rowsInserted, this, &TrackModel::updateTrackLists);
  connect(this, &TrackModel::rowsRemoved,  this, &TrackModel::updateTrackLists);
  connect(this, &TrackModel::rowsMoved,    this, &TrackModel::updateTrackLists);
}

TrackModel::~TrackModel() {
}

void
TrackModel::retranslateUi() {
  Util::setDisplayableAndSymbolicColumnNames(*this, {
    { QY("Codec"),                   Q("codec")            },
    { QY("Type"),                    Q("type")             },
    { QY("Copy item"),               Q("muxThis")          },
    { QY("Language"),                Q("language")         },
    { QY("Name"),                    Q("name")             },
    { QY("ID"),                      Q("id")               },
    { QY("Default track in output"), Q("defaultTrackFlag") },
    { QY("Forced track"),            Q("forcedTrackFlag")  },
    { QY("Character set"),           Q("characterSet")     },
    { QY("Properties"),              Q("properties")       },
    { QY("Source file"),             Q("sourceFile")       },
    { QY("Source file's directory"), Q("sourceFileDir")    },
    { QY("Program"),                 Q("program")          },
  });

  horizontalHeaderItem(IDColumn)->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

  for (auto const &track : *m_tracks) {
    trackUpdated(track);

    for (auto const &appendedTrack : track->m_appendedTracks)
      trackUpdated(appendedTrack);
  }
}

void
TrackModel::setTracks(QList<Track *> &tracks) {
  m_ignoreTrackRemovals = true;
  removeRows(0, rowCount());

  m_tracks = &tracks;
  auto row = 0u;

  for (auto const &track : *m_tracks) {
    invisibleRootItem()->appendRow(createRow(track));

    for (auto const &appendedTrack : track->m_appendedTracks)
      item(row)->appendRow(createRow(appendedTrack));

    ++row;
  }

  m_ignoreTrackRemovals = false;
}

QList<QStandardItem *>
TrackModel::createRow(Track *track) {
  auto items = QList<QStandardItem *>{};
  for (int idx = 0, numColumns = columnCount(); idx < numColumns; ++idx)
    items << new QStandardItem{};

  setItemsFromTrack(items, track);

  return items;
}

void
TrackModel::setItemsFromTrack(QList<QStandardItem *> items,
                              Track *track) {
  auto fileInfo = QFileInfo{ track->m_file->m_fileName };

  items[CodecColumn]           ->setText(track->isChapters() || track->isGlobalTags() || track->isTags() ? QNY("%1 entry", "%1 entries", track->m_size).arg(track->m_size) : track->m_codec);
  items[TypeColumn]            ->setText(track->nameForType());
  items[MuxThisColumn]         ->setText(track->m_muxThis ? QY("Yes") : QY("No"));
  items[LanguageColumn]        ->setText(track->isAppended() ? QString{} : track->m_language);
  items[NameColumn]            ->setText(track->isAppended() ? QString{} : track->m_name);
  items[IDColumn]              ->setText(-1 == track->m_id ? Q("") : QString::number(track->m_id));
  items[DefaultTrackFlagColumn]->setText(!track->m_effectiveDefaultTrackFlag ? Q("") : *track->m_effectiveDefaultTrackFlag ? QY("Yes") : QY("No"));
  items[ForcedTrackFlagColumn] ->setText(!track->isRegular()                 ? Q("") : track->m_forcedTrackFlag            ? QY("Yes") : QY("No"));
  items[CharacterSetColumn]    ->setText(!track->m_file->isTextSubtitleContainer() ? QString{} : track->m_characterSet);
  items[PropertiesColumn]      ->setText(summarizeProperties(*track));
  items[SourceFileColumn]      ->setText(fileInfo.fileName());
  items[SourceFileDirColumn]   ->setText(QDir::toNativeSeparators(fileInfo.path()));
  items[ProgramColumn]         ->setText(programInfoFor(*track));

  items[CodecColumn]->setData(QVariant::fromValue(reinterpret_cast<qulonglong>(track)), Util::TrackRole);
  items[CodecColumn]->setCheckable(true);
  items[CodecColumn]->setCheckState(track->m_muxThis ? Qt::Checked : Qt::Unchecked);

  items[TypeColumn] ->setIcon(  track->isAudio()      ? m_audioIcon
                              : track->isVideo()      ? m_videoIcon
                              : track->isSubtitles()  ? m_subtitleIcon
                              : track->isAttachment() ? m_attachmentIcon
                              : track->isChapters()   ? m_chaptersIcon
                              : track->isTags()       ? m_tagsIcon
                              : track->isGlobalTags() ? m_tagsIcon
                              :                         m_genericIcon);
  items[MuxThisColumn]         ->setIcon(track->m_muxThis                                                                    ? MainWindow::yesIcon() : MainWindow::noIcon());
  items[DefaultTrackFlagColumn]->setIcon(!track->m_effectiveDefaultTrackFlag ? QIcon{} : *track->m_effectiveDefaultTrackFlag ? MainWindow::yesIcon() : MainWindow::noIcon());
  items[ForcedTrackFlagColumn] ->setIcon(!track->isRegular()                 ? QIcon{} : track->m_forcedTrackFlag            ? MainWindow::yesIcon() : MainWindow::noIcon());

  items[IDColumn]->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

  Util::setItemForegroundColorDisabled(items, !track->m_muxThis);
}

Track *
TrackModel::fromIndex(QModelIndex const &idx)
  const {
  if (!idx.isValid())
    return nullptr;
  return reinterpret_cast<Track *>(index(idx.row(), 0, idx.parent())
                                   .data(Util::TrackRole)
                                   .value<qulonglong>());
}

QModelIndex
TrackModel::indexFromTrack(Track *track) {
  for (auto topRow = 0, numTopRows = rowCount(); topRow < numTopRows; ++topRow) {
    auto topIdx = index(topRow, 0);
    if (fromIndex(topIdx) == track)
      return topIdx;

    auto topItem = itemFromIndex(topIdx);
    if (!topItem)
      continue;

    for (auto childRow = 0, numChildRows = topItem->rowCount(); childRow < numChildRows; ++childRow) {
      auto childIdx = topIdx.child(childRow, 0);
      if (fromIndex(childIdx) == track)
        return childIdx;
    }
  }

  return {};
}

QStandardItem *
TrackModel::itemFromTrack(Track *track) {
  return itemFromIndex(indexFromTrack(track));
}

void
TrackModel::trackUpdated(Track *track) {
  Q_ASSERT(m_tracks);

  auto idx = indexFromTrack(track);
  if (!idx.isValid())
    return;

  auto items = QList<QStandardItem *>{};
  for (auto column = 0, numColumns = columnCount(); column < numColumns; ++column)
    items << itemFromIndex(idx.sibling(idx.row(), column));

  setItemsFromTrack(items, track);
}

void
TrackModel::addTracks(QList<TrackPtr> const &tracks) {
  for (auto &track : tracks)
    invisibleRootItem()->appendRow(createRow(track.get()));
}

void
TrackModel::appendTracks(SourceFile *fileToAppendTo,
                         QList<TrackPtr> const &tracks) {
  auto rawPtrTracks = QList<Track *>{};
  for (auto const &track : tracks)
    rawPtrTracks << track.get();

  appendTracks(fileToAppendTo, rawPtrTracks);
}

void
TrackModel::appendTracks(SourceFile *fileToAppendTo,
                         QList<Track *> const &tracks) {
  if (tracks.isEmpty())
    return;

  auto lastTrack = boost::accumulate(*m_tracks, static_cast<Track *>(nullptr), [](Track *accu, Track *t) { return t->isRegular() ? t : accu; });
  Q_ASSERT(!!lastTrack);

  auto trackOffsets = QHash<Track::Type, int>{};

  for (auto &newTrack : tracks) {
    // Things like tags, chapters and attachments aren't appended to a
    // specific track. Instead they're appended to the top list.
    if (!newTrack->isRegular()) {
      invisibleRootItem()->appendRow(createRow(newTrack));
      continue;
    }

    newTrack->m_appendedTo = fileToAppendTo->findNthOrLastTrackOfType(newTrack->m_type, trackOffsets[newTrack->m_type]++);
    if (!newTrack->m_appendedTo) {
      newTrack->m_appendedTo = lastTrack;
      newTrack->m_muxThis    = false;
    }

    auto row = m_tracks->indexOf(newTrack->m_appendedTo);
    Q_ASSERT(row != -1);
    item(row)->appendRow(createRow(newTrack));
  }
}

void
TrackModel::removeTrack(Track *trackToBeRemoved) {
  if (trackToBeRemoved->m_appendedTo) {
    auto row            = trackToBeRemoved->m_appendedTo->m_appendedTracks.indexOf(trackToBeRemoved);
    auto parentTrackRow = m_tracks->indexOf(trackToBeRemoved->m_appendedTo);

    Q_ASSERT((-1 != row) && (-1 != parentTrackRow));

    item(parentTrackRow)->removeRow(row);

    return;
  }

  auto row = m_tracks->indexOf(trackToBeRemoved);
  Q_ASSERT(-1 != row);

  invisibleRootItem()->removeRow(row);
}

void
TrackModel::removeTracks(QSet<Track *> const &tracks) {
  auto tracksToRemoveLast = QList<Track *>{};
  for (auto const &trackToBeRemoved : tracks)
    if (trackToBeRemoved->m_appendedTo)
      removeTrack(trackToBeRemoved);
    else
      tracksToRemoveLast << trackToBeRemoved;

  for (auto const &trackToBeRemoved : tracksToRemoveLast)
    if (!trackToBeRemoved->m_appendedTo)
      removeTrack(trackToBeRemoved);
}

void
TrackModel::dumpTracks(QString const &label)
  const {
  auto dumpIt = [](std::string const &prefix, Track const *track) {
    log_it(boost::format("%1%%2% : %3% : %4% : %5% : %6% : %7%:\n")
           % prefix
           % (track->isChapters() || track->isGlobalTags() || track->isTags() ? QNY("%1 entry", "%1 entries", track->m_size).arg(track->m_size) : track->m_codec)
           % track->nameForType()
           % (track->m_muxThis ? QY("Yes") : QY("No"))
           % track->m_language
           % track->m_name
           % QFileInfo{ track->m_file->m_fileName }.fileName());
  };

  log_it(boost::format("Dumping tracks %1%\n") % label);

  for (auto const &track : *m_tracks) {
    dumpIt("  ", track);
    for (auto const &appendedTrack : track->m_appendedTracks)
      dumpIt("    ", appendedTrack);
  }
}

bool
TrackModel::canDropMimeData(QMimeData const *data,
                            Qt::DropAction action,
                            int,
                            int,
                            QModelIndex const &parent)
  const {
  if (!data || (Qt::MoveAction != action))
    return false;

  // Reordering and therefore dragging non-regular tracks (chapters,
  // tags etc.) is not possible. Neither is dropping on them.
  if (m_nonRegularSelected)
    return false;

  // If both appended and non-appended tracks have been selected then
  // those cannot be dragged & dropped at the same time.
  if (m_nonAppendedSelected && m_appendedSelected)
    return false;

  // If multiple appended tracks have been selected that are appended
  // to different parents then those cannot be dragged & dropped at
  // the moment, as cannot multiple tracks of different types.
  if (m_appendedMultiParentsSelected || m_appendedMultiTypeSelected)
    return false;

  // No dropping inside appended tracks.
  if (parent.isValid() && parent.parent().isValid())
    return false;

  auto indexTrack = fromIndex(parent);
  // Appended tracks can only be dropped onto tracks of the same kind.
  if (m_appendedSelected && indexTrack && (m_selectedTrackType != indexTrack->m_type))
    return false;

  // Appended tracks can only be dropped onto non-appended tracks
  // (meaning on model indexes that are valid) – but only on top level
  // items (meaning the parent index is invalid).
  if (m_appendedSelected && !parent.isValid())
    return false;

  // Non-appended tracks can only be dropped onto the root note (whose
  // index isn't valid).
  if (m_nonAppendedSelected && parent.isValid())
    return false;

  return true;
}

bool
TrackModel::dropMimeData(QMimeData const *data,
                         Qt::DropAction action,
                         int row,
                         int column,
                         QModelIndex const &parent) {
  if (!canDropMimeData(data, action, row, column, parent))
    return false;

  auto isInside = (-1 == row) && (-1 == column);
  auto result   = QStandardItemModel::dropMimeData(data, action, isInside ? -1 : row, isInside ? -1 : 0, parent.isValid() ? parent.sibling(parent.row(), 0) : parent);

  Util::requestAllItems(*this);

  return result;
}

Qt::DropActions
TrackModel::supportedDropActions()
  const {
  return Qt::MoveAction;
}

Qt::ItemFlags
TrackModel::flags(QModelIndex const &index)
  const {
  auto actualFlags = QStandardItemModel::flags(index) & ~Qt::ItemIsDropEnabled & ~Qt::ItemIsDragEnabled;

  // Reordering and therefore dragging non-regular tracks (chapters,
  // tags etc.) is not possible. Neither is dropping on them.
  if (m_nonRegularSelected)
    return actualFlags;

  // If both appended and non-appended tracks have been selected then
  // those cannot be dragged & dropped at the same time.
  if (m_nonAppendedSelected && m_appendedSelected)
    return actualFlags;

  // If multiple appended tracks have been selected that are appended
  // to different parents then those cannot be dragged & dropped at
  // the moment, as cannot multiple tracks of different types.
  if (m_appendedMultiParentsSelected || m_appendedMultiTypeSelected)
    return actualFlags;

  // Everyting else can be at least dragged.
  actualFlags |= Qt::ItemIsDragEnabled;

  auto indexTrack = fromIndex(index);

  // Appended tracks can only be dropped onto tracks of the same kind.
  if (m_appendedSelected && indexTrack && (m_selectedTrackType != indexTrack->m_type))
    return actualFlags;

  // Appended tracks can only be dropped onto non-appended tracks
  // (meaning on model indexes that are valid) – but only on top level
  // items (meaning the parent index is invalid).
  if (m_appendedSelected && index.isValid() && !index.parent().isValid())
    actualFlags |= Qt::ItemIsDropEnabled;

  // Non-appended tracks can only be dropped onto the root note (whose
  // index isn't valid).
  else if (m_nonAppendedSelected && !index.isValid())
    actualFlags |= Qt::ItemIsDropEnabled;

  return actualFlags;
}

void
TrackModel::updateSelectionStatus() {
  m_nonAppendedSelected          = false;
  m_appendedSelected             = false;
  m_nonRegularSelected           = false;
  m_appendedMultiParentsSelected = false;
  m_appendedMultiTypeSelected    = false;
  m_selectedTrackType            = static_cast<Track::Type>(Track::TypeMax + 1);

  auto appendedParent            = static_cast<Track *>(nullptr);
  auto selectionModel            = qobject_cast<QItemSelectionModel *>(QObject::sender());
  Q_ASSERT(selectionModel);

  Util::withSelectedIndexes(selectionModel, [this,&appendedParent](QModelIndex const &selectedIndex) {
    auto track = fromIndex(selectedIndex);
    if (!track)
      return;

    if (!track->isRegular())
      m_nonRegularSelected = true;

    else if (!track->isAppended())
      m_nonAppendedSelected = true;

    else {
      if (appendedParent && (appendedParent != track->m_appendedTo))
        m_appendedMultiParentsSelected = true;

      if ((static_cast<int>(m_selectedTrackType) != (Track::TypeMax + 1)) && (m_selectedTrackType != track->m_type))
        m_appendedMultiTypeSelected = true;

      appendedParent      = track->m_appendedTo;
      m_selectedTrackType = track->m_type;
      m_appendedSelected  = true;
    }
  });
}

void
TrackModel::updateTrackLists() {
  if (m_ignoreTrackRemovals || hasUnsetTrackRole())
    return;

  for (auto const &track : *m_tracks)
    track->m_appendedTracks.clear();

  m_tracks->clear();

  for (auto row = 0, numRows = rowCount(); row < numRows; ++row) {
    auto idx   = index(row, 0, QModelIndex{});
    auto track = fromIndex(idx);

    Q_ASSERT(track);

    *m_tracks << track;

    for (auto appendedRow = 0, numAppendedRows = rowCount(idx); appendedRow < numAppendedRows; ++appendedRow) {
      auto appendedTrack = fromIndex(index(appendedRow, 0, idx));
      Q_ASSERT(appendedTrack);

      appendedTrack->m_appendedTo = track;
      track->m_appendedTracks << appendedTrack;
    }
  }

  if (m_debug) {
    log_it(boost::format("### AFTER drag & drop ###\n"));
    MuxConfig::debugDumpSpecificTrackList(*m_tracks);
  }

  updateEffectiveDefaultTrackFlags();
}

bool
TrackModel::hasUnsetTrackRole(QModelIndex const &idx) {
  if (idx.isValid() && !fromIndex(idx))
    return true;

  for (auto row = 0, numRows = rowCount(idx); row < numRows; ++row)
    if (hasUnsetTrackRole(index(row, 0, idx)))
      return true;

  return false;
}

void
TrackModel::reDistributeAppendedTracksForFileRemoval(QSet<SourceFile *> const &filesToRemove) {
  auto tracksToRedistribute = QList<Track *>{};

  for (auto const &track : *m_tracks) {
    for (auto const &appendedTrack : track->m_appendedTracks) {
      if (filesToRemove.contains(appendedTrack->m_file))
        continue;

      if (filesToRemove.contains(appendedTrack->m_appendedTo->m_file))
        tracksToRedistribute << appendedTrack;
    }
  }

  if (tracksToRedistribute.isEmpty())
    return;

  removeTracks(tracksToRedistribute.toSet());

  Q_ASSERT(!m_tracks->isEmpty());

  appendTracks(m_tracks->at(0)->m_file, tracksToRedistribute);
}

void
TrackModel::sortTracks(QList<Track *> &tracks,
                       bool reverse) {
  auto rows = QHash<Track *, int>{};

  for (auto const &track : tracks)
    rows[track] = indexFromTrack(track).row();

  std::sort(tracks.begin(), tracks.end(), [&rows](Track *a, Track *b) -> bool {
    auto rowA = rows[a];
    auto rowB = rows[b];

    if (!a->m_appendedTo && !b->m_appendedTo)
      return rowA < rowB;

    if (!a->m_appendedTo &&  b->m_appendedTo)
      return true;

    if ( a->m_appendedTo && !b->m_appendedTo)
      return false;

    auto parentA = rows[a->m_appendedTo];
    auto parentB = rows[b->m_appendedTo];

    return (parentA < parentB)
        || ((parentA == parentB) && (rowA < rowB));
  });

  if (reverse) {
    std::reverse(tracks.begin(), tracks.end());
    std::stable_partition(tracks.begin(), tracks.end(), [](Track *track) { return track->isRegular() && !track->isAppended(); });
  }
}

void
TrackModel::moveTracksUpOrDown(QList<Track *> tracks,
                               bool up) {
  sortTracks(tracks, !up);

  // qDebug() << "move up?" << up << "tracks" << tracks;

  auto couldNotBeMoved = QHash<Track *, bool>{};
  auto isSelected      = QHash<Track *, bool>{};
  auto const direction = up ? -1 : +1;
  auto const topRows   = rowCount();

  for (auto const &track : tracks) {
    isSelected[track] = true;

    if (!track->isRegular())
      continue;

    if (track->isAppended() && isSelected[track->m_appendedTo])
      continue;

    auto idx = indexFromTrack(track);
    Q_ASSERT(idx.isValid());

    auto targetRow = idx.row() + direction;
    if (couldNotBeMoved[fromIndex(idx.sibling(targetRow, 0))]) {
      couldNotBeMoved[track] = true;
      continue;
    }

    if (!track->isAppended()) {
      if (!((0 <= targetRow) && (targetRow < topRows))) {
        couldNotBeMoved[track] = true;
        continue;
      }

      // qDebug() << "top level: would like to move" << idx.row() << "to" << targetRow;

      insertRow(targetRow, takeRow(idx.row()));

      continue;
    }

    auto parentItem         = itemFromIndex(idx.parent());
    auto const appendedRows = parentItem->rowCount();

    if ((0 <= targetRow) && (targetRow < appendedRows)) {
      // qDebug() << "appended level normal: would like to move" << idx.row() << "to" << targetRow;

      parentItem->insertRow(targetRow, parentItem->takeRow(idx.row()));
      continue;
    }

    auto parentIdx = indexFromTrack(track->m_appendedTo);
    Q_ASSERT(parentIdx.isValid());

    for (auto row = parentIdx.row() + direction; (0 <= row) && (row < topRows); row += direction) {
      auto otherParent = fromIndex(index(row, 0));
      Q_ASSERT(!!otherParent);

      if (otherParent->m_type != track->m_type)
        continue;

      auto otherParentItem = itemFromIndex(index(row, 0));
      auto rowItems        = parentItem->takeRow(idx.row());
      targetRow            = up ? otherParentItem->rowCount() : 0;

      // qDebug() << "appended level cross: would like to move" << idx.row() << "from" << track->m_appendedTo << "to" << otherParent << "as" << targetRow;

      otherParentItem->insertRow(targetRow, rowItems);
      track->m_appendedTo = otherParent;

      break;
    }
  }

  updateTrackLists();
}

void
TrackModel::updateEffectiveDefaultTrackFlags() {
  if (!m_tracks)
    return;

  auto isSet                = QHash<Track::Type, bool>{};
  auto regularEnabledTracks = QList<Track *>{};

  std::copy_if(m_tracks->begin(), m_tracks->end(), std::back_inserter(regularEnabledTracks),
               [](Track const *track) { return track->isRegular() && track->m_muxThis; });

  // Step one: reset all flags to undefined. Do this for all tracks,
  // not just for regular & enabled ones.
  for (auto &track : *m_tracks)
    if (track->m_muxThis)
      track->m_effectiveDefaultTrackFlag = boost::none;
    else
      track->m_effectiveDefaultTrackFlag = false;

  // Step two: check for explicitly set flags (set to yes/no). These
  // take precedence over everything else.
  for (auto &track : regularEnabledTracks) {
    if (isSet[track->m_type] || (2 == track->m_defaultTrackFlag))
      track->m_effectiveDefaultTrackFlag = false;

    else if (1 == track->m_defaultTrackFlag) {
      isSet[track->m_type]               = true;
      track->m_effectiveDefaultTrackFlag = true;
    }
  }

  // Step three: for tracks without explicit settings (determine
  // automatically) see if there are tracks which have their default
  // track flag set their source container. If it was set to "no" keep
  // it that way. For "yes" make sure not to set it to "yes" for
  // others, too.
  for (auto &track : regularEnabledTracks) {
    if (track->m_effectiveDefaultTrackFlag || !track->m_defaultTrackFlagWasPresent)
      continue;

    if (!track->m_defaultTrackFlagWasSet)
      track->m_effectiveDefaultTrackFlag = false;

    else if (!isSet[track->m_type]) {
      isSet[track->m_type]               = true;
      track->m_effectiveDefaultTrackFlag = true;
    }
  }

  // Step four: for track types for which no default track has been
  // set yet the first track of its type will have it set. This has
  // the lowest precedence.
  for (auto &track : regularEnabledTracks) {
    if (track->m_effectiveDefaultTrackFlag)
      continue;

    if (!isSet[track->m_type]) {
      isSet[track->m_type]               = true;
      track->m_effectiveDefaultTrackFlag = true;

    } else
      track->m_effectiveDefaultTrackFlag = false;
  }

  for (auto &track : *m_tracks)
    trackUpdated(track);
}

QString
TrackModel::summarizeProperties(Track const &track) {
  auto properties = QStringList{};

  if (track.isAudio()) {
    if (track.isPropertySet("audio_sampling_frequency"))
      properties << QY("%1 Hz").arg(track.m_properties.value("audio_sampling_frequency").toUInt());

    if (track.isPropertySet("audio_channels")) {
      auto channels = track.m_properties.value("audio_channels").toUInt();
      properties << QNY("%1 channel", "%1 channels", channels).arg(channels);
    }

    if (track.isPropertySet("audio_bits_per_sample")) {
      auto bitsPerSample = track.m_properties.value("audio_bits_per_sample").toUInt();
      properties << QNY("%1 bit per sample", "%1 bits per sample", bitsPerSample).arg(bitsPerSample);
    }

  } else if (track.isVideo()) {
    if (track.isPropertySet("pixel_dimensions"))
      properties << QY("%1 pixels").arg(track.m_properties.value("pixel_dimensions").toString());
  }

  return properties.join(Q(", "));
}

QString
TrackModel::programInfoFor(Track const &track) {
  if (!track.m_properties.contains(Q("program_number")))
    return {};

  auto programNumber = track.m_properties[Q("program_number")].toUInt();

  if (track.m_file->m_programMap.contains(programNumber))
    return track.m_file->m_programMap[programNumber].m_serviceName;

  return QString::number(programNumber);
}

}}}
