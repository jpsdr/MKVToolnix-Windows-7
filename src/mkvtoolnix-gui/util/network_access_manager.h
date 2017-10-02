#pragma once

#include "common/common_pch.h"

#include <QObject>

class QByteArray;
class QNetworkAccessManager;
class QUrl;

namespace mtx { namespace gui { namespace Util {

class NetworkAccessManagerPrivate;
class NetworkAccessManager : public QObject {
  Q_OBJECT;

  Q_DECLARE_PRIVATE(NetworkAccessManager);

  QScopedPointer<NetworkAccessManagerPrivate> const d_ptr;

  explicit NetworkAccessManager(NetworkAccessManagerPrivate &d);

public:
  NetworkAccessManager();
  virtual ~NetworkAccessManager();

  quint64 download(QUrl const &url);

signals:
  void downloadFinished(quint64 token, QByteArray const &content);

public slots:
  void httpFinished();

protected slots:
  void startDownload(quint64 token, QUrl const &url);

protected:
  QNetworkAccessManager &manager();
};

}}}
