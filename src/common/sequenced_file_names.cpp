/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   file name sequence detection

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include <QFileInfo>
#include <QRegularExpression>

#include "common/qt.h"
#include "common/sequenced_file_names.h"

namespace mtx::sequenced_file_names {

bool
SequencedFileNameData::follows(SequencedFileNameData const &previous)
  const {
  if (mode != previous.mode)
    return false;

  return (prefix == previous.prefix)
    && (suffix == previous.suffix)
    && (number == (previous.number + 1));
}

std::optional<SequencedFileNameData>
analyzeFileNameForSequenceData(QString const &fileNameWithPath) {
  static std::optional<QRegularExpression> s_rePrefixNumberSuffix, s_reGoPro2First, s_reGoPro2FollowingOr6;

  if (!s_rePrefixNumberSuffix.has_value()) {
    s_rePrefixNumberSuffix = QRegularExpression{Q(R"((.*?)(\d+)([^\d]+)$)")};

    // see https://community.gopro.com/s/article/GoPro-Camera-File-Naming-Convention?language=en_US
    s_reGoPro2First        = QRegularExpression{Q(R"(^GOPR(\d+.*))")};
    s_reGoPro2FollowingOr6 = QRegularExpression{Q(R"(^(G[PHX])(\d\d)(\d+.*))")};
  }

  auto fileName = QFileInfo{fileNameWithPath}.fileName();

  if (auto match = s_reGoPro2First->match(fileName); match.hasMatch())
    return SequencedFileNameData{ SequencedFileNameData::Mode::GoPro, Q("GP"), match.captured(1), 0 };

  if (auto match = s_reGoPro2FollowingOr6->match(fileName); match.hasMatch())
    return SequencedFileNameData{ SequencedFileNameData::Mode::GoPro, match.captured(1), match.captured(3), match.captured(2).toUInt() };

  if (auto match = s_rePrefixNumberSuffix->match(fileName); match.hasMatch())
    return SequencedFileNameData{ SequencedFileNameData::Mode::PrefixNumberSuffix, match.captured(1), match.captured(3), match.captured(2).toUInt() };

  return {};
}

}
