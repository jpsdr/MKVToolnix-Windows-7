#pragma once

#include "common/common_pch.h"

namespace mtx::gui::Util {

// Container stuff
template<typename Tstored, typename Tcontainer>
int
findPtr(Tstored *needle,
        Tcontainer const &haystack) {
  auto itr = std::find_if(haystack.begin(), haystack.end(), [needle](auto const &cmp) { return cmp.get() == needle; });
  return haystack.end() == itr ? -1 : std::distance(haystack.begin(), itr);
}

std::vector<std::string> toStdStringVector(QStringList const &strings, int offset = 0);
QStringList toStringList(std::vector<std::string> const &stdStrings, int offset = 0);

template<typename TQVector,
         typename TStdVector>
QVector<TQVector>
stdVectorToQVector(TStdVector const &v) {
  return QVector<TQVector>{v.begin(), v.end()};
}

template<typename T>
QSet<typename T::value_type>
qListToSet(T const &l) {
  return QSet<typename T::value_type>{l.begin(), l.end()};
}

template<typename T>
QList<typename T::value_type>
qSetToList(T const &l) {
  return QList<typename T::value_type>{l.begin(), l.end()};
}

}
