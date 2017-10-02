#pragma once

#include "common/common_pch.h"

class QStringList;

namespace mtx { namespace gui { namespace Util {

// Container stuff
template<typename Tstored, typename Tcontainer>
int
findPtr(Tstored *needle,
        Tcontainer const &haystack) {
  auto itr = brng::find_if(haystack, [needle](auto const &cmp) { return cmp.get() == needle; });
  return haystack.end() == itr ? -1 : std::distance(haystack.begin(), itr);
}

std::vector<std::string> toStdStringVector(QStringList const &strings, int offset = 0);
QStringList toStringList(std::vector<std::string> const &stdStrings, int offset = 0);

}}}
