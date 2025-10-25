/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   file name sequence detection

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include <QString>

namespace mtx::sequenced_file_names {

struct SequencedFileNameData {
  enum class Mode {
    PrefixNumberSuffix,
    GoPro,
  };

  Mode mode;
  QString prefix, suffix;
  unsigned int number{};

  bool follows(SequencedFileNameData const &previous) const;
};

std::optional<SequencedFileNameData> analyzeFileNameForSequenceData(QString const &fileNameWithPath);

}
