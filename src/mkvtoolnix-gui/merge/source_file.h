#ifndef MTX_MKVTOOLNIX_GUI_MERGE_SOURCE_FILE_H
#define MTX_MKVTOOLNIX_GUI_MERGE_SOURCE_FILE_H

#include "common/common_pch.h"

#include "common/file_types.h"
#include "common/qt.h"
#include "mkvtoolnix-gui/merge/mux_config.h"
#include "mkvtoolnix-gui/merge/track.h"

#include <QFileInfo>
#include <QList>
#include <QString>
#include <QVariant>

namespace mtx { namespace gui { namespace Merge {

class SourceFile;
using SourceFilePtr = std::shared_ptr<SourceFile>;

class SourceFile {
public:
  QVariantMap m_properties;
  QString m_fileName;
  QList<TrackPtr> m_tracks, m_attachedFiles;
  QList<SourceFilePtr> m_additionalParts, m_appendedFiles;
  QList<QFileInfo> m_playlistFiles;

  file_type_e m_type;
  bool m_appended, m_additionalPart, m_isPlaylist, m_dontScanForOtherPlaylists;
  SourceFile *m_appendedTo;

  uint64_t m_playlistDuration, m_playlistSize, m_playlistChapters;

  double m_probeRangePercentage;

public:
  explicit SourceFile(QString const &fileName = QString{""});
  SourceFile(SourceFile const &other);
  virtual ~SourceFile();

  SourceFile &operator =(SourceFile const &other);

  virtual QString container() const;
  virtual bool isTextSubtitleContainer() const;
  virtual bool isValid() const;
  virtual bool isRegular() const;
  virtual bool isAppended() const;
  virtual bool isAdditionalPart() const;
  virtual bool isPlaylist() const;
  virtual bool hasRegularTrack() const;

  virtual void saveSettings(Util::ConfigFile &settings) const;
  virtual void loadSettings(MuxConfig::Loader &l);
  virtual void fixAssociations(MuxConfig::Loader &l);
  virtual void setDefaults();

  virtual Track *findNthOrLastTrackOfType(Track::Type type, int nth) const;

  void buildMkvmergeOptions(QStringList &options) const;
};

}}}

#endif  // MTX_MKVTOOLNIX_GUI_SOURCE_FILE_H
