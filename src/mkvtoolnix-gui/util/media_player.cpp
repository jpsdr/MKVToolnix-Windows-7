#include "common/common_pch.h"

#include <Qt>

#if HAVE_QMEDIAPLAYER
# if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
#  include <QAudioOutput>
# endif
# include <QMediaPlayer>

#else  // HAVE_QMEDIAPLAYER
# include <QUrl>

// Fake Media Player class as Qt 6.1 hasn't re-added the media framework yet.
class QMediaPlayer {
public:
  static constexpr int PlayingState = 1;

  void setMedia(QUrl const &) {}
  void setVolume(int) {}

  int state() const { return 0; }

  void play() {}
  void stop() {}
};

#endif  // HAVE_QMEDIAPLAYER

#include "mkvtoolnix-gui/util/media_player.h"

namespace mtx::gui::Util {

using namespace mtx::gui;

class MediaPlayerPrivate {
public:

  std::unique_ptr<QMediaPlayer> player;
#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
  std::unique_ptr<QAudioOutput> audioOutput;
#endif

  explicit MediaPlayerPrivate()
    : player{new QMediaPlayer}
  {
#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
    audioOutput.reset(new QAudioOutput);
    player->setAudioOutput(audioOutput.get());
#endif
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
#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
  return p_func()->player->playbackState() == QMediaPlayer::PlayingState;
#else
  return p_func()->player->state() == QMediaPlayer::PlayingState;
#endif
}

void
MediaPlayer::playFile(QString const &fileName,
                      unsigned int volume) {
  auto p = p_func();

  stopPlayback();

#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
  p->audioOutput->setVolume(volume);
  p->player->setSource(QUrl::fromLocalFile(fileName));
#else
  p->player->setVolume(volume);
  p->player->setMedia(QUrl::fromLocalFile(fileName));
#endif
  p->player->play();
}

void
MediaPlayer::stopPlayback() {
  auto p = p_func();

  p->player->stop();
#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
  p->player->setSource({});
#else
  p->player->setMedia({});
#endif
}

}
