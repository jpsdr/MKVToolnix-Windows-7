#ifndef MTX_MKVTOOLNIX_GUI_MAIN_WINDOW_UPDATE_CHECK_THREAD_H
#define MTX_MKVTOOLNIX_GUI_MAIN_WINDOW_UPDATE_CHECK_THREAD_H

#include "common/common_pch.h"

#include <QObject>
#include <QThread>

#include "common/version.h"

namespace mtx { namespace gui {

enum class UpdateCheckStatus {
  Failed,
  NewReleaseAvailable,
  NoNewReleaseAvailable,
};

class UpdateCheckThreadPrivate;
class UpdateCheckThread : public QObject {
  Q_OBJECT;

  Q_DECLARE_PRIVATE(UpdateCheckThread);

  QScopedPointer<UpdateCheckThreadPrivate> const d_ptr;

  explicit UpdateCheckThread(UpdateCheckThreadPrivate &d);

public:
  UpdateCheckThread(QObject *parent);
  virtual ~UpdateCheckThread();

  void start(bool retrieveReleasesInfo);

signals:
  void checkStarted();
  void checkFinished(mtx::gui::UpdateCheckStatus status, mtx_release_version_t release);
  void releaseInformationRetrieved(std::shared_ptr<pugi::xml_document> releasesInfo);

public slots:
  void httpFinished();

protected:
  mtx::xml::document_cptr parseXml(QByteArray const &content);
};

}}

Q_DECLARE_METATYPE(mtx::gui::UpdateCheckStatus);
Q_DECLARE_METATYPE(mtx_release_version_t);
Q_DECLARE_METATYPE(std::shared_ptr<pugi::xml_document>);

#endif  // MTX_MKVTOOLNIX_GUI_MAIN_WINDOW_UPDATE_CHECK_THREAD_H
