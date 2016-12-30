#include "common/common_pch.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrl>

#include "common/compression.h"
#include "common/qt.h"
#include "common/version.h"
#include "mkvtoolnix-gui/main_window/update_check_thread.h"

namespace mtx { namespace gui {

class UpdateCheckThreadPrivate {
  friend class UpdateCheckThread;

  QNetworkAccessManager m_manager;
  std::vector<std::shared_ptr<QNetworkReply>> m_replies;
  unsigned int m_numFinished{};
  mtx_release_version_t m_release;
  mtx::xml::document_cptr m_updateInfo;

  explicit UpdateCheckThreadPrivate()
  {
  }
};

using namespace mtx::gui;

UpdateCheckThread::UpdateCheckThread(QObject *parent)
  : QObject{parent}
  , d_ptr{new UpdateCheckThreadPrivate{}}
{
}

UpdateCheckThread::~UpdateCheckThread() {
}

void
UpdateCheckThread::start(bool retrieveReleasesInfo) {
  Q_D(UpdateCheckThread);

  qDebug() << "UpdateCheckThread::start: initiating requests";

  emit checkStarted();

  auto urls = std::vector<std::string>{ MTX_VERSION_CHECK_URL };
  if (retrieveReleasesInfo)
    urls.emplace_back(MTX_RELEASES_INFO_URL);

  for (auto const &url : urls) {
    d->m_replies.emplace_back(d->m_manager.get(QNetworkRequest{QUrl{Q("%1.gz").arg(Q(url))}}));
    connect(d->m_replies.back().get(), &QNetworkReply::finished, this, &UpdateCheckThread::httpFinished);
  }
}

void
UpdateCheckThread::httpFinished() {
  Q_D(UpdateCheckThread);

  qDebug() << "UpdateCheckThread::httpFinished()";

  auto itr = brng::find_if(d->m_replies, [this](auto const &reply) { return this->sender() == reply.get(); });
  if (itr == d->m_replies.end()) {
    qDebug() << "UpdateCheckThread::httpFinished: for unknown reply object!?";
    return;
  }

  auto &reply = *itr;
  auto idx    = std::distance(d->m_replies.begin(), itr);
  auto doc    = parseXml(reply->readAll());

  ++d->m_numFinished;

  qDebug() << "UpdateCheckThread::httpFinished: for" << idx << "numFinished" << d->m_numFinished;

  if (idx == 0) {
    d->m_release = parse_latest_release_version(doc);

    auto status = !d->m_release.valid                                       ? UpdateCheckStatus::Failed
                : d->m_release.current_version < d->m_release.latest_source ? UpdateCheckStatus::NewReleaseAvailable
                :                                                             UpdateCheckStatus::NoNewReleaseAvailable;

    qDebug() << "UpdateCheckThread::httpFinished: latest version info retrieved; status:" << static_cast<int>(status);

    emit checkFinished(status, d->m_release);

  } else {
    qDebug() << "UpdateCheckThread::httpFinished: releases info retrieved";
    emit releaseInformationRetrieved(doc);
  }

  if (d->m_numFinished < d->m_replies.size())
    return;

  qDebug() << "UpdateCheckThread::httpFinished: done, deleting object";
  deleteLater();
}

mtx::xml::document_cptr
UpdateCheckThread::parseXml(QByteArray const &content) {
  try {
    auto data       = std::string{ content.data(), static_cast<std::string::size_type>(content.size()) };
    data            = compressor_c::create_from_file_name("dummy.gz")->decompress(data);
    auto sdata      = std::stringstream{data};
    auto doc        = std::make_shared<pugi::xml_document>();
    auto xml_result = doc->load(sdata);

    if (xml_result) {
      qDebug() << "UpdateCheckThread::parseXml: of" << content.size() << "bytes was OK";
      return doc;
    }

    qDebug() << "UpdateCheckThread::parseXml: of" << content.size() << "bytes failed:" << Q(xml_result.description()) << "at" << Q(xml_result.offset);

  } catch (mtx::compression_x &ex) {
    qDebug() << "UpdateCheckThread::parseXml: decompression exception:" << Q(ex.what());
  }

  return {};
}

}}
