#pragma once

#include "common/common_pch.h"

#include <QObject>

#include "common/qt.h"

namespace mtx { namespace gui { namespace Util {

class MediaPlayerPrivate;
class MediaPlayer : public QObject {
  Q_OBJECT;

protected:
  MTX_DECLARE_PRIVATE(MediaPlayer);

  std::unique_ptr<MediaPlayerPrivate> const p_ptr;

  explicit MediaPlayer(MediaPlayerPrivate &p);

public:
  MediaPlayer();
  virtual ~MediaPlayer();

  bool isPlaying() const;

public slots:
  void playFile(QString const &fileName, unsigned int volume);
  void stopPlayback();
};

}}}
