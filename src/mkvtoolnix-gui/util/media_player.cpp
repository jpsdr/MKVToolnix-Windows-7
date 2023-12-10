#include "common/common_pch.h"

#include <Qt>
#include <QAudioOutput>
#include <QMediaPlayer>

#include "mkvtoolnix-gui/util/media_player.h"

namespace mtx::gui::Util {

using namespace mtx::gui;

class MediaPlayerPrivate {
public:

  std::unique_ptr<QMediaPlayer> player;
  std::unique_ptr<QAudioOutput> audioOutput;
  QString lastPlayedFile;
  bool errorReported{};

  explicit MediaPlayerPrivate()
    : player{new QMediaPlayer}
    , audioOutput{new QAudioOutput}
  {
    player->setAudioOutput(audioOutput.get());
  }
};

MediaPlayer::MediaPlayer()
  : QObject{}
  , p_ptr{new MediaPlayerPrivate}
{
  connect(p_ptr->player.get(), &QMediaPlayer::errorOccurred, this, &MediaPlayer::handleError);
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
  return p_func()->player->playbackState() == QMediaPlayer::PlayingState;
}

void
MediaPlayer::playFile(QString const &fileName,
                      unsigned int volume) {
  auto &p = *p_func();

  stopPlayback();

  p.lastPlayedFile = fileName;

  p.audioOutput->setVolume(volume);
  p.player->setSource(QUrl::fromLocalFile(fileName));
  p.player->play();
}

void
MediaPlayer::stopPlayback() {
  auto &p = *p_func();

  p.player->stop();
  p.player->setSource({});

  p.lastPlayedFile.clear();
  p.errorReported = false;
}

}
