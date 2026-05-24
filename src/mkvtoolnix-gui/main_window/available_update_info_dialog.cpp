#include "common/common_pch.h"

#if defined(HAVE_UPDATE_CHECK)

#include <QDesktopServices>
#include <QRegularExpression>
#include <QUrl>
#include <QUrlQuery>

#include "common/common_urls.h"
#include "common/markdown.h"
#include "common/qt.h"
#include "common/strings/editing.h"
#include "mkvtoolnix-gui/forms/main_window/available_update_info_dialog.h"
#include "mkvtoolnix-gui/main_window/available_update_info_dialog.h"

namespace mtx::gui {

AvailableUpdateInfoDialog::AvailableUpdateInfoDialog(QWidget *parent)
  : QDialog{parent, Qt::Dialog | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint}
  , ui{new Ui::AvailableUpdateInfoDialog}
  , m_statusRetrieved{}
  , m_status{UpdateCheckStatus::Failed}
{
  // Setup UI controls.
  ui->setupUi(this);

  setWindowTitle(QY("Online check for updates"));
  updateDisplay();

  connect(ui->close,    &QPushButton::clicked, this, &AvailableUpdateInfoDialog::accept);
  connect(ui->download, &QPushButton::clicked, this, &AvailableUpdateInfoDialog::visitDownloadLocation);

  auto checker = new UpdateChecker{parent};

  connect(checker, &UpdateChecker::checkFinished,               this, &AvailableUpdateInfoDialog::updateCheckFinished);
  connect(checker, &UpdateChecker::releaseInformationRetrieved, this, &AvailableUpdateInfoDialog::setReleaseInformation);

  checker->setRetrieveReleasesInfo(true).start();
}

AvailableUpdateInfoDialog::~AvailableUpdateInfoDialog() {
}

void
AvailableUpdateInfoDialog::setReleaseInformation(std::shared_ptr<pugi::xml_document> releasesInfo) {
  m_releasesInfo = releasesInfo;

  updateDisplay();
}

void
AvailableUpdateInfoDialog::setChangeLogContent(QString const &content) {
  auto html = QStringList{};
  html << u"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">"
            "<html><head><meta name=\"qrichtext\" content=\"1\" />"
            "<style type=\"text/css\">"
            "p, li { white-space: pre-wrap; }\n"
            "</style>"
            "</head><body>"_s;

  html << u"<h1>%1 [<a href=\"%2\">%3</a>]</h1>"_s
    .arg(QY("MKVToolNix news & changes").toHtmlEscaped())
    .arg(UpdateChecker::newsURL().toHtmlEscaped())
    .arg(QY("Read full NEWS.md file online").toHtmlEscaped());

  html << content;

  html << u"</body></html>"_s;

  ui->changeLog->setHtml(html.join(u""_s));
}

void
AvailableUpdateInfoDialog::updateCheckFinished(UpdateCheckStatus status,
                                               mtx_release_version_t releaseVersion) {
  m_statusRetrieved = true;
  m_status          = status;
  m_releaseVersion  = releaseVersion;

  updateDisplay();
}

void
AvailableUpdateInfoDialog::updateDisplay() {
  updateStatusDisplay();
  updateReleasesInfoDisplay();
}

void
AvailableUpdateInfoDialog::updateStatusDisplay() {
  auto statusStr = !m_statusRetrieved                                   ? QY("Downloading release information")
                 : UpdateCheckStatus::NoNewReleaseAvailable == m_status ? QY("Currently no newer version is available online.")
                 : UpdateCheckStatus::NewReleaseAvailable   == m_status ? QY("There is a new version available online.")
                 :                                                        QY("There was an error querying the update status.");
  ui->status->setText(statusStr);

  if (!m_statusRetrieved || (UpdateCheckStatus::Failed == m_status))
    return;

  ui->currentVersion->setText(to_qs(m_releaseVersion.current_version.to_string()));
  ui->availableVersion->setText(to_qs(m_releaseVersion.latest_source.to_string()));
  ui->close->setText(QY("&Close"));

  auto url = m_releaseVersion.urls.find("general");
  if ((url != m_releaseVersion.urls.end()) && !url->second.empty()) {
    m_downloadURL = to_qs(url->second);
    ui->downloadURL->setText(u"<html><body><a href=\"%1\">%1</a></body></html>"_s.arg(m_downloadURL.toHtmlEscaped()));
    ui->download->setEnabled(true);
  }
}

QString
AvailableUpdateInfoDialog::formattedVersionHeading(pugi::xpath_node const &release) {
  auto versionStrQ = Q(std::string{release.node().attribute("version").value()}).toHtmlEscaped();
  auto song        = Q(release.node().attribute("codename").value());
  auto album       = Q(release.node().attribute("codename-album").value());
  auto artist      = Q(release.node().attribute("codename-artist").value());

  if (song.isEmpty() || artist.isEmpty())
    return QY("Version %1").arg(versionStrQ);

  QString links;
  auto songQ  = song.toHtmlEscaped();
  auto urlBase    = u"https://www.youtube.com/results"_s;
  auto urlSong    = QUrl{urlBase};
  auto querySong  = QUrlQuery{};

  querySong.addQueryItem(u"search_query"_s, u"%1 %2"_s.arg(artist).arg(song));
  urlSong.setQuery(querySong);

  if (!album.isEmpty() && (song.toLower() != album.toLower())) {
    auto urlAlbum    = QUrl{urlBase};
    auto queryAlbum  = QUrlQuery{};

    queryAlbum.addQueryItem(u"search_query"_s, u"%1 %2"_s.arg(artist).arg(album));
    urlAlbum.setQuery(queryAlbum);

    links = QY("Listen to <a href=\"%1\">song</a> or <a href=\"%2\">album</a> on YouTube")
      .arg(urlSong .toString(QUrl::FullyEncoded))
      .arg(urlAlbum.toString(QUrl::FullyEncoded));

  } else
    links = QY("Listen to <a href=\"%1\">song</a> on YouTube").arg(urlSong.toString(QUrl::FullyEncoded));

  return u"%1 [%2]"_s
    .arg(QY("Version %1 \"%2\"").arg(versionStrQ).arg(songQ))
    .arg(links);
}

void
AvailableUpdateInfoDialog::updateReleasesInfoDisplay() {
  if (!m_releasesInfo) {
    setChangeLogContent(u""_s);
    return;
  }

  if (!m_statusRetrieved || (UpdateCheckStatus::Failed == m_status))
    return;

  auto html              = QStringList{};
  auto numReleasesOutput = 0;
  auto releases          = m_releasesInfo->select_nodes("/mkvtoolnix-releases/release[not(@version='HEAD')]");
  auto reReleased        = QRegularExpression{"^released\\s+v?[\\d\\.]+"};
  auto reBug             = QRegularExpression{"(#\\d+)"};
  auto reNewlines        = QRegularExpression{"\r?\n"};
  auto bugFormatter      = [](QRegularExpressionMatch const &matches) {
    auto number_str = matches.captured(1).mid(1);
    return u"<a href=\"%2%1\">#%1</a>"_s.arg(number_str).arg(MTX_URL_ISSUES);
  };

  releases.sort();

  for (auto &release : releases) {
    auto versionStr  = std::string{release.node().attribute("version").value()};
    auto headingQ     = formattedVersionHeading(release);
    auto currentTypeQ = QString{};
    auto inList       = false;

    html << u"<h2>%1</h2>"_s.arg(headingQ);

    for (auto change = release.node().child("changes").first_child() ; change ; change = change.next_sibling()) {
      if (   (std::string{change.name()} != "change")
          || Q(change.child_value()).contains(reReleased))
        continue;

      auto typeQ = Q(change.attribute("type").value()).toHtmlEscaped();
      if (typeQ != currentTypeQ) {
        currentTypeQ = typeQ;

        if (inList) {
          inList = false;
          html << u"</ul></p>"_s;
        }

        html << u"<h3>%1</h3>"_s.arg(typeQ);
      }

      if (!inList) {
        inList = true;
        html << u"<p><ul>"_s;
      }

      auto text = mtx::string::replace(Q(mtx::markdown::to_html(change.child_value())), reBug,  bugFormatter).replace(reNewlines, " ");
      html     << u"<li>%1</li>"_s.arg(text);
    }

    if (inList)
      html << u"</ul></p>"_s;

    numReleasesOutput++;
    if ((10 < numReleasesOutput) && (version_number_t{versionStr} < m_releaseVersion.current_version))
      break;
  }

  html << u"<p><a href=\"%1\">%2</a></h1>"_s.arg(UpdateChecker::newsURL()).arg(QY("Read full NEWS.md file online"));

  setChangeLogContent(html.join(u"\n"_s));
}

void
AvailableUpdateInfoDialog::visitDownloadLocation() {
  QDesktopServices::openUrl(m_downloadURL);
}

}

#endif  // HAVE_UPDATE_CHECK
