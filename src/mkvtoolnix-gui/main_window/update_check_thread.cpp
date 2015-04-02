#include "common/common_pch.h"

#if defined(HAVE_CURL_EASY_H)

# include "common/version.h"
# include "mkvtoolnix-gui/main_window/update_check_thread.h"

namespace mtx { namespace gui {

UpdateCheckThread::UpdateCheckThread(QObject *parent)
  : QThread{parent}
{
  connect(this, &UpdateCheckThread::finished, this, &QObject::deleteLater);
}

void
UpdateCheckThread::run() {
  emit checkStarted();

  auto release = get_latest_release_version();

  if (!release.valid) {
    emit checkFinished(UpdateCheckStatus::Failed, mtx_release_version_t{});
    return;
  }

  emit releaseInformationRetrieved(get_releases_info());
  emit checkFinished(release.current_version < release.latest_source ? UpdateCheckStatus::NewReleaseAvailable : UpdateCheckStatus::NoNewReleaseAvailable, release);
}

}}

#endif  // HAVE_CURL_EASY_H
