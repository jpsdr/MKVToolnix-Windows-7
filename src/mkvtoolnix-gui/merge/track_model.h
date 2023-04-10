#pragma once

#include "common/common_pch.h"

#include "mkvtoolnix-gui/merge/source_file.h"

#include <QStandardItemModel>
#include <QIcon>
#include <QList>
#include <QSet>

class QItemSelectionModel;

namespace mtx::gui::Merge {

class TrackModel;
using TrackModelPtr = std::shared_ptr<TrackModel>;

class TrackModel : public QStandardItemModel {
  Q_OBJECT

public:
  static int const CodecColumn            =  0;
  static int const TypeColumn             =  1;
  static int const MuxThisColumn          =  2;
  static int const LanguageColumn         =  3;
  static int const NameColumn             =  4;
  static int const IDColumn               =  5;
  static int const DefaultTrackFlagColumn =  6;
  static int const ForcedTrackFlagColumn  =  7;
  static int const CharacterSetColumn     =  8;
  static int const PropertiesColumn       =  9;
  static int const SourceFileColumn       = 10;
  static int const SourceFileDirColumn    = 11;
  static int const ProgramColumn          = 12;
  static int const DelayColumn            = 13;

protected:
  QList<Track *> *m_tracks;
  QIcon m_audioIcon, m_videoIcon, m_subtitleIcon, m_attachmentIcon, m_chaptersIcon, m_tagsIcon, m_genericIcon, m_yesIcon, m_noIcon;
  bool m_ignoreTrackRemovals, m_nonAppendedSelected, m_appendedSelected, m_nonRegularSelected, m_appendedMultiParentsSelected, m_appendedMultiTypeSelected;
  TrackType m_selectedTrackType;

  debugging_option_c m_debug;

public:
  TrackModel(QObject *parent);
  virtual ~TrackModel();

  virtual void retranslateUi();

  virtual void setTracks(QList<Track *> &tracks);
  virtual void addTracks(QList<TrackPtr> const &tracks);
  virtual void addTrackAtAppropriatePlace(TrackPtr const &track, bool sortByType);
  virtual void appendTracks(SourceFile *fileToAppendTo, QList<TrackPtr> const &tracks);
  virtual void appendTracks(SourceFile *fileToAppendTo, QList<Track *> const &tracks);
  virtual void removeTrack(Track *trackToBeRemoved);
  virtual void removeTracks(QSet<Track *> const &tracks);
  virtual void reDistributeAppendedTracksForFileRemoval(QSet<SourceFile *> const &filesToRemove);

  virtual void moveTracksUpOrDown(QList<Track *> tracks, bool up);

  virtual void trackUpdated(Track *track);

  virtual Track *fromIndex(QModelIndex const &idx) const;
  virtual QModelIndex indexFromTrack(Track *track);
  virtual QStandardItem *itemFromTrack(Track *track);

  virtual Qt::DropActions supportedDropActions() const;
  virtual Qt::ItemFlags flags(QModelIndex const &index) const;

  virtual bool canDropMimeData(QMimeData const *data, Qt::DropAction action, int row, int column, QModelIndex const &parent) const override;
  virtual bool dropMimeData(QMimeData const *data, Qt::DropAction action, int row, int column, QModelIndex const &parent) override;

public Q_SLOTS:
  void updateTrackLists();
  void updateSelectionStatus();
  void updateTrackColors();

protected:
  QList<QStandardItem *> createRow(Track *track);
  void setItemsFromTrack(QList<QStandardItem *> items, Track *track);

  void dumpTracks(QString const &label) const;
  bool hasUnsetTrackRole(QModelIndex const &idx = QModelIndex{});

  void sortTracks(QList<Track *> &tracks, bool reverse = false);

  bool addTrackSortedByType(TrackPtr const &track);

  static QString summarizeProperties(Track const &track);
  static QString programInfoFor(Track const &track);
};

}
