#include "common/common_pch.h"

#include <QColor>
#include <QLocale>
#include <QVector>

#include <matroska/KaxBlock.h>

#include "common/bit_reader.h"
#include "common/math.h"
#include "common/qt.h"
#include "mkvtoolnix-gui/info/element_highlighter.h"

namespace mtx::gui::Info {

using namespace mtx::gui;

namespace {

unsigned int
vintLength(uint8_t b) {
  for (int shift = 7; shift > 0; --shift)
    if (b & (1 << shift))
      return 8 - shift;

  return 8;
}

unsigned int
vintValue(unsigned int length,
          uint64_t vint) {
  // len = 3
  //                            1 << length
  //                   00001000 - 1
  //                   00000111 << (8 - length)
  //                   11100000 << (8 * (length - 1))
  // 11100000 00000000 00000000 ~
  // 00011111 11111111 11111111

  auto mask = ~(((1ull << length) - 1) << (8 - length + (8 * (length - 1))));
  return vint & mask;
}

int
vintSignedValue(unsigned int length,
                uint64_t vint) {
  // len = 3
  //                            1 << length
  //                   00001000 - 1
  //                   00000111 << (8 - length)
  //                   11100000 << (8 * (length - 1))
  // 11100000 00000000 00000000 ~
  // 00011111 11111111 11111111

  auto signedValue = static_cast<int>(vintValue(length, vint));

  return signedValue - (  length == 1 ?        63
                        : length == 2 ?      8191
                        : length == 3 ?   1048575
                        :               134217727);
}

QString
vintExplanation(unsigned int length,
                uint64_t vint,
                bool withValue) {
  auto explanation = Q("<span class=\"monospace\">&nbsp;%1</span>&nbsp;%2")
    .arg(Q(std::string(length - 1u, '0') + "1" + std::string(8u - length, '.')))
    .arg(QNY("Length: %1 byte", "Length: %1 bytes", length).arg(length).toHtmlEscaped());

  if (!withValue)
    return explanation;

  return Q("%1<br><span class=\"monospace\">&nbsp;%2</span>&nbsp;%3")
    .arg(explanation)
    .arg(Q(std::string(length, '.') + std::string(8u - length, '#')))
    .arg(QY("Value: %1").arg(QLocale::system().toString(static_cast<quint64>(vintValue(length, vint)))).toHtmlEscaped());
}

QColor
laceColor(int laceNum) {
  // auto r = 0xff - ((0xff - 0xbf) * laceNum / 8);
  // auto g = 0xaa - ((0xaa - 0x7f) * laceNum / 8);
  auto r  = 0xcf - ((0xcf - 0x9e) * laceNum / 8);
  auto gb = 0xff - ((0xff - 0xc3) * laceNum / 8);
  return { r, gb, gb };
}

QColor
frameColor(int frameNum) {
  auto value = 0xf8 - (8 * frameNum);
  return { value, value, value };
}

QVector<unsigned int>
highlightLaceTypeEBML(mtx::bits::reader_c &r,
                      ElementHighlighter::Highlights &highlights,
                      unsigned int numFrames,
                      unsigned int totalFrameSize) {
  QVector<unsigned int> frameSizes;
  unsigned int size{};

  for (auto idx = 0u; idx < numFrames - 1u; ++idx) {
    unsigned int position  = r.get_bit_position() / 8;
    auto length            = vintLength(r.peek_bits(8));
    auto rawSize           = r.get_bits(length * 8);
    auto diffSize          = idx == 0 ? vintValue(length, rawSize) : vintSignedValue(length, rawSize);
    size                  += diffSize;
    frameSizes            << size;
    totalFrameSize        -= size;
    totalFrameSize        -= length;

    highlights            << ElementHighlighter::Highlight{ position, length, "#000000", laceColor(idx), QY("Frame size #%1: %2").arg(idx).arg(QLocale::system().toString(static_cast<quint64>(size))) };
  }

  frameSizes << totalFrameSize;

  return frameSizes;
}

QVector<unsigned int>
highlightLaceTypeXiph(mtx::bits::reader_c &r,
                      ElementHighlighter::Highlights &highlights,
                      unsigned int numFrames,
                      unsigned int totalFrameSize) {
  QVector<unsigned int> frameSizes;

  for (auto idx = 0u; idx < numFrames - 1u; ++idx) {
    uint64_t size{}, byte{};
    unsigned int position = r.get_bit_position() / 8;

    do {
      byte  = r.get_bits(8);
      size += byte;
    } while (byte == 0xff);

    unsigned int length  = r.get_bit_position() / 8 - position;
    frameSizes          << size;
    totalFrameSize      -= size;
    totalFrameSize      -= length;

    highlights          << ElementHighlighter::Highlight{ position, length, "#000000", laceColor(idx), QY("Frame size #%1: %2").arg(idx).arg(QLocale::system().toString(static_cast<quint64>(size))) };
  }

  frameSizes << totalFrameSize;

  return frameSizes;
}

QVector<unsigned int>
highlightLaceTypeFixed(unsigned int numFrames,
                       unsigned int totalFrameSize) {
  return QVector<unsigned int>(static_cast<int>(numFrames), static_cast<unsigned int>(totalFrameSize / numFrames));
}

void
highlightBlockOrSimpleBlock(mtx::bits::reader_c &r,
                            ElementHighlighter::Highlights &highlights,
                            bool isSimpleBlock) {
  auto locale      = QLocale::system();
  auto length      = vintLength(r.peek_bits(8));
  auto trackNumber = r.get_bits(8 * length);
  auto text        = Q("%1<br>%2").arg(QY("Track number:").toHtmlEscaped()).arg(vintExplanation(length, trackNumber, true));

  highlights << ElementHighlighter::Highlight{ r.get_bit_position() / 8 - length, length, "#000000", "#bbbbff", text };

  auto timestamp = mtx::math::to_signed(r.get_bits(2 * 8));
  text           = QY("Relative timestamp: %1").arg(locale.toString(static_cast<qint64>(timestamp))).toHtmlEscaped();

  highlights << ElementHighlighter::Highlight{ r.get_bit_position() / 8 - 2u, 2, "#000000", "#ffaaff", text };

  auto flags       = r.get_bits(8);
  auto lacingValue = (flags >> 1) & 0x03;
  auto lacingName  = lacingValue == 0x00 ? QY("No lacing")
                   : lacingValue == 0x01 ? QY("Xiph lacing")
                   : lacingValue == 0x02 ? QY("Fixed-sized lacing")
                   :                       QY("EBML lacing");

  QStringList lines;
  lines << QY("Flags:");

  if (isSimpleBlock)
    lines << Q("<span class=\"monospace\">&nbsp;%1.......</span>&nbsp;%2") .arg(flags & 0x80 ? 1 : 0)                          .arg(flags & 0x80 ? QY("Key frame").toHtmlEscaped()   : QY("Not a key frame").toHtmlEscaped());

  lines   << Q("<span class=\"monospace\">&nbsp;....%1...</span>&nbsp;%2") .arg(flags & 0x08 ? 1 : 0)                          .arg(flags & 0x08 ? QY("Invisible").toHtmlEscaped()   : QY("Visible").toHtmlEscaped());
  lines   << Q("<span class=\"monospace\">&nbsp;.....%1%2.</span>&nbsp;%3").arg(flags & 0x04 ? 1 : 0).arg(flags & 0x02 ? 1 : 0).arg(lacingName.toHtmlEscaped());
  lines   << Q("<span class=\"monospace\">&nbsp;.......%1</span>&nbsp;%2") .arg(flags & 0x01 ? 1 : 0)                          .arg(flags & 0x01 ? QY("Discardable").toHtmlEscaped() : QY("Not discardable").toHtmlEscaped());

  highlights << ElementHighlighter::Highlight{ r.get_bit_position() / 8u - 1u, 1, "#000000", "#00cc00", lines.join(Q("<br>")) };

  if (lacingValue == 0x00) {
    highlights << ElementHighlighter::Highlight{ r.get_bit_position() / 8u, r.get_remaining_bits() / 8u, "#000000", frameColor(0), QY("Frame").toHtmlEscaped() };
    return;
  }

  auto numFrames = r.get_bits(8) + 1;
  highlights << ElementHighlighter::Highlight{ r.get_bit_position() / 8u - 1u, 1, "#000000", "#dcdca5", QY("Number of frames: %1").arg(numFrames).toHtmlEscaped() };

  auto totalFrameSize = r.get_remaining_bits() / 8;
  auto frameSizes     = lacingValue == 0x01 ? highlightLaceTypeXiph(r, highlights, numFrames, totalFrameSize)
                      : lacingValue == 0x03 ? highlightLaceTypeEBML(r, highlights, numFrames, totalFrameSize)
                      :                       highlightLaceTypeFixed(              numFrames, totalFrameSize);
  auto frameIdx       = 0u;
  auto position       = r.get_bit_position() / 8u;

  for (auto const &frameSize : frameSizes) {
    highlights << ElementHighlighter::Highlight{ position, frameSize, "#000000", frameColor(frameIdx), QY("Frame #%1").arg(frameIdx).toHtmlEscaped() };
    position   += frameSize;
    frameIdx   += 1;
  }
}

} // anonymous namespace

ElementHighlighter::Highlights
ElementHighlighter::highlightsForElement(memory_c const &mem) {
  Highlights highlights;

  mtx::bits::reader_c r{mem.get_buffer(), mem.get_size()};

  try {
    auto length   = vintLength(r.peek_bits(8));
    auto id       = r.get_bits(length * 8);
    auto text     = Q("%1<br>%2").arg(QY("Element ID:").toHtmlEscaped()).arg(vintExplanation(length, id, false));

    highlights << Highlight{ 0u, length, "#000000", "#aaff7f", text };

    auto position = length;
    length        = vintLength(r.peek_bits(8));
    auto size     = r.get_bits(length * 8);
    text          = Q("%1<br>%2").arg(QY("Content size:").toHtmlEscaped()).arg(vintExplanation(length, size, true));

    highlights << Highlight{ position, length, "#000000", "#ffff7f", text };

    if (id == EBML_ID(libmatroska::KaxSimpleBlock).GetValue())
      highlightBlockOrSimpleBlock(r, highlights, true);

    else if (id == EBML_ID(libmatroska::KaxBlock).GetValue())
      highlightBlockOrSimpleBlock(r, highlights, false);

  } catch (mtx::mm_io::end_of_file_x &) {
  }

  return highlights;
}

}
