#pragma once

#include "common/common_pch.h"

#include <QObject>
#include <QMediaPlayer>

#include "common/qt.h"

namespace mtx::gui::Util {

class MediaPlayerPrivate;
class MediaPlayer : public QObject {
  Q_OBJECT

protected:
  MTX_DECLARE_PRIVATE(MediaPlayerPrivate)

  std::unique_ptr<MediaPlayerPrivate> const p_ptr;

  explicit MediaPlayer(MediaPlayerPrivate &p);

public:
  MediaPlayer();
  virtual ~MediaPlayer();

  bool isPlaying() const;

public Q_SLOTS:
  void playFile(QString const &fileName, unsigned int volume);
  void stopPlayback();
  void handleError(QMediaPlayer::Error error);

Q_SIGNALS:
  void errorOccurred(QMediaPlayer::Error error, QString const &fileName);
};

}
