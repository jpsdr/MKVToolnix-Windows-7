#ifndef MTX_MKVTOOLNIX_GUI_HEADER_EDITOR_TRACK_TYPE_PAGE_H
#define MTX_MKVTOOLNIX_GUI_HEADER_EDITOR_TRACK_TYPE_PAGE_H

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
  uint64_t m_trackIdxMkvmerge, m_trackType, m_trackNumber;
  QString m_codecId;

public:
  TrackTypePage(Tab &parent, EbmlMaster &master, uint64_t trackIdxMkvmerge);
  virtual ~TrackTypePage();

public slots:
  virtual void retranslateUi() override;
};

}}}

#endif  // MTX_MKVTOOLNIX_GUI_HEADER_EDITOR_TRACK_TYPE_PAGE_H
