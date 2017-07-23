#include "common/common_pch.h"

#if defined(HAVE_UPDATE_CHECK)

#include <QDebug>
#include <QUrl>
#include <QVector>

#include "common/compression.h"
#include "common/qt.h"
#include "common/version.h"
#include "mkvtoolnix-gui/app.h"
#include "mkvtoolnix-gui/main_window/update_checker.h"
#include "mkvtoolnix-gui/util/network_access_manager.h"

namespace mtx { namespace gui {

class UpdateCheckerPrivate {
  friend class UpdateChecker;

  QVector<quint64> m_tokens;
  int m_numFinished{};
  mtx_release_version_t m_release;
  mtx::xml::document_cptr m_updateInfo;
  bool m_retrieveReleasesInfo{};

  explicit UpdateCheckerPrivate()
  {
  }
};

using namespace mtx::gui;

UpdateChecker::UpdateChecker(QObject *parent)
  : QObject{parent}
  , d_ptr{new UpdateCheckerPrivate{}}
{
}

UpdateChecker::~UpdateChecker() {
}

UpdateChecker &
UpdateChecker::setRetrieveReleasesInfo(bool enable) {
  Q_D(UpdateChecker);

  d->m_retrieveReleasesInfo = enable;

  return *this;
}

void
UpdateChecker::start() {
  Q_D(UpdateChecker);

  qDebug() << "UpdateChecker::start: initiating requests";

  emit checkStarted();
  qDebug() << "UpdateChecker::start: checkStarted emitted";

  auto &manager = App::instance()->networkAccessManager();
  auto urls     = QVector<std::string>{ MTX_VERSION_CHECK_URL };

  debugging_c::requested("version_check_url", &urls[0]);

  if (d->m_retrieveReleasesInfo) {
    urls << MTX_RELEASES_INFO_URL;
    debugging_c::requested("releases_info_url", &urls[1]);
  }

  connect(&manager, &Util::NetworkAccessManager::downloadFinished, this, &UpdateChecker::handleDownloadedContent);

  qDebug() << "UpdateChecker::start: URL list built";

  for (auto const &url : urls)
    d->m_tokens.push_back(manager.download(QUrl{Q("%1.gz").arg(Q(url))}));

  qDebug() << "UpdateChecker::start: startup done";
}

void
UpdateChecker::handleDownloadedContent(quint64 token,
                                       QByteArray const &content) {
  Q_D(UpdateChecker);

  qDebug() << "UpdateChecker::handleDownloadedContent: token" << token;

  auto idx = d->m_tokens.indexOf(token);
  if (idx < 0) {
    qDebug() << "UpdateChecker::handleDownloadedContent: token unknown";
    return;
  }

  auto doc = parseXml(content);

  ++d->m_numFinished;

  qDebug() << "UpdateChecker::handleDownloadedContent: for" << idx << "numFinished" << d->m_numFinished;

  if (idx == 0) {
    d->m_release = parse_latest_release_version(doc);

    auto status = !d->m_release.valid                                       ? UpdateCheckStatus::Failed
                : d->m_release.current_version < d->m_release.latest_source ? UpdateCheckStatus::NewReleaseAvailable
                :                                                             UpdateCheckStatus::NoNewReleaseAvailable;

    qDebug() << "UpdateChecker::handleDownloadedContent: latest version info retrieved; status:" << static_cast<int>(status);

    emit checkFinished(status, d->m_release);

  } else {
    qDebug() << "UpdateChecker::handleDownloadedContent: releases info retrieved";
    emit releaseInformationRetrieved(doc);
  }

  if (d->m_numFinished < d->m_tokens.size())
    return;

  qDebug() << "UpdateChecker::handleDownloadedContent: done, deleting object";
  deleteLater();
}

mtx::xml::document_cptr
UpdateChecker::parseXml(QByteArray const &content) {
  try {
    auto data = std::string{ content.data(), static_cast<std::string::size_type>(content.size()) };
    data      = compressor_c::create_from_file_name("dummy.gz")->decompress(data);
    auto doc  = std::make_shared<pugi::xml_document>();

    std::stringstream sdata{data};
    auto xml_result = doc->load(sdata);

    if (xml_result) {
      qDebug() << "UpdateChecker::parseXml: of" << content.size() << "bytes was OK";
      return doc;
    }

    qDebug() << "UpdateChecker::parseXml: of" << content.size() << "bytes failed:" << Q(xml_result.description()) << "at" << Q(xml_result.offset);

  } catch (mtx::compression_x &ex) {
    qDebug() << "UpdateChecker::parseXml: decompression exception:" << Q(ex.what());
  }

  return {};
}

}}

#endif  // HAVE_UPDATE_CHECK
