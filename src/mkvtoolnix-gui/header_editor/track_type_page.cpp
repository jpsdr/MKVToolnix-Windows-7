#include "common/common_pch.h"

#include "common/ebml.h"
#include "common/qt.h"
#include "mkvtoolnix-gui/forms/header_editor/track_type_page.h"
#include "mkvtoolnix-gui/header_editor/track_type_page.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/util/model.h"

using namespace libmatroska;
using namespace mtx::gui;

namespace mtx::gui::HeaderEditor {

TrackTypePage::TrackTypePage(Tab &parent,
                             EbmlMaster &master,
                             uint64_t trackIdxMkvmerge)
  : TopLevelPage{parent, "", true}
  , ui{new Ui::TrackTypePage}
  , m_master(master)
  , m_trackIdxMkvmerge{trackIdxMkvmerge}
  , m_trackType{FindChildValue<KaxTrackType>(m_master)}
  , m_trackNumber{FindChildValue<KaxTrackNumber>(m_master)}
  , m_trackUid{FindChildValue<KaxTrackUID>(m_master)}
  , m_codecId{Q(FindChildValue<KaxCodecID>(m_master))}
  , m_name{Q(FindChildValue<KaxTrackName>(m_master))}
  , m_defaultTrackFlag{!!FindChildValue<KaxTrackFlagDefault>(m_master, 1u)}
  , m_forcedTrackFlag{!!FindChildValue<KaxTrackFlagForced>(m_master, 0u)}
  , m_enabledTrackFlag{!!FindChildValue<KaxTrackFlagEnabled>(m_master, 1u)}
  , m_yesIcon{Util::fixStandardItemIcon(MainWindow::yesIcon())}
  , m_noIcon{Util::fixStandardItemIcon(MainWindow::noIcon())}
{
  m_language = mtx::bcp47::language_c::parse(FindChildValue<KaxLanguageIETF>(m_master));
  if (!m_language.is_valid())
    m_language = mtx::bcp47::language_c::parse(FindChildValue<KaxTrackLanguage>(m_master, "eng"s));

  ui->setupUi(this);
}

TrackTypePage::~TrackTypePage() {
}

void
TrackTypePage::retranslateUi() {
  summarizeProperties();

  auto title = track_audio    == m_trackType ? QY("Audio track %1")
             : track_video    == m_trackType ? QY("Video track %1")
             : track_subtitle == m_trackType ? QY("Subtitle track %1")
             :                                 QY("Button track %1");
  title      = title.arg(m_trackNumber);
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

  ui->m_lUidLabel->setText(QY("UID:"));
  ui->m_lUid->setText(Q("%1").arg(m_trackUid));

  ui->m_lLanguageLabel->setText(QY("Language:"));
  ui->m_lLanguage->setText(Q(m_language.format_long()));

  ui->m_lNameLabel->setText(QY("Name:"));
  ui->m_lName->setText(m_name);

  ui->m_lDefaultTrackFlagLabel->setText(QY("\"Default track\" flag:"));
  ui->m_lDefaultTrackFlag->setText(   m_defaultTrackFlag ? QY("Yes") : QY("No"));
  ui->m_iDefaultTrackFlag->setPixmap((m_defaultTrackFlag ? m_yesIcon : m_noIcon).pixmap({ 16, 16 }));

  ui->m_lForcedTrackFlagLabel->setText(QY("\"Forced display\" flag:"));
  ui->m_lForcedTrackFlag->setText(   m_forcedTrackFlag ? QY("Yes") : QY("No"));
  ui->m_iForcedTrackFlag->setPixmap((m_forcedTrackFlag ? m_yesIcon : m_noIcon).pixmap({ 16, 16 }));

  ui->m_lEnabledTrackFlagLabel->setText(QY("\"Track enabled\" flag:"));
  ui->m_lEnabledTrackFlag->setText(   m_enabledTrackFlag ? QY("Yes") : QY("No"));
  ui->m_iEnabledTrackFlag->setPixmap((m_enabledTrackFlag ? m_yesIcon : m_noIcon).pixmap({ 16, 16 }));

  ui->m_lPropertiesLabel->setText(QY("Properties:"));
  ui->m_lProperties->setText(m_properties);
}

void
TrackTypePage::setItems(QList<QStandardItem *> const &items)
  const {
  TopLevelPage::setItems(items);

  items.at(PageModel::CodecColumn)        ->setText(m_codecId);
  items.at(PageModel::LanguageColumn)     ->setText(Q(m_language.format_long()));
  items.at(PageModel::NameColumn)         ->setText(m_name);
  items.at(PageModel::UidColumn)          ->setText(QString::number(m_trackUid));
  items.at(PageModel::DefaultTrackColumn) ->setText(m_defaultTrackFlag ? QY("Yes") : QY("No"));
  items.at(PageModel::ForcedDisplayColumn)->setText(m_forcedTrackFlag  ? QY("Yes") : QY("No"));
  items.at(PageModel::EnabledColumn)      ->setText(m_enabledTrackFlag ? QY("Yes") : QY("No"));
  items.at(PageModel::PropertiesColumn)   ->setText(m_properties);

  items.at(PageModel::DefaultTrackColumn) ->setIcon(m_defaultTrackFlag ? m_yesIcon : m_noIcon);
  items.at(PageModel::ForcedDisplayColumn)->setIcon(m_forcedTrackFlag  ? m_yesIcon : m_noIcon);
  items.at(PageModel::EnabledColumn)      ->setIcon(m_enabledTrackFlag ? m_yesIcon : m_noIcon);

  items.at(PageModel::UidColumn)          ->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
}

void
TrackTypePage::summarizeProperties() {
  m_properties.clear();

  auto properties = QStringList{};

  if (track_audio == m_trackType) {
    auto trackAudio = FindChild<KaxTrackAudio>(m_master);

    if (!trackAudio)
      return;

    auto channels      = FindChildValue<KaxAudioChannels>(trackAudio, 1);
    auto bitsPerSample = FindChild<KaxAudioBitDepth>(trackAudio);

    properties << QY("%1 Hz").arg(FindChildValue<KaxAudioSamplingFreq>(trackAudio, 8000.0));
    properties << QNY("%1 channel", "%1 channels", channels).arg(channels);
    if (bitsPerSample)
      properties << QNY("%1 bit per sample", "%1 bits per sample", bitsPerSample->GetValue()).arg(bitsPerSample->GetValue());

  } else if (track_video == m_trackType) {
    auto trackVideo = FindChild<KaxTrackVideo>(m_master);

    if (!trackVideo)
      return;

    properties << QY("%1 pixels").arg(Q("%1x%2").arg(FindChildValue<KaxVideoPixelWidth>(trackVideo, 0u)).arg(FindChildValue<KaxVideoPixelHeight>(trackVideo, 0u)));
  }

  m_properties = properties.join(Q(", "));
}

QString
TrackTypePage::internalIdentifier()
  const {
  return Q("track %1").arg(m_trackIdxMkvmerge);
}

}
