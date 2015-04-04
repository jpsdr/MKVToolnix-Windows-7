#include "common/common_pch.h"

#include "common/ebml.h"
#include "common/qt.h"
#include "mkvtoolnix-gui/forms/header_editor/track_type_page.h"
#include "mkvtoolnix-gui/header_editor/track_type_page.h"

namespace mtx { namespace gui { namespace HeaderEditor {

using namespace mtx::gui;

TrackTypePage::TrackTypePage(Tab &parent,
                             EbmlMaster &master,
                             uint64_t trackIdxMkvmerge)
  : TopLevelPage{parent, "", true}
  , ui{new Ui::TrackTypePage}
  , m_master{master}
  , m_trackIdxMkvmerge{trackIdxMkvmerge}
  , m_trackType{FindChildValue<KaxTrackType>(m_master)}
  , m_trackNumber{FindChildValue<KaxTrackNumber>(m_master)}
  , m_codecId{Q(FindChildValue<KaxCodecID>(m_master))}
{
  ui->setupUi(this);
}

TrackTypePage::~TrackTypePage() {
}

void
TrackTypePage::retranslateUi() {
  auto title = track_audio    == m_trackType ? QY("Audio track %1 (%2)")
             : track_video    == m_trackType ? QY("Video track %1 (%2)")
             : track_subtitle == m_trackType ? QY("Subtitle track %1 (%2)")
             :                                 QY("Button track %1 (%2)");
  title      = title.arg(m_trackNumber).arg(m_codecId);
  m_title    = translatable_string_c{to_utf8(title)};

  auto type  = track_audio    == m_trackType ? QY("Audio")
             : track_video    == m_trackType ? QY("Video")
             : track_subtitle == m_trackType ? QY("Subtitles")
             :                                 QY("Buttons");

  ui->m_lTitle->setText(title);

  ui->m_lTypeLabel->setText(QY("Type:"));
  ui->m_lType->setText(type);

  ui->m_lCodecIdLabel->setText(QY("Codec ID:"));
  ui->m_lCodecId->setText(m_codecId);

  ui->m_lTrackIdxMkvmergeLabel->setText(QY("Track ID for mkvmerge & mkvextract:"));
  ui->m_lTrackIdxMkvmerge->setText(Q("%1").arg(m_trackIdxMkvmerge));

  ui->m_lTrackNumberMkvpropeditLabel->setText(QY("Track number for mkvpropedit:"));
  ui->m_lTrackNumberMkvpropedit->setText(Q("track:@%1").arg(m_trackNumber));
}

}}}
