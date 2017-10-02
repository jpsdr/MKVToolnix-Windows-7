#pragma once

#include "common/common_pch.h"

#if defined(HAVE_UPDATE_CHECK)

#include <QDialog>

#include "common/version.h"
#include "common/xml/xml.h"
#include "mkvtoolnix-gui/main_window/update_checker.h"

namespace mtx { namespace gui {

namespace Ui {
class AvailableUpdateInfoDialog;
}

class AvailableUpdateInfoDialog : public QDialog {
  Q_OBJECT;

protected:
  // UI stuff:
  std::unique_ptr<Ui::AvailableUpdateInfoDialog> ui;
  std::shared_ptr<pugi::xml_document> m_releasesInfo;
  QString m_downloadURL;

  bool m_statusRetrieved;
  UpdateCheckStatus m_status;
  mtx_release_version_t m_releaseVersion;

public:
  explicit AvailableUpdateInfoDialog(QWidget *parent);
  ~AvailableUpdateInfoDialog();

  void setChangeLogContent(QString const &content);

public slots:
  void setReleaseInformation(std::shared_ptr<pugi::xml_document> releasesInfo);
  void updateCheckFinished(mtx::gui::UpdateCheckStatus status, mtx_release_version_t releaseVersion);
  void visitDownloadLocation();

protected:
  void updateDisplay();
  void updateStatusDisplay();
  void updateReleasesInfoDisplay();

  static QString formattedCodename(QString const &codename, QString const &artist);
};

}}

#endif  // HAVE_UPDATE_CHECK
