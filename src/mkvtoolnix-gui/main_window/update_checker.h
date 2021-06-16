#pragma once

#include "common/common_pch.h"

#if defined(HAVE_UPDATE_CHECK)

#include <QObject>
#include <QThread>

#include "common/qt.h"
#include "common/version.h"

namespace mtx::gui {

enum class UpdateCheckStatus {
  Failed,
  NewReleaseAvailable,
  NoNewReleaseAvailable,
};

class UpdateCheckerPrivate;
class UpdateChecker : public QObject {
  Q_OBJECT

protected:
  MTX_DECLARE_PRIVATE(UpdateCheckerPrivate)

  std::unique_ptr<UpdateCheckerPrivate> const p_ptr;

  explicit UpdateChecker(UpdateCheckerPrivate &p);

public:
  UpdateChecker(QObject *parent = nullptr);
  virtual ~UpdateChecker();

  UpdateChecker &setRetrieveReleasesInfo(bool enable);

Q_SIGNALS:
  void checkStarted();
  void checkFinished(mtx::gui::UpdateCheckStatus status, mtx_release_version_t release);
  void releaseInformationRetrieved(std::shared_ptr<pugi::xml_document> releasesInfo);

public Q_SLOTS:
  void handleDownloadedContent(quint64 token, QByteArray const &content);
  void start();

public:
  static QString versionCheckURL();
  static QString releasesInfoURL();
  static QString downloadURL();
  static QString newsURL();

protected:
  mtx::xml::document_cptr parseXml(QByteArray const &content);
};

}

Q_DECLARE_METATYPE(mtx::gui::UpdateCheckStatus)
Q_DECLARE_METATYPE(mtx_release_version_t)
Q_DECLARE_METATYPE(std::shared_ptr<pugi::xml_document>)

#endif  // HAVE_UPDATE_CHECK
