#include "common/common_pch.h"

#include <QColor>
#include <QVector>

#include "common/bit_reader.h"
#include "common/qt.h"
#include "mkvtoolnix-gui/info/element_highlighter.h"

namespace mtx { namespace gui { namespace Info {

using namespace mtx::gui;

ElementHighlighter::Highlights
ElementHighlighter::highlightsForElement(memory_c const &mem) {
  Highlights highlights;

  mtx::bits::reader_c r{mem.get_buffer(), mem.get_size()};

  auto vint_length = [](uint8_t b) -> unsigned int {
    for (int shift = 7; shift > 0; --shift)
      if (b & (1 << shift))
        return 8 - shift;

    return 8;
  };

  try {
    auto length   = vint_length(r.get_bits(8));
    auto position = length;

    highlights << Highlight{ 0u, length, "#000000", "#aaff7f", QY("Element ID") };

    r.set_bit_position(position * 8);

    length = vint_length(r.get_bits(8));

    highlights << Highlight{ position, length, "#000000", "#ffff7f", QY("Element size") };

    position += length;
    r.set_bit_position(position * 8);

  } catch (mtx::mm_io::end_of_file_x &) {
  }

  return highlights;
}

}}}
