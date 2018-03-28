#include "common/common_pch.h"

#include <QDebug>
#include <QFont>
#include <QFontDatabase>
#include <QLocale>

#include "common/checksums/base.h"
#include "common/kax_element_names.h"
#include "common/kax_info.h"
#include "common/qt.h"
#include "common/strings/formatting.h"
#include "mkvtoolnix-gui/forms/info/element_viewer_dialog.h"
#include "mkvtoolnix-gui/info/element_viewer_dialog.h"
#include "mkvtoolnix-gui/info/model.h"
#include "mkvtoolnix-gui/main_window/main_window.h"

namespace mtx { namespace gui { namespace Info {

namespace {

QString
monospaceFontFamily() {
  return QFontDatabase::systemFont(QFontDatabase::FixedFont).family();
}

}

using namespace mtx::gui;

class ElementViewerDialogPrivate {
public:
  std::unique_ptr<Ui::ElementViewerDialog> m_ui;
  ElementHighlighter::Highlights m_highlights;
  boost::optional<uint64_t> m_signaledSize;
  uint64_t m_effectiveSize{}, m_numBytesShown{};
  uint32_t m_elementId{};

  ElementViewerDialogPrivate()
    : m_ui{new Ui::ElementViewerDialog}
  {
  }
};

ElementViewerDialog::ElementViewerDialog(QWidget *parent)
  : QDialog{parent}
  , p_ptr{new ElementViewerDialogPrivate}
{
  // Setup UI controls.
  p_ptr->m_ui->setupUi(this);

  connect(p_ptr->m_ui->closeButton,        &QPushButton::clicked,           this, &ElementViewerDialog::accept);
  connect(p_ptr->m_ui->detachWindowButton, &QPushButton::clicked,           this, &ElementViewerDialog::requestDetachingWindow);
  connect(MainWindow::get(),               &MainWindow::preferencesChanged, this, &ElementViewerDialog::retranslateUi);
}

ElementViewerDialog::~ElementViewerDialog() {
}

int
ElementViewerDialog::exec() {
  retranslateUi();
  adjustSize();
  return QDialog::exec();
}

void
ElementViewerDialog::requestDetachingWindow() {
  done(DetachWindow);
}

void
ElementViewerDialog::detachWindow() {
  auto p        = p_func();
  auto geometry = saveGeometry();

  connect(this, &QDialog::finished, this, &QDialog::deleteLater);

  setModal(false);

  p->m_ui->detachWindowButton->hide();
  show();
  restoreGeometry(geometry);
}

QString
ElementViewerDialog::elementName()
  const {
  auto p = p_func();

  if (p->m_elementId == PseudoTypes::Frame)
    return QY("Frame");

  auto name = Q(kax_element_names_c::get(p->m_elementId));

  if (!name.isEmpty())
    return name;

  return Q(boost::format(Y("Unknown element (ID: 0x%1%)")) % kax_info_c::format_ebml_id_as_hex(p->m_elementId));
}

void
ElementViewerDialog::retranslateUi() {
  auto p       = p_func();
  auto name    = elementName();
  auto locale  = QLocale::system();
  auto haveAll = p->m_numBytesShown == p->m_effectiveSize;

  setWindowTitle(QY("Element viewer: %1").arg(name));

  p->m_ui->retranslateUi(this);
  p->m_ui->size->setText(p->m_signaledSize ? locale.toString(static_cast<quint64>(*p->m_signaledSize)) : QY("unknown"));
  p->m_ui->name->setText(name);

  if (!haveAll)
    p->m_ui->limitedBytesShownLabel->setText(QY("Only the first %1 bytes are shown.").arg(locale.toString(static_cast<quint64>(p->m_numBytesShown))));

  p->m_ui->limitedBytesShownLabel->setVisible(!haveAll);
  p->m_ui->adler32Label->setVisible(haveAll);
  p->m_ui->adler32->setVisible(haveAll);
}

QString
ElementViewerDialog::createHtmlHead(ElementHighlighter::Highlights const &highlights,
                                    QString const &css) {
  QStringList legend;

  legend << Q("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
              "<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
              "%1\n"
              "p, li { white-space: pre-wrap; }\n").arg(css);

  auto idx = 0;

  for (auto const &highlight : highlights)
    legend << Q(".c%1 { color: %2; background-color: %3; }\n")
      .arg(idx++)
      .arg(highlight.m_foregroundColor.name())
      .arg(highlight.m_backgroundColor.name());

  legend << Q("</style></head>");

  return legend.join(QString{});
}

QString
ElementViewerDialog::createHexDump(memory_c const &mem,
                                   ElementHighlighter::Highlights const &highlights) {
  QHash<unsigned char, QString> htmlChars;

  htmlChars['<']        = Q("&lt;");
  htmlChars['>']        = Q("&gt;");
  htmlChars['"']        = Q("&quot;");
  htmlChars[' ']        = Q("&nbsp;");
  auto dot              = Q(".");
  auto endSpanTag       = Q("</span>");
  auto hexChars         = Q("0123456789abcdef");
  auto inLine           = false;
  auto position         = 0u;
  auto buffer           = mem.get_buffer();
  auto size             = mem.get_size();
  auto lines            = QStringList{ createHtmlHead(highlights, Q("body { font-family: '%1'; }").arg(monospaceFontFamily())) };
  auto currentHighlight = highlights.begin();
  auto lastHighlight    = highlights.end();
  auto highlightIdx     = 0;

  QStringList hex;
  QString ascii, span;

  lines << Q("<body><p>");

  while (position < size) {
    QString spanStart, spanEnd;

    if ((currentHighlight != lastHighlight) && (currentHighlight->m_start == position)) {
      span      = Q("<span class=\"c%1\">").arg(highlightIdx++);
      spanStart = span;

    } else if (!inLine)
      spanStart = span;

    if ((currentHighlight != lastHighlight) && ((currentHighlight->m_start + currentHighlight->m_length) == (position + 1))) {
      ++currentHighlight;
      span.clear();
      spanEnd = endSpanTag;
    }

    inLine       = true;
    auto byte    = buffer[position];
    auto oneChar = (byte < 32) || (byte >= 127) ? dot
                 : htmlChars.contains(byte)     ? htmlChars.value(byte)
                 :                                QString::fromLatin1(reinterpret_cast<char const *>(&buffer[position]), 1);

    hex << Q("%1%2%3%4")
      .arg(spanStart)
      .arg(hexChars[byte >> 4])
      .arg(hexChars[byte & 0x0f])
      .arg(spanEnd);

    ascii += Q("%1%2%3")
      .arg(spanStart)
      .arg(oneChar)
      .arg(spanEnd);

    if (!((position + 1) % 8)) {
      inLine  = false;
      lines  << Q("%1  %2%3  %4%5<br>")
        .arg(Q(boost::format("%|1$08x|") % (position - 7)))
        .arg(hex.join(Q(" ")))
        .arg(span.isEmpty() ? QString{} : endSpanTag)
        .arg(ascii)
        .arg(span.isEmpty() ? QString{} : endSpanTag);
      hex.clear();
      ascii.clear();
    }

    ++position;
  }

  if (inLine)
    lines << Q("%1  %2%3%4  %5%6<br>")
      .arg(Q(boost::format("%|1$08x|") % ((position / 8) * 8)))
      .arg(hex.join(Q(" ")))
      .arg(span.isEmpty() ? QString{} : endSpanTag)
      .arg(Q(' '), (8 - hex.size()) * 3)
      .arg(ascii)
      .arg(span.isEmpty() ? QString{} : endSpanTag);

  lines << Q("</p></body></html>");

  return lines.join(QString{});
}

QString
ElementViewerDialog::createLegend(ElementHighlighter::Highlights const &highlights) {
  if (highlights.isEmpty())
    return {};

  auto idx    = 0;
  auto legend = createHtmlHead(highlights, Q(".monospace { font-family: '%1'; }").arg(monospaceFontFamily()));

  legend += Q("<body><p>");

  for (auto const &highlight : highlights) {
    if (!highlight.m_label.isEmpty())
      legend += Q("<span class=\"monospace c%1\">....</span> %2<br>").arg(idx).arg(highlight.m_label);
    ++idx;
  }

  return legend + Q("</p></body></html>");
}

ElementViewerDialog &
ElementViewerDialog::setContent(memory_c const &mem,
                                ElementHighlighter::Highlights const &highlights) {
  auto p             = p_func();
  p->m_highlights    = highlights;
  p->m_numBytesShown = mem.get_size();

  p->m_ui->legend      ->setVisible(!highlights.isEmpty());
  p->m_ui->legendLabel->setVisible(!highlights.isEmpty());

  p->m_ui->content->setText(createHexDump(mem, highlights));
  p->m_ui->legend ->setHtml(createLegend(highlights));

  p->m_ui->adler32->setText(Q(boost::format("0x%|1$08x|") % mtx::checksum::calculate_as_uint(mtx::checksum::algorithm_e::adler32, mem.get_buffer(), mem.get_size())));

  return *this;
}

ElementViewerDialog &
ElementViewerDialog::setId(uint32_t id) {
  p_func()->m_elementId = id;
  return *this;
}

ElementViewerDialog &
ElementViewerDialog::setPosition(uint64_t position) {
  p_func()->m_ui->position->setText(QLocale::system().toString(static_cast<quint64>(position)));
  return *this;
}

ElementViewerDialog &
ElementViewerDialog::setSize(boost::optional<uint64_t> signaledSize,
                             uint64_t effectiveSize) {
  auto p             = p_func();
  p->m_effectiveSize = effectiveSize;
  p->m_signaledSize  = signaledSize;

  return *this;
}

}}}
