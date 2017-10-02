#pragma once

#include "common/common_pch.h"

#if defined(HAVE_UPDATE_CHECK)

#include <QObject>
#include <QThread>

#include "common/version.h"

namespace mtx { namespace gui {

enum class UpdateCheckStatus {
  Failed,
  NewReleaseAvailable,
  NoNewReleaseAvailable,
};

class UpdateCheckerPrivate;
class UpdateChecker : public QObject {
  Q_OBJECT;

  Q_DECLARE_PRIVATE(UpdateChecker);

  QScopedPointer<UpdateCheckerPrivate> const d_ptr;

  explicit UpdateChecker(UpdateCheckerPrivate &d);

public:
  UpdateChecker(QObject *parent = nullptr);
  virtual ~UpdateChecker();

  UpdateChecker &setRetrieveReleasesInfo(bool enable);

signals:
  void checkStarted();
  void checkFinished(mtx::gui::UpdateCheckStatus status, mtx_release_version_t release);
  void releaseInformationRetrieved(std::shared_ptr<pugi::xml_document> releasesInfo);

public slots:
  void handleDownloadedContent(quint64 token, QByteArray const &content);
  void start();

protected:
  mtx::xml::document_cptr parseXml(QByteArray const &content);
};

}}

Q_DECLARE_METATYPE(mtx::gui::UpdateCheckStatus);
Q_DECLARE_METATYPE(mtx_release_version_t);
Q_DECLARE_METATYPE(std::shared_ptr<pugi::xml_document>);

#endif  // HAVE_UPDATE_CHECK
