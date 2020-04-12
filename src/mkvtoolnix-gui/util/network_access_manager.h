#pragma once

#include "common/common_pch.h"

#include <QObject>

#include "common/qt.h"

class QByteArray;
class QNetworkAccessManager;
class QUrl;

namespace mtx::gui::Util {

class NetworkAccessManagerPrivate;
class NetworkAccessManager : public QObject {
  Q_OBJECT

protected:
  MTX_DECLARE_PRIVATE(NetworkAccessManagerPrivate)

  std::unique_ptr<NetworkAccessManagerPrivate> const p_ptr;

  explicit NetworkAccessManager(NetworkAccessManagerPrivate &p);

public:
  NetworkAccessManager();
  virtual ~NetworkAccessManager();

  quint64 download(QUrl const &url);

Q_SIGNALS:
  void downloadFinished(quint64 token, QByteArray const &content);

public Q_SLOTS:
  void httpFinished();

protected Q_SLOTS:
  void startDownload(quint64 token, QUrl const &url);

protected:
  QNetworkAccessManager &manager();
};

}
