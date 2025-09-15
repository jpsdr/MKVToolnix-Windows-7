#include "common/common_pch.h"

#include <Qt>
#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
# include <QAudioOutput>
#endif
#include <QDebug>
#include <QMediaPlayer>

#include "mkvtoolnix-gui/util/media_player.h"

namespace mtx::gui::Util {

using namespace mtx::gui;

class MediaPlayerPrivate {
public:

  std::unique_ptr<QMediaPlayer> player;
#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
  std::unique_ptr<QAudioOutput> audioOutput;
#endif
  QString lastPlayedFile;
  bool errorReported{};

  explicit MediaPlayerPrivate()
    : player{new QMediaPlayer}
#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
    , audioOutput{new QAudioOutput}
#endif
  {
#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
    player->setAudioOutput(audioOutput.get());
#endif
  }
};

MediaPlayer::MediaPlayer()
  : QObject{}
  , p_ptr{new MediaPlayerPrivate}
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
  connect(p_ptr->player.get(), &QMediaPlayer::errorOccurred, this, &MediaPlayer::handleError);
#else
  connect(p_ptr->player.get(), static_cast<void (QMediaPlayer::*)(QMediaPlayer::Error)>(&QMediaPlayer::error), this, &MediaPlayer::handleError);
#endif
}

MediaPlayer::~MediaPlayer() {
}

void
MediaPlayer::handleError(QMediaPlayer::Error error) {
  auto &p = *p_func();

  qDebug() << "MediaPlayer::handleError" << error << p.errorReported;

  if (p.lastPlayedFile.isEmpty() || p.errorReported)
    return;

  p.errorReported = true;

  Q_EMIT errorOccurred(error, p.lastPlayedFile);
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
  auto &p = *p_func();

  stopPlayback();

  p.lastPlayedFile = fileName;

#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
  p.audioOutput->setVolume(volume);
  p.player->setSource(QUrl::fromLocalFile(fileName));
#else
  p.player->setVolume(volume);
  p.player->setMedia(QUrl::fromLocalFile(fileName));
#endif
  p.player->play();
}

void
MediaPlayer::stopPlayback() {
  auto &p = *p_func();

  p.player->stop();
#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
  p.player->setSource({});
#else
  p.player->setMedia({});
#endif

  p.lastPlayedFile.clear();
  p.errorReported = false;
}

}
