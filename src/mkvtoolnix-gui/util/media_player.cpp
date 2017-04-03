#include "common/common_pch.h"

#include <QMediaPlayer>

#include "mkvtoolnix-gui/util/media_player.h"

namespace mtx { namespace gui { namespace Util {

using namespace mtx::gui;

class MediaPlayerPrivate {
public:

  std::unique_ptr<QMediaPlayer> player;

  explicit MediaPlayerPrivate()
    : player{new QMediaPlayer}
  {
  }
};

MediaPlayer::MediaPlayer()
  : QObject{}
  , d_ptr{new MediaPlayerPrivate}
{
}

MediaPlayer::~MediaPlayer() {
}

bool
MediaPlayer::isPlaying()
  const {
  Q_D(const MediaPlayer);

  return d->player->state() == QMediaPlayer::PlayingState;
}

void
MediaPlayer::playFile(QString const &fileName,
                      unsigned int volume) {
  Q_D(MediaPlayer);

  stopPlayback();

  d->player->setVolume(volume);
  d->player->setMedia(QUrl::fromLocalFile(fileName));
  d->player->play();
}

void
MediaPlayer::stopPlayback() {
  Q_D(MediaPlayer);

  d->player->stop();
  d->player->setMedia({});
}

}}}
