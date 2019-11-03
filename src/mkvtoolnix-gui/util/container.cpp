#include "common/common_pch.h"

#include <QStringList>

#include "common/qt.h"
#include "mkvtoolnix-gui/util/container.h"

namespace mtx::gui::Util {

std::vector<std::string>
toStdStringVector(QStringList const &strings,
                  int offset) {
  auto stdStrings = std::vector<std::string>{};
  auto numStrings = strings.count();

  if (offset > numStrings)
    return stdStrings;

  stdStrings.reserve(numStrings - offset);

  for (auto idx = offset; idx < numStrings; ++idx)
    stdStrings.emplace_back(to_utf8(strings[idx]));

  return stdStrings;
}

QStringList
toStringList(std::vector<std::string> const &stdStrings,
             int offset) {
  auto strings       = QStringList{};
  auto numStdStrings = static_cast<int>(stdStrings.size());

  if (offset > numStdStrings)
    return strings;

  strings.reserve(numStdStrings - offset);

  for (auto idx = offset; idx < numStdStrings; ++idx)
    strings << to_qs(stdStrings[idx]);

  return strings;
}

}
