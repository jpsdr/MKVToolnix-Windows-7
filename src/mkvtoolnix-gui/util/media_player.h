#ifndef MTX_MKVTOOLNIX_GUI_UTIL_MEDIA_PLAYER_H
#define MTX_MKVTOOLNIX_GUI_UTIL_MEDIA_PLAYER_H

#include "common/common_pch.h"

#include <QObject>

namespace mtx { namespace gui { namespace Util {

class MediaPlayerPrivate;
class MediaPlayer : public QObject {
  Q_OBJECT;

protected:
  Q_DECLARE_PRIVATE(MediaPlayer);

  QScopedPointer<MediaPlayerPrivate> const d_ptr;

  explicit MediaPlayer(MediaPlayerPrivate &d);

public:
  MediaPlayer();
  virtual ~MediaPlayer();

  bool isPlaying() const;

public slots:
  void playFile(QString const &fileName, unsigned int volume);
  void stopPlayback();
};

}}}

#endif  // MTX_MKVTOOLNIX_GUI_UTIL_MEDIA_PLAYER_H
