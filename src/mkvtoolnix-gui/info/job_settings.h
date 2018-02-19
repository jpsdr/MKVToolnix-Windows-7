#pragma once

#include "common/common_pch.h"

#include <QString>

namespace mtx { namespace gui { namespace Info {

struct JobSettings {
  enum class Mode {
    Tree,
    Summary,
  };

  enum class Verbosity {
    StopAtFirstCluster,
    AllElements,
  };

  enum class HexDumps {
    None,
    First16Bytes,
    AllBytes,
  };

  QString m_fileName;
  Mode m_mode{Mode::Tree};
  Verbosity m_verbosity{Verbosity::StopAtFirstCluster};
  HexDumps m_hexDumps{HexDumps::None};
  bool m_checksums{}, m_trackInfo{}, m_hexPositions{};
};

}}}
