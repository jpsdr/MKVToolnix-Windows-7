#pragma once

#include "common/common_pch.h"

#include "common/bluray/disc_library.h"
#include "common/file_types.h"
#include "common/qt.h"
#include "mkvtoolnix-gui/merge/mux_config.h"
#include "mkvtoolnix-gui/merge/track.h"

#include <QFileInfo>
#include <QList>
#include <QString>
#include <QVariant>

namespace mtx::gui::Merge {

class SourceFile;
using SourceFilePtr = std::shared_ptr<SourceFile>;

class SourceFile {
public:
  struct Program {
    QString m_serviceProvider, m_serviceName;
  };

public:
  QVariantMap m_properties;
  QHash<unsigned int, Program> m_programMap;
  QString m_fileName;
  QList<TrackPtr> m_tracks, m_attachedFiles;
  QList<SourceFilePtr> m_additionalParts, m_appendedFiles;
  QList<QFileInfo> m_playlistFiles;

  mtx::file_type_e m_type{mtx::file_type_e::is_unknown};
  bool m_appended{}, m_additionalPart{}, m_isPlaylist{}, m_dontScanForOtherPlaylists{};
  SourceFile *m_appendedTo{};

  uint64_t m_playlistDuration{}, m_playlistSize{}, m_playlistChapters{};

  double m_probeRangePercentage{0.3};

  std::optional<mtx::bluray::disc_library::info_t> m_discLibraryInfoToAdd;
  bool m_discLibraryInfoSelected{};

public:
  explicit SourceFile(QString const &fileName = QString{""});
  SourceFile(SourceFile const &other) = default;
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
  virtual bool hasAudioTrack() const;
  virtual bool hasSubtitlesTrack() const;
  virtual bool hasVideoTrack() const;

  virtual void saveSettings(Util::ConfigFile &settings) const;
  virtual void loadSettings(MuxConfig::Loader &l);
  virtual void fixAssociations(MuxConfig::Loader &l);
  virtual void setDefaults();
  virtual void setupProgramMapFromProperties();

  virtual Track *findNthOrLastTrackOfType(TrackType type, int nth) const;

  void buildMkvmergeOptions(QStringList &options) const;

protected:
  virtual mtx::bcp47::language_c deriveLanguageFromFileName();
};

}
