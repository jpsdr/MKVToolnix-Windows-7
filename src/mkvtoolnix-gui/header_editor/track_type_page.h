#pragma once

#include "common/common_pch.h"

#include "common/bcp47.h"
#include "mkvtoolnix-gui/header_editor/top_level_page.h"

namespace mtx::gui::HeaderEditor {

namespace Ui {
class TrackTypePage;
}

class TrackTypePage: public TopLevelPage {
  Q_OBJECT

protected:
  std::unique_ptr<Ui::TrackTypePage> ui;

public:
  libebml::EbmlMaster &m_master;
  uint64_t m_trackIdxMkvmerge, m_trackType, m_trackNumber, m_trackUid;
  QString m_codecId, m_name, m_properties;
  mtx::bcp47::language_c m_language;
  bool m_defaultTrackFlag, m_forcedTrackFlag, m_enabledTrackFlag;
  QIcon m_yesIcon, m_noIcon;

public:
  TrackTypePage(Tab &parent, libebml::EbmlMaster &master, uint64_t trackIdxMkvmerge);
  virtual ~TrackTypePage();

  virtual QString internalIdentifier() const override;

protected:
  virtual void setItems(QList<QStandardItem *> const &items) const override;
  virtual void summarizeProperties();

public Q_SLOTS:
  virtual void retranslateUi() override;
};

}
