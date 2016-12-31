#include "common/common_pch.h"

#include <QDebug>
#include <QNetworkAccessManager>
#include <QNetworkProxyFactory>
#include <QNetworkReply>
#include <QUrl>

#include "common/qt.h"
#include "mkvtoolnix-gui/util/network_access_manager.h"

namespace mtx { namespace gui { namespace Util {

using namespace mtx::gui;

class NetworkAccessManagerPrivate {
  friend class NetworkAccessManager;

  QNetworkAccessManager *m_manager{};
  quint64 m_nextToken{};
  QHash<QNetworkReply *, quint64> m_tokenByReply;

  explicit NetworkAccessManagerPrivate()
  {
  }
};

NetworkAccessManager::NetworkAccessManager()
  : QObject{}
  , d_ptr{new NetworkAccessManagerPrivate{}}
{
}

NetworkAccessManager::~NetworkAccessManager() {
}

QNetworkAccessManager &
NetworkAccessManager::manager() {
  Q_D(NetworkAccessManager);

  if (!d->m_manager) {
    qDebug() << "NetworkAccessManager::manager: creating QNetworkAccessManager";
    QNetworkProxyFactory::setUseSystemConfiguration(true);
    d->m_manager = new QNetworkAccessManager{this};
  }

  return *d->m_manager;
}

quint64
NetworkAccessManager::download(QUrl const &url) {
  Q_D(NetworkAccessManager);

  auto token = d->m_nextToken++;
  QMetaObject::invokeMethod(this, "startDownload", Q_ARG(quint64, token), Q_ARG(QUrl, url));

  return token;
}

void
NetworkAccessManager::startDownload(quint64 token,
                                    QUrl const &url) {
  Q_D(NetworkAccessManager);

  qDebug() << "NetworkAccessManager::startDownload: token" << token << "starting for url" << url;

  auto reply               = manager().get(QNetworkRequest{url});
  d->m_tokenByReply[reply] = token;

  connect(reply, &QNetworkReply::finished, this, &NetworkAccessManager::httpFinished);

  qDebug() << "NetworkAccessManager::startDownload: token" << token << "request in progress";
}

void
NetworkAccessManager::httpFinished() {
  Q_D(NetworkAccessManager);

  auto reply = qobject_cast<QNetworkReply *>(sender());
  if (!reply)
    return;

  if (d->m_tokenByReply.contains(reply)) {
    auto token   = d->m_tokenByReply.take(reply);
    auto content = reply->readAll();

    qDebug() << "NetworkAccessManager::httpFinished: token" << token << "request done, read" << content.size();

    emit downloadFinished(token, content);

  } else
    qDebug() << "NetworkAccessManager::httpFinished: unknown reply?";

  reply->deleteLater();
}

}}}
