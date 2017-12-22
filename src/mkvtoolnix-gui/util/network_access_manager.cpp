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
  , p_ptr{new NetworkAccessManagerPrivate{}}
{
}

NetworkAccessManager::~NetworkAccessManager() {
}

QNetworkAccessManager &
NetworkAccessManager::manager() {
  auto p = p_func();

  if (!p->m_manager) {
    qDebug() << "NetworkAccessManager::manager: creating QNetworkAccessManager";
    QNetworkProxyFactory::setUseSystemConfiguration(true);
    p->m_manager = new QNetworkAccessManager{this};
  }

  return *p->m_manager;
}

quint64
NetworkAccessManager::download(QUrl const &url) {
  auto p     = p_func();
  auto token = p->m_nextToken++;

  QMetaObject::invokeMethod(this, "startDownload", Q_ARG(quint64, token), Q_ARG(QUrl, url));

  return token;
}

void
NetworkAccessManager::startDownload(quint64 token,
                                    QUrl const &url) {
  qDebug() << "NetworkAccessManager::startDownload: token" << token << "starting for url" << url;

  auto p                   = p_func();
  auto reply               = manager().get(QNetworkRequest{url});
  p->m_tokenByReply[reply] = token;

  connect(reply, &QNetworkReply::finished, this, &NetworkAccessManager::httpFinished);

  qDebug() << "NetworkAccessManager::startDownload: token" << token << "request in progress";
}

void
NetworkAccessManager::httpFinished() {
  auto p     = p_func();
  auto reply = qobject_cast<QNetworkReply *>(sender());

  if (!reply)
    return;

  if (p->m_tokenByReply.contains(reply)) {
    auto token   = p->m_tokenByReply.take(reply);
    auto content = reply->readAll();

    qDebug() << "NetworkAccessManager::httpFinished: token" << token << "request done, read" << content.size();

    emit downloadFinished(token, content);

  } else
    qDebug() << "NetworkAccessManager::httpFinished: unknown reply?";

  reply->deleteLater();
}

}}}
