#include "common/common_pch.h"

#include <QMediaPlayer>

#include "mkvtoolnix-gui/util/media_player.h"

namespace mtx::gui::Util {

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
  , p_ptr{new MediaPlayerPrivate}
{
}

MediaPlayer::~MediaPlayer() {
}

bool
MediaPlayer::isPlaying()
  const {
  return p_func()->player->state() == QMediaPlayer::PlayingState;
}

void
MediaPlayer::playFile(QString const &fileName,
                      unsigned int volume) {
  auto p = p_func();

  stopPlayback();

  p->player->setVolume(volume);
  p->player->setMedia(QUrl::fromLocalFile(fileName));
  p->player->play();
}

void
MediaPlayer::stopPlayback() {
  auto p = p_func();

  p->player->stop();
  p->player->setMedia({});
}

}
