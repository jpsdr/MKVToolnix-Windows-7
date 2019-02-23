#pragma once

#include "common/common_pch.h"

#include <QObject>

#include "common/qt.h"

class QByteArray;
class QNetworkAccessManager;
class QUrl;

namespace mtx { namespace gui { namespace Util {

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
