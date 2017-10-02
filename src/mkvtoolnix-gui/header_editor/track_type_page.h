#pragma once

#include "common/common_pch.h"

#include "mkvtoolnix-gui/header_editor/top_level_page.h"

namespace mtx { namespace gui { namespace HeaderEditor {

namespace Ui {
class TrackTypePage;
}

using namespace libebml;

class TrackTypePage: public TopLevelPage {
  Q_OBJECT;

protected:
  std::unique_ptr<Ui::TrackTypePage> ui;

public:
  EbmlMaster &m_master;
  uint64_t m_trackIdxMkvmerge, m_trackType, m_trackNumber, m_trackUid;
  QString m_codecId, m_language, m_name, m_properties;
  bool m_defaultTrackFlag, m_forcedTrackFlag;

public:
  TrackTypePage(Tab &parent, EbmlMaster &master, uint64_t trackIdxMkvmerge);
  virtual ~TrackTypePage();

  virtual QString internalIdentifier() const override;

protected:
  virtual void setItems(QList<QStandardItem *> const &items) const override;
  virtual void summarizeProperties();

public slots:
  virtual void retranslateUi() override;
};

}}}
