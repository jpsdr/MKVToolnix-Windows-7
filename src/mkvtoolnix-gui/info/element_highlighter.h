#pragma once

#include "common/common_pch.h"

namespace mtx { namespace gui { namespace Info {

class ElementHighlighter {
public:
  struct Highlight {
    unsigned int m_start, m_length;
    QColor m_foregroundColor, m_backgroundColor;
    QString m_label;
  };

  using Highlights = QVector<Highlight>;

public:
  static Highlights highlightsForElement(memory_c const &mem);
};

}}}
